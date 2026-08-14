#include "Game/Systems/CameraSystem.h"

#include <Core/App.h>
#include <Core/Extension/World/ECS/World.h>

bool FCameraSystem::ExecuteStage(Maho::EEngineStage Stage, float DeltaTime, Maho::FWorld& World)
{
	if (Stage != Maho::EEngineStage::Update)
	{
		return true;
	}

	(void)World;
	(void)DeltaTime;

	// Placeholder — camera input will be wired here
	return true;
}
