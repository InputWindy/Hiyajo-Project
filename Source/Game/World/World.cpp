#include "Game/World/World.h"
#include "Game/World/TriangleActor.h"

#include <Core/System/Log.h>

#include <cstring>
#include <utility>

bool FWorld::Initialize(std::string InName)
{
	if (bInitialized)
	{
		return true;
	}

	Name = InName.empty() ? "MainWorld" : std::move(InName);
	TickCount = 0;
	Actors.clear();

	SpawnActor<FTriangleActor>("HelloTriangle");

	bInitialized = true;
	MAHO_INFO("FWorld initialized (\"{}\", actors={})", Name, Actors.size());
	return true;
}

void FWorld::Tick(float DeltaSeconds)
{
	if (!bInitialized)
	{
		return;
	}
	++TickCount;
	for (auto& Actor : Actors)
	{
		if (Actor)
		{
			Actor->Tick(DeltaSeconds);
		}
	}
}

void FWorld::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}

	Actors.clear();
	MAHO_INFO("FWorld shut down (\"{}\", ticks={})", Name, TickCount);
	Name.clear();
	TickCount = 0;
	bInitialized = false;
}

Maho::FSceneUpdatePacket FWorld::BuildSceneUpdatePacket() const
{
	Maho::FSceneUpdatePacket Packet;
	for (const auto& Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (dynamic_cast<const FTriangleActor*>(Actor.get()) == nullptr)
		{
			continue;
		}

		Maho::FSceneDrawItem Item;
		Item.Type = Maho::EScenePrimitiveType::ColoredTriangle;
		std::memcpy(Item.LocalToWorld, Actor->GetLocalToWorld(), sizeof(Item.LocalToWorld));
		Packet.Draws.push_back(Item);
	}
	return Packet;
}
