#pragma once

#include <cstdint>
#include <cstring>

namespace Maho
{

/**
 * ECS component: world-space transform (4x4 column-major matrix).
 */
struct FTransformComponent
{
	float LocalToWorld[16] = {};

	FTransformComponent()
	{
		SetIdentity();
	}

	void SetIdentity()
	{
		std::memset(LocalToWorld, 0, sizeof(LocalToWorld));
		LocalToWorld[0] = LocalToWorld[5] = LocalToWorld[10] = LocalToWorld[15] = 1.0f;
	}
};

} // namespace Maho

static_assert(sizeof(Maho::FTransformComponent) == 64, "Expected 16 floats = 64 bytes");
