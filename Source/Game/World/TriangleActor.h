#pragma once

#include "Game/World/Actor.h"

/**
 * Demo actor: a colored triangle drawn by TriangleBasePassFeature.
 */
class FTriangleActor final : public FActor
{
public:
	explicit FTriangleActor(std::string InName = "Triangle")
		: FActor(std::move(InName))
	{
	}
};
