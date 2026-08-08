#pragma once

#include "ECS/World.h"

#include <Core/Sequencer/EngineExtension.h>

#include <string>

/**
 * Project layer that owns and ticks the ECS world.
 */
class FWorldLayer final : public Maho::FLayer
{
public:
	explicit FWorldLayer(std::string WorldName = "MainWorld");
	~FWorldLayer() override = default;

	virtual bool ExecuteStage(Maho::EEngineStage Stage) override;

	[[nodiscard]] Maho::FWorld& GetECSWorld() { return ECSWorld; }
	[[nodiscard]] const Maho::FWorld& GetECSWorld() const { return ECSWorld; }

private:
	std::string WorldName;
	Maho::FWorld ECSWorld;
	bool bWorldReady = false;
};
