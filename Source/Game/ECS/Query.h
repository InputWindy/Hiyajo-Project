#pragma once

#include "ECS/Archetype.h"
#include "ECS/ComponentType.h"
#include "ECS/EntityHandle.h"

#include <cstddef>
#include <vector>

namespace Maho
{

/**
 * ChunkView: iterates over all entity rows within a single chunk that match
 * the component mask. Returns entity handle + component refs.
 */
template <typename... Ts>
struct TChunkView
{
	FChunk* Chunk = nullptr;

	[[nodiscard]] std::size_t Count() const { return Chunk ? Chunk->Count : 0; }

	template <typename F>
	void ForEach(F&& Func)
	{
		if (Chunk == nullptr)
		{
			return;
		}
		FEntityRow* Rows = Chunk->GetEntityRows();
		for (std::size_t Row = 0; Row < Chunk->Count; ++Row)
		{
			Func(Rows[Row].Handle, *Chunk->GetComponent<Ts>(Row)...);
		}
	}
};

/**
 * Query: iterates over all chunks across all matching archetypes.
 * Usage:
 *   for (auto& View : Query.Iterate(Manager))
 *     View.ForEach([](FEntityHandle H, TransformComponent& T, ...) { ... });
 */
template <typename... Ts>
struct TComponentQuery
{
	std::vector<TChunkView<Ts...>> Views;

	void Gather(const class FEntityManager& Manager);

	template <typename F>
	void ForEach(F&& Func)
	{
		for (auto& View : Views)
		{
			View.ForEach(std::forward<F>(Func));
		}
	}
};

} // namespace Maho
