#pragma once

#include <ECS/System.h>

/**
 * Drives per-entity Lua scripts. Queries entities with FScriptComponent
 * and forwards them to FScriptSystem::TickEntityScript each frame.
 */
class FScriptExecutionSystem final : public Maho::ISystem
{
public:
	[[nodiscard]] const char* GetName() const override { return "ScriptExecutionSystem"; }
	static const char* StaticName() { return "ScriptExecutionSystem"; }

	void OnUpdate(float DeltaTime, Maho::FWorld& World) override;
};
