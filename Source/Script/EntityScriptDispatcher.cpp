#include "Script/EntityScriptDispatcher.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <Core/System/Log.h>

#include "Script/LuaComponentBindings.h"

#include <filesystem>

namespace Maho
{

namespace
{

[[nodiscard]] std::string ResolveScriptPath(const std::string& ScriptsDirectory, const std::string& FilePath)
{
	namespace fs = std::filesystem;
	const fs::path Path = FilePath;
	if (Path.is_absolute())
	{
		return Path.string();
	}
	return (fs::path(ScriptsDirectory) / Path).string();
}

[[nodiscard]] std::uint32_t PackEntityKey(FEntityHandle Handle)
{
	return (static_cast<std::uint32_t>(Handle.Index) << 8) | static_cast<std::uint32_t>(Handle.Generation);
}

} // namespace

struct FEntityScriptDispatcher::FImpl
{
	sol::state* Lua = nullptr;
	std::string ScriptsDirectory;
	std::unordered_map<std::string, sol::table> ScriptPrototypes;
	std::unordered_map<std::uint32_t, sol::table> EntityInstances;
};

FEntityScriptDispatcher::FEntityScriptDispatcher(FScriptSystem& Script)
	: Impl(std::make_unique<FImpl>())
{
	Impl->ScriptsDirectory = Script.GetScriptsDirectory();
	Impl->Lua = static_cast<sol::state*>(Script.TryGetLuaState());

	static bool bRegisteredBindings = false;
	if (!bRegisteredBindings && Impl->Lua)
	{
		RegisterLuaComponentBindings(*Impl->Lua);
		bRegisteredBindings = true;
	}
}

FEntityScriptDispatcher::~FEntityScriptDispatcher() = default;

void FEntityScriptDispatcher::Dispatch(FEntityHandle Handle, const char* ScriptPath, FTransformComponent* Transform, float DeltaTime, const char* HookName)
{
	if (!Impl || !Impl->Lua || !ScriptPath || ScriptPath[0] == '\0' || !HookName || HookName[0] == '\0')
	{
		return;
	}

	sol::state& Lua = *Impl->Lua;
	const std::string Resolved = ResolveScriptPath(Impl->ScriptsDirectory, ScriptPath);

	// 1. Load / cache script prototype (script returns a table of hooks).
	sol::table Prototype;
	auto ProtoIt = Impl->ScriptPrototypes.find(Resolved);
	if (ProtoIt == Impl->ScriptPrototypes.end())
	{
		sol::protected_function_result Result = Lua.safe_script_file(Resolved);
		if (!Result.valid())
		{
			const sol::error Error = Result;
			MAHO_CORE_ERROR("FEntityScriptDispatcher::Dispatch('{}'): {}", Resolved, Error.what());
			return;
		}
		if (Result.return_count() < 1)
		{
			MAHO_CORE_ERROR("FEntityScriptDispatcher::Dispatch('{}'): script must return a table", Resolved);
			return;
		}
		sol::object Returned = Result[0];
		if (!Returned.is<sol::table>())
		{
			MAHO_CORE_ERROR("FEntityScriptDispatcher::Dispatch('{}'): script must return a table", Resolved);
			return;
		}
		Prototype = Returned.as<sol::table>();
		Impl->ScriptPrototypes[Resolved] = Prototype;
	}
	else
	{
		Prototype = ProtoIt->second;
	}

	// 2. Get or create per-entity instance (prototype as metatable __index).
	const std::uint32_t Key = PackEntityKey(Handle);
	sol::table Instance;
	auto InstIt = Impl->EntityInstances.find(Key);
	if (InstIt == Impl->EntityInstances.end())
	{
		Instance = Lua.create_table_with();
		sol::table Metatable = Lua.create_table_with();
		Metatable["__index"] = Prototype;
		Instance[sol::metatable_key] = Metatable;
		Impl->EntityInstances[Key] = Instance;

		sol::protected_function OnBegin = Instance["OnBegin"];
		if (OnBegin.valid())
		{
			sol::protected_function_result Result = OnBegin(Instance, DeltaTime);
			if (!Result.valid())
			{
				const sol::error Error = Result;
				MAHO_CORE_ERROR("FEntityScriptDispatcher::OnBegin('{}'): {}", Resolved, Error.what());
			}
		}
	}
	else
	{
		Instance = InstIt->second;
	}

	// 3. Mount the transform as a typed usertype pointer (refresh each dispatch).
	if (Transform)
	{
		Instance["Transform"] = Transform;
	}
	else
	{
		Instance["Transform"] = sol::nil;
	}

	// 4. Stage hook.
	sol::protected_function Hook = Instance[HookName];
	if (Hook.valid())
	{
		sol::protected_function_result Result = Hook(Instance, DeltaTime);
		if (!Result.valid())
		{
			const sol::error Error = Result;
			MAHO_CORE_ERROR("FEntityScriptDispatcher::{}({}): {}", HookName, Resolved, Error.what());
		}
	}
}

void FEntityScriptDispatcher::Destroy(FEntityHandle Handle)
{
	if (!Impl || !Impl->Lua)
	{
		return;
	}

	const std::uint32_t Key = PackEntityKey(Handle);
	auto It = Impl->EntityInstances.find(Key);
	if (It == Impl->EntityInstances.end())
	{
		return;
	}

	sol::table Instance = It->second;
	sol::protected_function OnDestroy = Instance["OnDestroy"];
	if (OnDestroy.valid())
	{
		sol::protected_function_result Result = OnDestroy(Instance);
		if (!Result.valid())
		{
			const sol::error Error = Result;
			MAHO_CORE_ERROR("FEntityScriptDispatcher::OnDestroy: {}", Error.what());
		}
	}

	Impl->EntityInstances.erase(It);
}

} // namespace Maho
