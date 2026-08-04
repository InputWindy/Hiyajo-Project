#include "ECS/Archetype.h"

#include <algorithm>
#include <memory>
#include <new>

namespace Maho
{

FChunk::FChunk(const ComponentMaskType& InMask, const std::vector<std::size_t>& InSizes, const std::vector<std::size_t>& InOffsets)
	: Mask(InMask)
{
	// Compute row stride: entity row header + all component columns.
	std::size_t TotalStride = sizeof(FEntityRow);
	for (std::size_t S : InSizes)
	{
		TotalStride += S;
	}
	RowStride = TotalStride;
	ColumnSizes = InSizes;
	ColumnOffsets = InOffsets;

	Data = new std::uint8_t[ECSChunkSize * RowStride]();
	Count = 0;
}

FChunk::FChunk(FChunk&& Other) noexcept
	: Mask(Other.Mask)
	, ColumnSizes(std::move(Other.ColumnSizes))
	, ColumnOffsets(std::move(Other.ColumnOffsets))
	, RowStride(Other.RowStride)
	, Count(Other.Count)
	, Data(Other.Data)
{
	Other.Data = nullptr;
	Other.Count = 0;
}

FChunk& FChunk::operator=(FChunk&& Other) noexcept
{
	if (this != &Other)
	{
		delete[] Data;
		Mask = Other.Mask;
		ColumnSizes = std::move(Other.ColumnSizes);
		ColumnOffsets = std::move(Other.ColumnOffsets);
		RowStride = Other.RowStride;
		Count = Other.Count;
		Data = Other.Data;
		Other.Data = nullptr;
		Other.Count = 0;
	}
	return *this;
}

FChunk::~FChunk()
{
	delete[] Data;
}

void* FChunk::GetComponentColumn(std::size_t ColumnIndex)
{
	assert(ColumnIndex < ColumnSizes.size());
	return static_cast<char*>(static_cast<void*>(GetEntityRows() + ECSChunkSize)) + ColumnOffsets[ColumnIndex] * ECSChunkSize;
}

const void* FChunk::GetComponentColumn(std::size_t ColumnIndex) const
{
	assert(ColumnIndex < ColumnSizes.size());
	return static_cast<const char*>(static_cast<const void*>(GetEntityRows() + ECSChunkSize)) + ColumnOffsets[ColumnIndex] * ECSChunkSize;
}

FArchetype::FArchetype(const ComponentMaskType& InMask, const std::vector<std::size_t>& InSizes)
	: Mask(InMask)
{
	// Compute column offsets for SoA layout.
	ComponentSizes = InSizes;
	ComponentOffsets.resize(InSizes.size(), 0);
	std::size_t Offset = 0;
	for (std::size_t Index = 0; Index < InSizes.size(); ++Index)
	{
		ComponentOffsets[Index] = Offset;
		Offset += InSizes[Index];
	}
	EntityRowSize = sizeof(FEntityRow) + Offset;
}

} // namespace Maho
