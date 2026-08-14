#pragma once

#include <Core/Extension/World/ECS/System.h>

class FCameraSystem : public Maho::ISystem
{
public:
	FCameraSystem() = default;
	bool ExecuteStage(Maho::EEngineStage Stage, float DeltaTime, Maho::FWorld& World) override;
	[[nodiscard]] const char* GetName() const override { return "CameraSystem"; }
	static const char* StaticName() { return "CameraSystem"; }
};
