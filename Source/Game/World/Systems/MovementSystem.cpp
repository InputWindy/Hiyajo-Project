#include "Game/World/Systems/MovementSystem.h"

#include <ECS/World.h>
#include <ECS/Query.h>
#include "Game/World/Components/TransformComponent.h"

namespace
{
	void ApplyRotation(Maho::FTransformComponent& Transform, float DeltaSeconds)
	{
		Transform.LocalToWorld[0] = Transform.LocalToWorld[0] + 0.01f * DeltaSeconds;
	}
}

void FMovementSystem::OnUpdate(float DeltaTime, Maho::FWorld& World)
{
	auto Query = World.Query<Maho::FTransformComponent>();
	Query.ForEach([DeltaTime](Maho::FEntityHandle /*Handle*/, Maho::FTransformComponent& Transform)
	{
		ApplyRotation(Transform, DeltaTime);
	});
}
