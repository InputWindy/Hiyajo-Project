#include "Game/ECS/SystemGroup.h"
#include "Game/ECS/EntityCommandBuffer.h"
#include "Game/ECS/World.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace Maho
{

// ─── FECBSystem ─────────────────────────────────────────────────

FECBSystem::FECBSystem(FEntityCommandBuffer& InECB, const char* InName)
	: ECB(InECB)
	, Name(InName)
{
}

void FECBSystem::OnUpdate(float DeltaTime, FWorld& World)
{
	ECB.Playback(World.GetEntityManager());
}

// ─── FSystemGroup ────────────────────────────────────────────────

FSystemGroup::FSystemGroup(const char* InName)
	: Name(InName)
{
	BeginECB = std::make_unique<FEntityCommandBuffer>();
	EndECB = std::make_unique<FEntityCommandBuffer>();

	std::string BeginName = std::string("Begin_") + InName;
	std::string EndName = std::string("End_") + InName;

	BeginECBSystem = std::make_unique<FECBSystem>(*BeginECB, BeginName.c_str());
	EndECBSystem = std::make_unique<FECBSystem>(*EndECB, EndName.c_str());
}

FSystemGroup::~FSystemGroup() = default;

void FSystemGroup::OnUpdate(float DeltaTime, FWorld& World)
{
	// 1. Playback Begin ECB
	if (BeginECBSystem)
	{
		BeginECBSystem->OnUpdate(DeltaTime, World);
	}

	// 2. Execute systems in registration order (depth-first for sub-groups)
	for (std::size_t I = 0; I < Groups.size(); ++I)
	{
		if (Groups[I] != nullptr)
		{
			Groups[I]->OnUpdate(DeltaTime, World);
		}
	}

	for (std::size_t I = 0; I < Systems.size(); ++I)
	{
		if (Systems[I] != nullptr)
		{
			Systems[I]->OnUpdate(DeltaTime, World);
		}
	}

	// 3. Playback End ECB
	if (EndECBSystem)
	{
		EndECBSystem->OnUpdate(DeltaTime, World);
	}
}

void FSystemGroup::UpdateBeforeByName(const char* A, const char* B)
{
	// For now, use a simple topological sort based on registration order.
	// If A should be before B, and A appears after B in the list, swap positions.

	std::size_t IndexA = static_cast<std::size_t>(-1);
	std::size_t IndexB = static_cast<std::size_t>(-1);

	for (std::size_t I = 0; I < Systems.size(); ++I)
	{
		if (std::string(Systems[I]->GetName()) == A)
		{
			IndexA = I;
		}
		if (std::string(Systems[I]->GetName()) == B)
		{
			IndexB = I;
		}
	}

	if (IndexA != static_cast<std::size_t>(-1) && IndexB != static_cast<std::size_t>(-1) && IndexA > IndexB)
	{
		std::swap(Systems[IndexA], Systems[IndexB]);
	}
}

} // namespace Maho
