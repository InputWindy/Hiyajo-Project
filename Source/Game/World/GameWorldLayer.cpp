#include "Game/World/GameWorldLayer.h"

#include <Core/Extension/World/Components/TransformComponent.h>
#include <Core/ECS/EntityHandle.h>
#include <Core/ECS/SystemGroup.h>

#include "Game/World/Systems/MovementSystem.h"
#include "Game/World/Systems/CameraSystem.h"
#include "Game/World/Systems/SceneGatherSystem.h"
#include "Game/Components/AllComponents.h"

#include <glm/glm.hpp>

#include <utility>

GameWorldLayer::GameWorldLayer(std::string WorldName)
	: Maho::FWorldLayer(std::move(WorldName))
{
}

void GameWorldLayer::RegisterSystems(Maho::FSystemGroup& SimGroup)
{
	SimGroup.AddSystem<FMovementSystem>();
	SimGroup.AddSystem<FCameraSystem>();
	SimGroup.AddSystem<FSceneGatherSystem>();
}

void GameWorldLayer::SpawnInitialEntities(Maho::FWorld& World)
{
	// Spawn demo entity with a TransformComponent.
	{
		Maho::FEntityHandle Handle = World.CreateEntity();
		Maho::FTransformComponent Transform;
		Transform.SetIdentity();
		World.SetComponent<Maho::FTransformComponent>(Handle, Transform);
	}

	// Main camera entity (engine-owned, tagged for editor hiding).
	{
		Maho::FEntityHandle CamHandle = World.CreateEntity();
		World.AddTag<Maho::FMainCameraTag>(CamHandle);

		Maho::FTransformComponent CamTransform;
		CamTransform.SetIdentity();
		CamTransform.Position = glm::vec3(0.0f, 0.0f, -5.0f);
		World.SetComponent<Maho::FTransformComponent>(CamHandle, CamTransform);

		Maho::FCameraComponent CamComp;
		CamComp.bMainCamera = true;
		CamComp.FOV = 60.0f;
		CamComp.NearPlane = 0.1f;
		CamComp.FarPlane = 1000.0f;
		CamComp.AspectRatio = 16.0f / 9.0f;
		World.SetComponent<Maho::FCameraComponent>(CamHandle, CamComp);
	}
}
