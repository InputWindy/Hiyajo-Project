#pragma once

#include "ECS/System.h"

#include <memory>
#include <string>
#include <vector>

namespace Maho
{

class FEntityCommandBuffer;

/**
 * A group of systems that executes in dependency-sorted order.
 *
 * FSystemGroup itself is an ISystem, enabling deep nesting:
 *
 *   FInitializationSystemGroup
 *     └─ FSimulationSystemGroup
 *          ├─ FMovementSystem
 *          └─ FDeathSystem
 *
 * Each group automatically creates Begin/End ECB systems.
 * Execution is depth-first: parent OnUpdate recursively calls children.
 */
class FSystemGroup : public ISystem
{
public:
	explicit FSystemGroup(const char* InName);
	virtual ~FSystemGroup();

	const char* GetName() const override { return Name.c_str(); }

	virtual void OnUpdate(float DeltaTime, FWorld& World) override;

	/** Add a system to this group (takes ownership via unique_ptr). */
	template <typename T, typename... Args>
	T* AddSystem(Args&&... InArgs)
	{
		static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");
		auto Sys = std::make_unique<T>(std::forward<Args>(InArgs)...);
		T* Ptr = Sys.get();
		OwnedChildren.push_back(std::move(Sys));
		Systems.push_back(Ptr);
		return Ptr;
	}

	/** Add a sub-group to this group (takes ownership). */
	template <typename T, typename... Args>
	T* AddGroup(Args&&... InArgs)
	{
		static_assert(std::is_base_of_v<FSystemGroup, T>, "T must derive from FSystemGroup");
		auto Grp = std::make_unique<T>(std::forward<Args>(InArgs)...);
		T* Ptr = Grp.get();
		OwnedGroups.push_back(std::move(Grp));
		Groups.push_back(Ptr);
		return Ptr;
	}

	/** Declare that A must update before B within this group. */
	template <typename A, typename B>
	void UpdateBefore()
	{
		UpdateBeforeByName(A::StaticName(), B::StaticName());
	}

	/** Declare that A must update after B within this group. */
	template <typename A, typename B>
	void UpdateAfter()
	{
		UpdateBefore<B, A>();
	}

	/** Access the Begin ECB for this group. */
	FEntityCommandBuffer& GetBeginECB() { return *BeginECB; }

	/** Access the End ECB for this group. */
	FEntityCommandBuffer& GetEndECB() { return *EndECB; }

	/** Static name helper: override in subclasses. */
	static const char* StaticName() { return "SystemGroup"; }

protected:
	void UpdateBeforeByName(const char* A, const char* B);

	std::string Name;

	std::vector<ISystem*> Systems;
	std::vector<FSystemGroup*> Groups;

	std::vector<std::unique_ptr<ISystem>> OwnedChildren;
	std::vector<std::unique_ptr<FSystemGroup>> OwnedGroups;

	std::unique_ptr<FEntityCommandBuffer> BeginECB;
	std::unique_ptr<FEntityCommandBuffer> EndECB;
	std::unique_ptr<ISystem> BeginECBSystem;
	std::unique_ptr<ISystem> EndECBSystem;
};

/**
 * Built-in system groups matching Unity DOTS conventions.
 */
class FInitializationSystemGroup : public FSystemGroup
{
public:
	FInitializationSystemGroup() : FSystemGroup("InitializationSystemGroup") {}
	static const char* StaticName() { return "InitializationSystemGroup"; }
};

class FSimulationSystemGroup : public FSystemGroup
{
public:
	FSimulationSystemGroup() : FSystemGroup("SimulationSystemGroup") {}
	static const char* StaticName() { return "SimulationSystemGroup"; }
};

class FPresentationSystemGroup : public FSystemGroup
{
public:
	FPresentationSystemGroup() : FSystemGroup("PresentationSystemGroup") {}
	static const char* StaticName() { return "PresentationSystemGroup"; }
};

} // namespace Maho
