#include "Game/GameWorldLayer.h"

#include <Core/Application/App.h>
#include <Core/Extension/Script/ScriptSystem.h>
#include <Core/Extension/World/ECS/EntityHandle.h>
#include <Core/Extension/World/ECS/Query.h>
#include <Core/Extension/World/ECS/SystemGroup.h>

#include "Game/Components/AllComponents.h"
#include "Game/Components/TransformComponent.h"
#include "Game/Systems/MovementSystem.h"
#include "Game/Systems/CameraSystem.h"
#include "Script/LuaComponentBindings.h"

#include <glm/glm.hpp>

#include <utility>

namespace
{

/** Register project Lua bindings once, after FScriptSystem owns a live sol::state. */
void EnsureLuaBindings()
{
	static bool bRegistered = false;
	if (bRegistered)
	{
		return;
	}

	Maho::FScriptSystem* Script = Maho::GApp ? Maho::GApp->GetExtension<Maho::FScriptSystem>() : nullptr;
	if (!Script || !Script->IsLuaInitialized())
	{
		return;
	}

	void* LuaState = Script->TryGetLuaState();
	if (!LuaState)
	{
		return;
	}

	Maho::RegisterLuaComponentBindings(*static_cast<sol::state*>(LuaState));
	bRegistered = true;
}

/** EEngineStage → per-entity script hook name (nullptr = no hook for this stage). */
[[nodiscard]] const char* GetScriptHookForStage(Maho::EEngineStage Stage)
{
	switch (Stage)
	{
	case Maho::EEngineStage::BeginFrame: return "OnBeginFrame";
	case Maho::EEngineStage::ProcessInput: return "OnProcessInput";
	case Maho::EEngineStage::FixedUpdate: return "OnFixedUpdate";
	case Maho::EEngineStage::Update: return "OnUpdate";
	case Maho::EEngineStage::LateUpdate: return "OnLateUpdate";
	case Maho::EEngineStage::EndFrame: return "OnEndFrame";
	case Maho::EEngineStage::PreRender: return "OnPreRender";
	case Maho::EEngineStage::PostRender: return "OnPostRender";
	default: return nullptr;
	}
}

} // namespace

GameWorldLayer::GameWorldLayer(std::string WorldName)
	: Maho::FWorldLayer(std::move(WorldName))
{
}

void GameWorldLayer::RegisterSystems(Maho::FSystemGroup& SimGroup)
{
	SimGroup.AddSystem<FMovementSystem>();
	SimGroup.AddSystem<FCameraSystem>();
}

void GameWorldLayer::SpawnInitialEntities(Maho::FWorld& World)
{
	// Unit cube at world origin (ForwardRenderer draws a cube per instance).
	{
		Maho::FEntityHandle Handle = World.CreateEntity();
		Maho::FTransformComponent Transform;
		Transform.SetIdentity();
		World.SetComponent<Maho::FTransformComponent>(Handle, Transform);
	}

	// Main camera: above + in front of the cube, tilted 45° down looking at origin.
	{
		Maho::FEntityHandle CamHandle = World.CreateEntity();
		World.AddTag<Maho::FMainCameraTag>(CamHandle);

		const glm::vec3 CamPos(0.0f, 5.0f, 5.0f);
		const glm::vec3 Target(0.0f, 0.0f, 0.0f);

		Maho::FTransformComponent CamTransform;
		CamTransform.SetIdentity();
		CamTransform.Position = CamPos;
		CamTransform.Rotation = glm::quatLookAt(
			glm::normalize(Target - CamPos),
			glm::vec3(0.0f, 1.0f, 0.0f));
		World.SetComponent<Maho::FTransformComponent>(CamHandle, CamTransform);

		Maho::FCameraComponent CamComp;
		CamComp.bMainCamera = true;
		CamComp.FOV = 60.0f;
		CamComp.NearPlane = 0.1f;
		CamComp.FarPlane = 1000.0f;
		CamComp.AspectRatio = 16.0f / 9.0f;
		World.SetComponent<Maho::FCameraComponent>(CamHandle, CamComp);
	}
}

void GameWorldLayer::OnStageDispatched(Maho::EEngineStage Stage, float DeltaTime)
{
	EnsureLuaBindings();

	const char* Hook = GetScriptHookForStage(Stage);
	if (!Hook)
	{
		return;
	}

	Maho::FScriptSystem* Script = Maho::GApp ? Maho::GApp->GetExtension<Maho::FScriptSystem>() : nullptr;
	if (!Script || !Script->IsLuaInitialized())
	{
		return;
	}

	auto Query = GetWorld().Query<Maho::FScriptComponent>();
	Query.ForEach([&](Maho::FEntityHandle Handle, Maho::FScriptComponent& Component)
	{
		if (!Component.bEnabled || !Component.IsValid())
		{
			return;
		}

		Maho::FTransformComponent* Transform =
			GetWorld().GetEntityManager().GetComponent<Maho::FTransformComponent>(Handle);

		Script->DispatchEntityScript(Handle, Component.ScriptPath, Transform, DeltaTime, Hook);
	});
}
