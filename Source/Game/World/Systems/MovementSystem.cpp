#include "Game/World/Systems/MovementSystem.h"

#include "ECS/World.h"
#include "ECS/Query.h"
#include "Game/World/Components/TransformComponent.h"

namespace
{
	void ApplyRotation(Maho::FTransformComponent& Transform, float DeltaSeconds)
	{
		Transform.LocalToWorld[0] = Transform.LocalToWorld[0] + 0.01f * DeltaSeconds;
	}
}

void FMovementSystem::OnUpdate(float DeltaTime, Maho::FECSWorld& World)
{
	Maho::TComponentQuery<Maho::FTransformComponent> Query;
	Query.Gather(World.GetEntityManager());
	Query.ForEach([DeltaTime](Maho::FEntityHandle /*Handle*/, Maho::FTransformComponent& Transform)
	{
		ApplyRotation(Transform, DeltaTime);
	});
}
