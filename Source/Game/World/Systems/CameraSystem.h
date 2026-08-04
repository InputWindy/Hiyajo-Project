#pragma once

#include "ECS/System.h"

class FCameraSystem : public Maho::ISystem
{
public:
	FCameraSystem() = default;
	void OnUpdate(float DeltaTime, Maho::FECSWorld& World) override;
	[[nodiscard]] const char* GetName() const override { return "CameraSystem"; }
};
