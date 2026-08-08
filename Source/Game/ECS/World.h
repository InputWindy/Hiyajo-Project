#pragma once

#include "ECS/EntityManager.h"
#include "ECS/Query.h"
#include "ECS/System.h"
#include "ECS/SystemScheduler.h"

#include <string>
#include <vector>

namespace Maho
{

class UWorld;

/**
 * FECSWorld wraps the entity manager and system list.
 * Game layers call Tick(dt) once per frame.
 */
class FECSWorld
{
public:
	FECSWorld() = default;
	~FECSWorld() = default;

	FECSWorld(const FECSWorld&) = delete;
	FECSWorld& operator=(const FECSWorld&) = delete;

	/** Register a system (must outlive the world or be unregistered). */
	void AddSystem(ISystem* InSystem);

	/** Tick all systems in order (currently sequential; scheduler for parallel). */
	void Tick(float DeltaTime);

	/** Process deferred component add/remove operations. */
	void EndFrame();

	[[nodiscard]] FEntityManager& GetEntityManager() { return Manager; }
	[[nodiscard]] const FEntityManager& GetEntityManager() const { return Manager; }

	// ─── Persistent entities (camera, game manager, etc.) ───

	/** Create a persistent entity. These are NOT part of any ULevel. */
	FEntityHandle CreatePersistentEntity(ComponentMaskType Mask);
	void DestroyPersistentEntity(FEntityHandle Handle);

	[[nodiscard]] const std::vector<FEntityHandle>& GetPersistentEntities() const { return PersistentEntities; }

	[[nodiscard]] bool IsPersistentEntity(FEntityHandle Handle) const
	{
		for (FEntityHandle H : PersistentEntities)
		{
			if (H.Index == Handle.Index && H.Generation == Handle.Generation)
				return true;
		}
		return false;
	}

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

	// ─── Level management ───

	/** Load a level's entities from its blob. */
	bool LoadLevelFromBlob(const std::vector<std::uint8_t>& Blob);
	/** Serialize all non-persistent entities to blob. */
	std::vector<std::uint8_t> SaveLevelToBlob() const;
	/** Remove all non-persistent entities from the world. */
	void UnloadAllLevels();

private:
	FEntityManager Manager;
	std::vector<ISystem*> Systems;
	FSystemScheduler Scheduler;

	std::vector<FEntityHandle> PersistentEntities;
};

} // namespace Maho
