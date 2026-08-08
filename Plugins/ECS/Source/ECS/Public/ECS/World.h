#pragma once

#include "ECS/ECSApi.h"
#include "ECS/EntityManager.h"
#include "ECS/Query.h"
#include "ECS/SystemGroup.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Maho
{

/**
 * FWorld is the top-level ECS container.
 *
 * Owns:
 *   - FEntityManager (entity storage)
 *   - Root FSystemGroup tree (ticking infrastructure)
 *   - ECB systems for Begin/End synchronization points
 *
 * Usage:
 *   FWorld World;
 *   auto& SimGroup = World.GetOrCreateSystemGroup<FSimulationSystemGroup>();
 *   SimGroup.AddSystem<FMySystem>();
 *   // Each frame:
 *   World.Tick(DeltaTime);
 */
class MAHO_ECS_API FWorld
{
public:
	FWorld();
	virtual ~FWorld() = default;

	FWorld(const FWorld&) = delete;
	FWorld& operator=(const FWorld&) = delete;
	FWorld(FWorld&&) = delete;
	FWorld& operator=(FWorld&&) = delete;

	// ─── System groups ────────────────────────────────────────────

	/** Get or create a system group by type. Created on first call. */
	template <typename T>
	T& GetOrCreateSystemGroup()
	{
		static_assert(std::is_base_of_v<FSystemGroup, T>, "T must derive from FSystemGroup");
		for (auto* G : SystemGroups)
		{
			T* Casted = dynamic_cast<T*>(G);
			if (Casted)
			{
				return *Casted;
			}
		}
		auto Grp = new T();
		SystemGroups.push_back(Grp);
		return *Grp;
	}

	// ─── Tick ─────────────────────────────────────────────────────

	/** Called once after systems are registered (Attach). */
	void TickCreate();

	/** Called once before systems are destroyed (Detach / Shutdown). */
	void TickDestroy();

	/** Called at the start of every frame. */
	void TickBeginFrame();

	/** Fixed-timestep update (0..N times per frame). */
	void TickFixedUpdate(float DeltaTime);

	/** Main per-frame update. */
	void TickUpdate(float DeltaTime);

	/** Called after Update. */
	void TickLateUpdate(float DeltaTime);

	/** Called at the end of the frame. */
	void TickEndFrame();

	/** Called before render submission. */
	void TickPreRender();

	/** Called after rendering completes. */
	void TickPostRender();

	// ─── EntityManager access ─────────────────────────────────────

	[[nodiscard]] FEntityManager& GetEntityManager() { return Manager; }
	[[nodiscard]] const FEntityManager& GetEntityManager() const { return Manager; }

	// ─── Convenience: direct entity manipulation ──────────────────

	FEntityHandle CreateEntity()
	{
		return Manager.CreateEntity();
	}

	template <typename T>
	void SetComponent(FEntityHandle Handle, const T& Value)
	{
		Manager.SetComponent<T>(Handle, Value);
	}

	template <typename T>
	void AddTag(FEntityHandle Handle)
	{
		Manager.AddTag<T>(Handle);
	}

	template <typename T>
	void RemoveTag(FEntityHandle Handle)
	{
		Manager.RemoveTag<T>(Handle);
	}

	// ─── Convenience: query builder ────────────────────────────────

	template <typename... Ts>
	TComponentQuery<Ts...> Query()
	{
		TComponentQuery<Ts...> Q;
		Q.Gather(Manager);
		return Q;
	}

	// ─── ECB access ────────────────────────────────────────────────

	/** Convenience: get the End ECB of the simulation group. */
	FEntityCommandBuffer& GetEndSimECB();

	// ─── Persistent entities ──────────────────────────────────────

	FEntityHandle CreatePersistentEntity(const ComponentMaskType& Mask);
	void DestroyPersistentEntity(FEntityHandle Handle);

	[[nodiscard]] const std::vector<FEntityHandle>& GetPersistentEntities() const { return PersistentEntities; }

	[[nodiscard]] bool IsPersistentEntity(FEntityHandle Handle) const;

	template <typename T>
	T* GetPersistentComponent()
	{
		for (FEntityHandle Handle : PersistentEntities)
		{
			T* Comp = Manager.GetComponent<T>(Handle);
			if (Comp) return Comp;
		}
		return nullptr;
	}

	template <typename T>
	const T* GetPersistentComponent() const
	{
		for (FEntityHandle Handle : PersistentEntities)
		{
			const T* Comp = Manager.GetComponent<T>(Handle);
			if (Comp) return Comp;
		}
		return nullptr;
	}

private:
	FEntityManager Manager;
	std::vector<FSystemGroup*> SystemGroups;
	std::vector<FEntityHandle> PersistentEntities;
};

} // namespace Maho
