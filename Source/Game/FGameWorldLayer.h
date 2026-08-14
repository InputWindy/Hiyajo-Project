#pragma once

#include <Core/Extension/World/ECS/SystemGroup.h>

/**
 * Project world: the root system group + engine extension. Registers game
 * systems, spawns initial entities, and bridges the world to the render system
 * on top of the engine FSystemGroup driver.
 */
class FGameWorldLayer : public Maho::FInitializationSystemGroup
{
public:
	FGameWorldLayer();

	bool ExecuteStage(Maho::EEngineStage Stage) override;

protected:
	void RegisterSystems(Maho::FSystemGroup& SimGroup) override;
	void SpawnInitialEntities(Maho::FWorld& World) override;
};
