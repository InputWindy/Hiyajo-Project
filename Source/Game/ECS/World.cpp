#include "ECS/World.h"

#include <cstring>

namespace Maho
{

void FECSWorld::AddSystem(ISystem* InSystem)
{
	if (InSystem != nullptr)
	{
		Systems.push_back(InSystem);
	}
}

void FECSWorld::Tick(float DeltaTime)
{
	for (ISystem* Sys : Systems)
	{
		if (Sys != nullptr)
		{
			Sys->OnUpdate(DeltaTime, *this);
		}
	}
	Manager.EndFrame();
}

void FECSWorld::EndFrame()
{
	Manager.EndFrame();
}

FEntityHandle FECSWorld::CreatePersistentEntity(ComponentMaskType Mask)
{
	FEntityHandle Handle = Manager.CreateEntity(Mask);
	PersistentEntities.push_back(Handle);
	return Handle;
}

void FECSWorld::DestroyPersistentEntity(FEntityHandle Handle)
{
	auto It = std::find(PersistentEntities.begin(), PersistentEntities.end(), Handle);
	if (It != PersistentEntities.end())
	{
		PersistentEntities.erase(It);
		Manager.DestroyEntity(Handle);
	}
}

bool FECSWorld::LoadLevelFromBlob(const std::vector<std::uint8_t>& Blob)
{
	if (Blob.empty()) return false;

	std::size_t Pos = 0;
	auto Read = [&](void* Dst, std::size_t Size) -> bool
	{
		if (Pos + Size > Blob.size()) return false;
		std::memcpy(Dst, Blob.data() + Pos, Size);
		Pos += Size;
		return true;
	};

	std::uint32_t EntityCount = 0;
	if (!Read(&EntityCount, sizeof(EntityCount))) return false;

	for (std::uint32_t I = 0; I < EntityCount; ++I)
	{
		ComponentMaskType Mask;
		{
			std::uint64_t Raw[1] = {0};
			if (!Read(Raw, sizeof(Raw))) return false;
			Mask = ComponentMaskType(*Raw);
		}
		FEntityHandle Handle = Manager.CreateEntity(Mask);
		if (!Handle.IsValid()) return false;
	}

	return true;
}

std::vector<std::uint8_t> FECSWorld::SaveLevelToBlob() const
{
	std::vector<std::uint8_t> Blob;
	return Blob;
}

void FECSWorld::UnloadAllLevels()
{
	// Destroy all entities except persistent ones — stub for now
}

} // namespace Maho
