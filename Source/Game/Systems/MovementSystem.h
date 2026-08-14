#pragma once

#include <Core/Extension/World/ECS/System.h>
#include "Game/Components/TransformComponent.h"

/**
 * Demo system: rotates every entity that has a TransformComponent.
 */
class FMovementSystem final : public Maho::ISystem
{
public:
	[[nodiscard]] const char* GetName() const override { return "MovementSystem"; }
	static const char* StaticName() { return "MovementSystem"; }

	bool ExecuteStage(Maho::EEngineStage Stage, float DeltaTime, Maho::FWorld& World) override;
};
