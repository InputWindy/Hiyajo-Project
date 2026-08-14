#pragma once

#include <Core/Extension/World/ECS/System.h>

#include <memory>

namespace Maho
{

/**
 * Game system that drives per-entity Lua scripts (same level as FMovementSystem).
 * Maps engine stages (ExecuteStage) to per-entity script hooks. Owns the script
 * prototype cache + per-entity instance tables.
 */
class FScriptDispatchSystem : public ISystem
{
public:
	FScriptDispatchSystem();
	~FScriptDispatchSystem() override;

	[[nodiscard]] const char* GetName() const override { return "ScriptDispatchSystem"; }

	bool ExecuteStage(EEngineStage Stage, float DeltaTime, FWorld& World) override;

private:
	struct FImpl;
	std::unique_ptr<FImpl> Impl;

	void DispatchStage(FWorld& InWorld, float DeltaTime, const char* HookName);
};

} // namespace Maho
