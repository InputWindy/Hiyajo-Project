#include "ECS/EntityManager.h"

#include <algorithm>
#include <cassert>

namespace Maho
{

FEntityManager::~FEntityManager()
{
	for (auto& E : Archetypes)
	{
		for (FChunk* C : E.Archetype->Chunks)
		{
			delete C;
		}
		delete E.Archetype;
	}
	Archetypes.clear();

	for (auto& Op : PendingAdds)
	{
		::operator delete(Op.ComponentData);
	}
	PendingAdds.clear();
}

FEntityManager::FEntitySlot& FEntityManager::GetSlot(std::uint32_t Index)
{
	if (Index >= EntitySlots.size())
	{
		EntitySlots.resize(Index + 1);
	}
	return EntitySlots[Index];
}

const FEntityManager::FEntitySlot& FEntityManager::GetSlot(std::uint32_t Index) const
{
	static FEntitySlot DeadSlot;
	if (Index >= EntitySlots.size())
	{
		return DeadSlot;
	}
	return EntitySlots[Index];
}

FEntityHandle FEntityManager::CreateEntity(const ComponentMaskType& Mask)
{
	// Collect component sizes for matching archetype.
	std::vector<std::size_t> Sizes;
	Sizes.reserve(ECSMaxComponentTypes);
	for (std::size_t Bit = 0; Bit < ECSMaxComponentTypes; ++Bit)
	{
		if (Mask.test(Bit))
		{
			Sizes.push_back(4); // Minimum 4-byte alignment placeholder
		}
	}

	FArchetype* Archetype = FindOrCreateArchetype(Mask, Sizes);
	FChunk* Chunk = AllocateChunk(*Archetype);

	std::uint32_t Index;
	if (!FreeIndices.empty())
	{
		Index = FreeIndices.back();
		FreeIndices.pop_back();
	}
	else
	{
		Index = static_cast<std::uint32_t>(EntitySlots.size());
	}

	FEntitySlot& Slot = GetSlot(Index);
	Slot.Chunk = Chunk;
	Slot.Row = Chunk->Count;
	Slot.ArchetypeIndex = 0;
	for (std::size_t I = 0; I < Archetypes.size(); ++I)
	{
		if (Archetypes[I].Archetype == Archetype)
		{
			Slot.ArchetypeIndex = I;
			break;
		}
	}
	Slot.bAlive = true;

	// Init entity row.
	FEntityRow* Rows = Chunk->GetEntityRows();
	Rows[Chunk->Count].Handle = FEntityHandle::Make(Index, Slot.Generation);

	Chunk->Count++;
	EntityCount++;

	return FEntityHandle::Make(Index, Slot.Generation);
}

void FEntityManager::DestroyEntity(FEntityHandle Handle)
{
	if (!Handle.IsValid() || Handle.Index >= EntitySlots.size())
	{
		return;
	}

	FEntitySlot& Slot = GetSlot(Handle.Index);
	if (!Slot.bAlive || Slot.Generation != Handle.Generation)
	{
		return;
	}

	// Move-last entity into this slot (swap+pop compaction within chunk).
	FChunk* Chunk = Slot.Chunk;
	std::size_t LastRow = Chunk->Count - 1;
	if (Slot.Row != LastRow && LastRow < ECSChunkSize)
	{
		FEntityRow* Rows = Chunk->GetEntityRows();

		// Move entity row.
		Rows[Slot.Row] = Rows[LastRow];

		// Update moved entity's slot mapping.
		std::uint32_t MovedIndex = Rows[LastRow].Handle.Index;
		if (MovedIndex < EntitySlots.size())
		{
			EntitySlots[MovedIndex].Row = Slot.Row;
		}

		// Move component data columns.
		for (std::size_t Col = 0; Col < Chunk->ColumnSizes.size(); ++Col)
		{
			char* ColData = static_cast<char*>(Chunk->GetComponentColumn(Col));
			std::memcpy(ColData + Slot.Row * Chunk->ColumnSizes[Col],
			            ColData + LastRow * Chunk->ColumnSizes[Col],
			            Chunk->ColumnSizes[Col]);
		}
	}

	Chunk->Count--;

	Slot.bAlive = false;
	Slot.Generation++;
	Slot.Chunk = nullptr;
	FreeIndices.push_back(Handle.Index);
	EntityCount--;
}

bool FEntityManager::IsValid(FEntityHandle Handle) const
{
	if (!Handle.IsValid() || Handle.Index >= EntitySlots.size())
	{
		return false;
	}
	const FEntitySlot& Slot = GetSlot(Handle.Index);
	return Slot.bAlive && Slot.Generation == Handle.Generation;
}

FEntityManager::FEntityLocation FEntityManager::LocateEntity(FEntityHandle Handle) const
{
	FEntityLocation Loc;
	if (!Handle.IsValid() || Handle.Index >= EntitySlots.size())
	{
		return Loc;
	}
	const FEntitySlot& Slot = GetSlot(Handle.Index);
	if (!Slot.bAlive || Slot.Generation != Handle.Generation)
	{
		return Loc;
	}
	Loc.Chunk = Slot.Chunk;
	Loc.Row = Slot.Row;
	Loc.ArchetypeIndex = Slot.ArchetypeIndex;
	return Loc;
}

FArchetype* FEntityManager::FindOrCreateArchetype(const ComponentMaskType& Mask, const std::vector<std::size_t>& Sizes)
{
	for (auto& E : Archetypes)
	{
		if (E.Archetype->MatchesMask(Mask))
		{
			return E.Archetype;
		}
	}

	auto* A = new FArchetype(Mask, Sizes);
	Archetypes.push_back({A, Sizes});
	return A;
}

FChunk* FEntityManager::AllocateChunk(FArchetype& Archetype)
{
	if (!Archetype.Chunks.empty() && !Archetype.Chunks.back()->IsFull())
	{
		return Archetype.Chunks.back();
	}

	auto* Chunk = new FChunk(Archetype.Mask, Archetype.ComponentSizes, Archetype.ComponentOffsets);
	Archetype.Chunks.push_back(Chunk);
	return Chunk;
}

void FEntityManager::MigrateEntity(FEntityHandle Handle, FArchetype& FromArchetype, FArchetype* ToArchetype, const std::vector<char>& NewComponentsData)
{
	// Simplified migration: destroy old entity and create new one in target archetype.
	// Full implementation would move in-place without handle change.
	FEntityLocation OldLoc = LocateEntity(Handle);
	if (OldLoc.Chunk == nullptr)
	{
		return;
	}

	// Copy existing component data into target archetype.
	std::vector<char> Data(FromArchetype.EntityRowSize);
	char* Src = reinterpret_cast<char*>(OldLoc.Chunk->GetEntityRows()) + OldLoc.Row * FromArchetype.EntityRowSize;
	std::memcpy(Data.data(), Src, FromArchetype.EntityRowSize);

	// Destroy current placement.
	DestroyEntity(Handle);

	// Create in new archetype.
	FEntityHandle NewHandle = CreateEntity(ToArchetype->Mask);
	FEntityLocation NewLoc = LocateEntity(NewHandle);
	if (NewLoc.Chunk == nullptr)
	{
		return;
	}

	// Copy entity row header.
	FEntityRow* OldRows = reinterpret_cast<FEntityRow*>(Data.data());
	FEntityRow* NewRows = NewLoc.Chunk->GetEntityRows();
	NewRows[NewLoc.Row] = OldRows[0];

	// Copy existing component columns into the destination chunk.
	std::size_t CommonSize = std::min(FromArchetype.EntityRowSize, ToArchetype->EntityRowSize);
	if (CommonSize > sizeof(FEntityRow))
	{
		char* Dst = reinterpret_cast<char*>(NewLoc.Chunk->GetEntityRows()) + NewLoc.Row * ToArchetype->EntityRowSize;
		std::memcpy(Dst + sizeof(FEntityRow), Data.data() + sizeof(FEntityRow), CommonSize - sizeof(FEntityRow));
	}

	// Apply newly added component data at the end.
	if (!NewComponentsData.empty())
	{
		char* Dst = reinterpret_cast<char*>(NewLoc.Chunk->GetEntityRows()) + NewLoc.Row * ToArchetype->EntityRowSize;
		std::memcpy(Dst + sizeof(FEntityRow) + FromArchetype.ComponentOffsets.back() + FromArchetype.ComponentSizes.back(),
		            NewComponentsData.data(),
		            NewComponentsData.size());
	}
}

void FEntityManager::GatherMatchingChunks(const ComponentMaskType& Mask, std::vector<FChunk*>& OutChunks) const
{
	for (const auto& E : Archetypes)
	{
		if ((E.Archetype->Mask & Mask) == Mask)
		{
			for (FChunk* C : E.Archetype->Chunks)
			{
				if (!C->IsEmpty())
				{
					OutChunks.push_back(C);
				}
			}
		}
	}
}

void FEntityManager::EndFrame()
{
	// Process deferred additions.
	for (auto& Op : PendingAdds)
	{
		if (!IsValid(Op.Handle))
		{
			continue;
		}

		FEntitySlot& Slot = GetSlot(Op.Handle.Index);
		if (!Slot.bAlive)
		{
			continue;
		}

		FArchetype* Current = Archetypes[Slot.ArchetypeIndex].Archetype;
		ComponentMaskType NewMask = Current->Mask | Op.Mask;

		std::vector<std::size_t> NewSizes = Archetypes[Slot.ArchetypeIndex].ComponentSizes;
		NewSizes.push_back(Op.ComponentSize);

		FArchetype* Target = FindOrCreateArchetype(NewMask, NewSizes);
		std::vector<char> CompData(static_cast<const char*>(Op.ComponentData), static_cast<const char*>(Op.ComponentData) + Op.ComponentSize);
		MigrateEntity(Op.Handle, *Current, Target, CompData);

		::operator delete(Op.ComponentData);
	}
	PendingAdds.clear();

	// Process deferred removals.
	for (auto& Op : PendingRemoves)
	{
		if (!IsValid(Op.Handle))
		{
			continue;
		}

		FEntitySlot& Slot = GetSlot(Op.Handle.Index);
		if (!Slot.bAlive)
		{
			continue;
		}

		FArchetype* Current = Archetypes[Slot.ArchetypeIndex].Archetype;
		ComponentMaskType NewMask = Current->Mask & ~Op.Mask;

		std::vector<std::size_t> NewSizes;
		for (std::size_t Bit = 0; Bit < ECSMaxComponentTypes; ++Bit)
		{
			if (NewMask.test(Bit))
			{
				NewSizes.push_back(4); // Simplified
			}
		}

		FArchetype* Target = FindOrCreateArchetype(NewMask, NewSizes);
		MigrateEntity(Op.Handle, *Current, Target, {});
	}
	PendingRemoves.clear();
}

} // namespace Maho
