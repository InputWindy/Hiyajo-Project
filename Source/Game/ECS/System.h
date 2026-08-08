#pragma once

#include "ECS/ComponentType.h"

#include <string>

namespace Maho
{

class FWorld;

/**
 * ECS System base. Override OnUpdate for per-frame logic.
 *
 * Systems live inside FSystemGroup, which is itself an ISystem.
 * This enables nested group trees with depth-first execution.
 */
class ISystem
{
public:
	virtual ~ISystem() = default;

	[[nodiscard]] virtual const char* GetName() const = 0;

	virtual void OnUpdate(float DeltaTime, FWorld& World) = 0;
};

/**
 * Declarative helpers for system reads/writes.
 * Used by SystemGroup to derive component masks for automatic ordering.
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
