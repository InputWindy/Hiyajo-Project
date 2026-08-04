#pragma once

#include "ECS/ComponentType.h"
#include "ECS/EntityHandle.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Maho
{

constexpr std::size_t ECSChunkSize = 16 * 1024; // 16 KB

/**
 * Stores entity data for a single archetype (fixed set of component types).
 * SoA layout: each component type gets its own contiguous column.
 *
 * Layout within chunk:
 *   [EntityRow 0..N-1] — FEntityRow array at front
 *   [CompA Row 0..N-1] — Component A column
 *   [CompB Row 0..N-1] — Component B column
 *   ...
 */
struct FEntityRow
{
	FEntityHandle Handle;
};

struct FChunk
{
	static constexpr std::size_t Capacity = ECSChunkSize;

	explicit FChunk(const ComponentMaskType& InMask, const std::vector<std::size_t>& InOffsets, const std::vector<std::size_t>& InSizes);

	FChunk(const FChunk&) = delete;
	FChunk& operator=(const FChunk&) = delete;
	FChunk(FChunk&& Other) noexcept;
	FChunk& operator=(FChunk&& Other) noexcept;

	~FChunk();

	[[nodiscard]] bool IsFull() const { return Count >= Capacity; }
	[[nodiscard]] bool IsEmpty() const { return Count == 0; }

	/**
	 * Returns pointer to raw data for component at the given type-index column.
	 * Use GetComponentTypeId<T>() to get the column index.
	 */
	[[nodiscard]] void* GetComponentColumn(std::size_t ColumnIndex);
	[[nodiscard]] const void* GetComponentColumn(std::size_t ColumnIndex) const;

	/**
	 * Returns pointer to component data for a specific row within this chunk.
	 */
	template <typename T>
	[[nodiscard]] T* GetComponent(std::size_t Row)
	{
		return reinterpret_cast<T*>(static_cast<char*>(GetComponentColumn(GetComponentTypeId<T>())) + Row * sizeof(T));
	}

	template <typename T>
	[[nodiscard]] const T* GetComponent(std::size_t Row) const
	{
		return reinterpret_cast<const T*>(static_cast<const char*>(GetComponentColumn(GetComponentTypeId<T>())) + Row * sizeof(T));
	}

	[[nodiscard]] FEntityRow* GetEntityRows() { return reinterpret_cast<FEntityRow*>(Data); }
	[[nodiscard]] const FEntityRow* GetEntityRows() const { return reinterpret_cast<const FEntityRow*>(Data); }

	ComponentMaskType Mask;
	std::vector<std::size_t> ColumnSizes; // byte sizes per component
	std::vector<std::size_t> ColumnOffsets; // byte offsets from Data
	std::size_t RowStride = 0; // total bytes per entity row (entity + all components)
	std::size_t Count = 0;
	std::uint8_t* Data = nullptr;
};

struct FArchetype
{
	explicit FArchetype(const ComponentMaskType& InMask, const std::vector<std::size_t>& InSizes);

	FArchetype(const FArchetype&) = delete;
	FArchetype& operator=(const FArchetype&) = delete;

	[[nodiscard]] bool MatchesMask(const ComponentMaskType& Other) const { return Mask == Other; }

	ComponentMaskType Mask;
	std::vector<std::size_t> ComponentSizes;
	std::vector<std::size_t> ComponentOffsets;
	std::size_t EntityRowSize = 0;
	std::vector<FChunk*> Chunks;
};

} // namespace Maho
