#pragma once

#include "ECS/System.h"

#include <functional>
#include <string>
#include <vector>

namespace Maho
{

/**
 * Simple sequential system scheduler.
 * Future: parallel scheduling based on read/write conflict graph.
 */
class FSystemScheduler
{
public:
	using FSystemEntry = std::pair<std::string, ISystem*>;

	/** Groups of systems that can run in parallel (each group = sequential, groups = parallelizable). */
	struct FSystemGroup
	{
		std::vector<ISystem*> Systems;
	};

	/**
	 * Schedule systems into groups based on read/write declarations.
	 * Current: single-group (sequential).
	 */
	std::vector<FSystemGroup> Schedule(const std::vector<ISystem*>& AllSystems);
};

} // namespace Maho
