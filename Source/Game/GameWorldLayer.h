#pragma once

#include <Core/Extension/World/WorldLayer.h>

#include <memory>
#include <string>

namespace Maho
{
class FEntityScriptDispatcher;
}

/**
 * Project world layer: registers game systems and spawns initial entities
 * on top of the engine FWorldLayer lifecycle.
 */
class GameWorldLayer : public Maho::FWorldLayer
{
public:
	explicit GameWorldLayer(std::string WorldName = "MainWorld");

protected:
	void RegisterSystems(Maho::FSystemGroup& SimGroup) override;
	void SpawnInitialEntities(Maho::FWorld& World) override;
	void OnStageDispatched(Maho::EEngineStage Stage, float DeltaTime) override;

private:
	std::unique_ptr<Maho::FEntityScriptDispatcher> ScriptDispatcher;
};
