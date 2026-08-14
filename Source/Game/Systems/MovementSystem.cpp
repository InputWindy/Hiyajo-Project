#include "Game/Systems/MovementSystem.h"

#include <Core/Extension/World/ECS/World.h>
#include <Core/Extension/World/ECS/Query.h>
#include "Game/Components/TransformComponent.h"
#include "Game/Components/AllComponents.h"

void FMovementSystem::OnUpdate(float DeltaTime, Maho::FWorld& World)
{
	// Rotate dynamic entities only — exclude the camera (has FCameraComponent).
	auto Query = World.Query<Maho::FTransformComponent>().Not<Maho::FCameraComponent>();
	Query.ForEach([DeltaTime](Maho::FEntityHandle /*Handle*/, Maho::FTransformComponent& Transform)
	{
		Transform.RotateY(0.01f * DeltaTime);
		Transform.ComputeLocalToWorld();
	});
}
