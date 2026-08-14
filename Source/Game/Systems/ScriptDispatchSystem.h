#pragma once

#include <Core/Extension/World/ECS/System.h>

#include <memory>

namespace Maho
{

/**
 * Game system that drives per-entity Lua scripts (same level as FMovementSystem).
 * Maps the ECS system stage hooks (OnUpdate/OnLateUpdate/…) to per-entity script
 * hooks. Owns the script prototype cache + per-entity instance tables.
 */
class FScriptDispatchSystem : public ISystem
{
public:
	FScriptDispatchSystem();
	~FScriptDispatchSystem() override;

	[[nodiscard]] const char* GetName() const override { return "ScriptDispatchSystem"; }

	void OnCreate(FWorld& World) override;
	void OnBeginFrame(FWorld& World) override;
	void OnFixedUpdate(float DeltaTime, FWorld& World) override;
	void OnUpdate(float DeltaTime, FWorld& World) override;
	void OnLateUpdate(float DeltaTime, FWorld& World) override;
	void OnEndFrame(FWorld& World) override;
	void OnPreRender(FWorld& World) override;
	void OnPostRender(FWorld& World) override;

private:
	struct FImpl;
	std::unique_ptr<FImpl> Impl;

	void DispatchStage(FWorld& World, float DeltaTime, const char* HookName);
};

} // namespace Maho
