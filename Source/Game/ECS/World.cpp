#include "Game/ECS/World.h"
#include "Game/ECS/EntityCommandBuffer.h"

namespace Maho
{

FWorld::FWorld()
{
}

void FWorld::Tick(float DeltaTime)
{
	// Execute all system groups in registration order.
	// Each group recursively executes its children depth-first.
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group != nullptr)
		{
			Group->OnUpdate(DeltaTime, *this);
		}
	}

	// Process any deferred component operations.
	Manager.EndFrame();
}

FEntityCommandBuffer& FWorld::GetEndSimECB()
{
	FSimulationSystemGroup& SimGroup = GetOrCreateSystemGroup<FSimulationSystemGroup>();
	return SimGroup.GetEndECB();
}

FEntityHandle FWorld::CreatePersistentEntity(const ComponentMaskType& Mask)
{
	FEntityHandle Handle = Manager.CreateEntity(Mask);
	if (Handle.IsValid())
	{
		PersistentEntities.push_back(Handle);
	}
	return Handle;
}

void FWorld::DestroyPersistentEntity(FEntityHandle Handle)
{
	if (!Manager.IsValid(Handle))
	{
		return;
	}

	for (auto It = PersistentEntities.begin(); It != PersistentEntities.end(); ++It)
	{
		if (It->Index == Handle.Index && It->Generation == Handle.Generation)
		{
			Manager.DestroyEntity(Handle);
			PersistentEntities.erase(It);
			return;
		}
	}
}

bool FWorld::IsPersistentEntity(FEntityHandle Handle) const
{
	for (FEntityHandle H : PersistentEntities)
	{
		if (H.Index == Handle.Index && H.Generation == Handle.Generation)
		{
			return true;
		}
	}
	return false;
}

} // namespace Maho
