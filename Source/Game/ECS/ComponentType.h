#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace Maho
{

constexpr std::size_t ECSMaxComponentTypes = 64;
using ComponentMaskType = std::bitset<ECSMaxComponentTypes>;

using FComponentTypeId = std::uint32_t;

namespace Internal
{
	inline FComponentTypeId NextComponentTypeId()
	{
		static FComponentTypeId Counter = 0;
		return Counter++;
	}
}

/**
 * Returns a unique component type id for T.
 * Components must be std::is_trivially_copyable_v<T> (enforced at EntityManager level).
 */
template <typename T>
[[nodiscard]] FComponentTypeId GetComponentTypeId()
{
	static FComponentTypeId Id = Internal::NextComponentTypeId();
	return Id;
}

/**
 * Build a component mask from a variadic list of types.
 */
template <typename... Ts>
[[nodiscard]] ComponentMaskType MakeComponentMask()
{
	ComponentMaskType Mask;
	((Mask.set(GetComponentTypeId<Ts>())), ...);
	return Mask;
}

} // namespace Maho
