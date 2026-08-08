#include "ECS/World.h"
#include "ECS/EntityCommandBuffer.h"

namespace Maho
{

FWorld::FWorld()
{
}

// --- Per-stage Tick ---

void FWorld::TickCreate()
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnCreate(*this); }
	}
}

void FWorld::TickDestroy()
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnDestroy(*this); }
	}
}

void FWorld::TickBeginFrame()
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnBeginFrame(*this); }
	}
}

void FWorld::TickFixedUpdate(float DeltaTime)
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnFixedUpdate(DeltaTime, *this); }
	}
}

void FWorld::TickUpdate(float DeltaTime)
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnUpdate(DeltaTime, *this); }
	}
	Manager.EndFrame();
}

void FWorld::TickLateUpdate(float DeltaTime)
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnLateUpdate(DeltaTime, *this); }
	}
}

void FWorld::TickEndFrame()
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnEndFrame(*this); }
	}
}

void FWorld::TickPreRender()
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnPreRender(*this); }
	}
}

void FWorld::TickPostRender()
{
	for (FSystemGroup* Group : SystemGroups)
	{
		if (Group) { Group->OnPostRender(*this); }
	}
}

// --- ECB access ---

FEntityCommandBuffer& FWorld::GetEndSimECB()
{
	FSimulationSystemGroup& SimGroup = GetOrCreateSystemGroup<FSimulationSystemGroup>();
	return SimGroup.GetEndECB();
}

// --- Persistent entities ---

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
