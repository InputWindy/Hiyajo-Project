#pragma once

#include "ECS/World.h"
#include "World/Systems/MovementSystem.h"
#include "World/Systems/CameraSystem.h"

#include <Core/Sequencer/EngineExtension.h>

/**
 * Project layer that owns and ticks the ECS world.
 */
class FWorldLayer final : public Maho::FLayer
{
public:
	explicit FWorldLayer(std::string WorldName = "MainWorld");
	~FWorldLayer() override = default;

	virtual bool ExecuteStage(Maho::EEngineStage Stage) override;

	[[nodiscard]] Maho::FECSWorld& GetECSWorld() { return ECSWorld; }
	[[nodiscard]] const Maho::FECSWorld& GetECSWorld() const { return ECSWorld; }

private:
	std::string WorldName;
	Maho::FECSWorld ECSWorld;
	FMovementSystem MovementSystem;
	FCameraSystem CameraSystem;
	bool bWorldReady = false;
};
