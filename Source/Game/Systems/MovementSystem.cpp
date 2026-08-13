#include "Game/Systems/MovementSystem.h"

#include <Core/Extension/World/ECS/World.h>
#include <Core/Extension/World/ECS/Query.h>
#include <Core/Extension/World/Components/TransformComponent.h>

void FMovementSystem::OnUpdate(float DeltaTime, Maho::FWorld& World)
{
	auto Query = World.Query<Maho::FTransformComponent>();
	Query.ForEach([DeltaTime](Maho::FEntityHandle /*Handle*/, Maho::FTransformComponent& Transform)
	{
		Transform.RotateY(0.01f * DeltaTime);
		Transform.ComputeLocalToWorld();
	});
}
