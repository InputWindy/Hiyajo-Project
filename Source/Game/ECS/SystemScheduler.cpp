#include "ECS/SystemScheduler.h"

namespace Maho
{

std::vector<FSystemScheduler::FSystemGroup> FSystemScheduler::Schedule(const std::vector<ISystem*>& AllSystems)
{
	// Current: single sequential group.
	std::vector<FSystemGroup> Groups;
	if (AllSystems.empty())
	{
		return Groups;
	}

	FSystemGroup Group;
	Group.Systems = AllSystems;
	Groups.push_back(std::move(Group));
	return Groups;
}

} // namespace Maho
