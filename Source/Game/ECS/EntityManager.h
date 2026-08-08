#pragma once

#include "ECS/Archetype.h"
#include "ECS/ComponentType.h"
#include "ECS/EntityHandle.h"
#include "ECS/Query.h"

#include <cstdint>
#include <functional>
#include <type_traits>
#include <vector>

namespace Maho
{

/**
 * Core entity storage. Entities live in archetypes based on their component
 * mask. Adding/removing components defers to EndFrame to batch-migrate.
 */
class FEntityManager
{
public:
	FEntityManager() = default;
	~FEntityManager();

	FEntityManager(const FEntityManager&) = delete;
	FEntityManager& operator=(const FEntityManager&) = delete;
	FEntityManager(FEntityManager&&) = delete;
	FEntityManager& operator=(FEntityManager&&) = delete;

	/** Create entity with given component mask (no initial component data). */
	[[nodiscard]] FEntityHandle CreateEntity(const ComponentMaskType& Mask);

	/** Destroy entity immediately (row is tombstoned, chunk may be compacted). */
	void DestroyEntity(FEntityHandle Handle);

	/**
	 * Add a component to an existing entity. Value is copy-constructed.
	 * Deferred: entity is marked for migration and moved at EndFrame.
	 */
	template <typename T>
	void AddComponent(FEntityHandle Handle, const T& Value)
	{
		static_assert(std::is_trivially_copyable_v<T>, "ECS component must be trivially copyable");
		PendingAdds.push_back({Handle, MakeComponentMask<T>(), 0, nullptr});
		// Store value for deferred copy.
		std::size_t Slot = PendingAdds.size() - 1;
		void* Data = ::operator new(sizeof(T));
		std::memcpy(Data, &Value, sizeof(T));
		PendingAdds.back().ComponentData = Data;
		PendingAdds.back().ComponentSize = sizeof(T);
	}

	/**
	 * Remove a component from an entity.
	 * Deferred: entity is marked for migration and moved at EndFrame.
	 */
	template <typename T>
	void RemoveComponent(FEntityHandle Handle)
	{
		PendingRemoves.push_back({Handle, MakeComponentMask<T>()});
	}

	/** Get mutable component reference. Asserts if entity lacks the component. */
	template <typename T>
	[[nodiscard]] T* GetComponent(FEntityHandle Handle)
	{
		FEntityLocation Loc = LocateEntity(Handle);
		if (Loc.Chunk == nullptr)
		{
			return nullptr;
		}
		return Loc.Chunk->GetComponent<T>(Loc.Row);
	}

	template <typename T>
	[[nodiscard]] const T* GetComponent(FEntityHandle Handle) const
	{
		FEntityLocation Loc = LocateEntity(Handle);
		if (Loc.Chunk == nullptr)
		{
			return nullptr;
		}
		return Loc.Chunk->GetComponent<T>(Loc.Row);
	}

	[[nodiscard]] bool IsValid(FEntityHandle Handle) const;

	/**
	 * Process all pending add/remove component operations.
	 * Migrates entities between archetypes as needed.
	 */
	void EndFrame();

	/** Collect all chunks matching a component mask.
	 * Used by TComponentQuery<Ts...>::Gather().
	 */
	void GatherMatchingChunks(const ComponentMaskType& Mask, std::vector<FChunk*>& OutChunks) const;

	[[nodiscard]] std::size_t GetEntityCount() const { return EntityCount; }
	[[nodiscard]] std::size_t GetArchetypeCount() const { return Archetypes.size(); }

	/** Iterate all alive entities. Callback receives FEntityHandle. */
	template <typename F>
	void ForEachEntity(F&& Func) const
	{
		for (std::size_t I = 0; I < EntitySlots.size(); ++I)
		{
			const FEntitySlot& Slot = EntitySlots[I];
			if (Slot.bAlive)
			{
				Func(FEntityHandle::Make(static_cast<std::uint32_t>(I), Slot.Generation));
			}
		}
	}

	/** Get the component mask for a given entity. */
	[[nodiscard]] ComponentMaskType GetComponentMask(FEntityHandle Handle) const
	{
		FEntityLocation Loc = LocateEntity(Handle);
		if (Loc.Chunk == nullptr || Loc.ArchetypeIndex >= Archetypes.size())
		{
			return {};
		}
		return Archetypes[Loc.ArchetypeIndex].Archetype->Mask;
	}

private:
	struct FEntityLocation
	{
		FChunk* Chunk = nullptr;
		std::size_t Row = 0;
		std::size_t ArchetypeIndex = 0;
	};

	struct FDeferredOp
	{
		FEntityHandle Handle;
		ComponentMaskType Mask;
		std::size_t ComponentSize = 0;
		void* ComponentData = nullptr;
	};

	FEntityLocation LocateEntity(FEntityHandle Handle) const;
	FArchetype* FindOrCreateArchetype(const ComponentMaskType& Mask, const std::vector<std::size_t>& Sizes);
	FChunk* AllocateChunk(FArchetype& Archetype);
	void MigrateEntity(FEntityHandle Handle, FArchetype& FromArchetype, FArchetype* ToArchetype, const std::vector<char>& NewComponentsData);

	// Entity slot management.
	struct FEntitySlot
	{
		FChunk* Chunk = nullptr;
		std::size_t Row = 0;
		std::size_t ArchetypeIndex = 0;
		std::uint8_t Generation = 0;
		bool bAlive = false;
	};

	FEntitySlot& GetSlot(std::uint32_t Index);
	const FEntitySlot& GetSlot(std::uint32_t Index) const;

	std::vector<FEntitySlot> EntitySlots;
	std::vector<std::uint32_t> FreeIndices;
	std::size_t EntityCount = 0;

	struct FArchetypeEntry
	{
		FArchetype* Archetype = nullptr;
		std::vector<std::size_t> ComponentSizes;
	};

	std::vector<FArchetypeEntry> Archetypes;

	std::vector<FDeferredOp> PendingAdds;
	std::vector<FDeferredOp> PendingRemoves;
};

// Template Gather
template <typename... Ts>
void TComponentQuery<Ts...>::Gather(const FEntityManager& Manager)
{
	ComponentMaskType Mask = MakeComponentMask<Ts...>();
	std::vector<FChunk*> Matching;
	Manager.GatherMatchingChunks(Mask, Matching);
	Views.clear();
	Views.reserve(Matching.size());
	for (FChunk* C : Matching)
	{
		TChunkView<Ts...> View;
		View.Chunk = C;
		Views.push_back(View);
	}
}

} // namespace Maho
