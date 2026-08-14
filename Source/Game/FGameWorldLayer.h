#pragma once

#include <Core/Extension/World/WorldLayer.h>

#include <string>

/**
 * Project world layer: registers game systems and spawns initial entities
 * on top of the engine FWorldLayer lifecycle.
 */
class FGameWorldLayer : public Maho::FWorldLayer
{
public:
	explicit FGameWorldLayer(std::string WorldName = "MainWorld");

protected:
	void RegisterSystems(Maho::FSystemGroup& SimGroup) override;
	void SpawnInitialEntities(Maho::FWorld& World) override;
};
