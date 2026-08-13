#pragma once

#include <ECS/World.h>
#include <ECS/SystemGroup.h>
#include <Core/Sequencer/EngineExtension.h>

#include <string>

/**
 * Owns the ECS world (pure data) + the root system group (driver skeleton).
 * Maps engine stages to SystemGroup lifecycle hooks.
 */
class FWorldLayer final : public Maho::FLayer
{
public:
	explicit FWorldLayer(std::string WorldName = "MainWorld");
	~FWorldLayer() override = default;

	bool ExecuteStage(Maho::EEngineStage Stage) override;

	[[nodiscard]] Maho::FWorld& GetWorld() { return World; }
	[[nodiscard]] const Maho::FWorld& GetWorld() const { return World; }

private:
	std::string WorldName;
	Maho::FWorld World;
	Maho::FInitializationSystemGroup RootGroup;
	bool bWorldReady = false;
};
