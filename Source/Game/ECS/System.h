#pragma once

#include <string>

namespace Maho
{

class FECSWorld;

/**
 * ECS System base. Override OnUpdate to define per-frame logic.
 */
class ISystem
{
public:
	virtual ~ISystem() = default;

	[[nodiscard]] virtual const char* GetName() const = 0;

	virtual void OnUpdate(float DeltaTime, FECSWorld& World) = 0;
};

/**
 * Declarative helpers for system reads/writes (used by SystemScheduler for
 * dependency sorting — matching based on component type masks).
 */
template <typename... Ts>
struct TReadsComponent
{
	static auto GetMask() { return MakeComponentMask<Ts...>(); }
};

template <typename... Ts>
struct TWritesComponent
{
	static auto GetMask() { return MakeComponentMask<Ts...>(); }
};

} // namespace Maho
