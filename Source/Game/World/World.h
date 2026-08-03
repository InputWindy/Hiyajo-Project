#pragma once

#include "Game/World/Actor.h"

#include <Render/SceneUpdatePacket.h>

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * Project gameplay world. Owns actors; builds FSceneUpdatePacket for render.
 */
class FWorld
{
public:
	FWorld() = default;

	FWorld(const FWorld&) = delete;
	FWorld& operator=(const FWorld&) = delete;

	bool Initialize(std::string InName = "MainWorld");
	void Tick(float DeltaSeconds);
	void Shutdown();

	[[nodiscard]] bool IsInitialized() const { return bInitialized; }
	[[nodiscard]] const std::string& GetName() const { return Name; }
	[[nodiscard]] std::uint64_t GetTickCount() const { return TickCount; }

	template <typename TActor, typename... TArgs>
	TActor& SpawnActor(TArgs&&... Args)
	{
		static_assert(std::is_base_of_v<FActor, TActor>, "TActor must derive from FActor");
		auto Owned = std::make_unique<TActor>(std::forward<TArgs>(Args)...);
		TActor& Ref = *Owned;
		Actors.push_back(std::move(Owned));
		return Ref;
	}

	[[nodiscard]] Maho::FSceneUpdatePacket BuildSceneUpdatePacket() const;

private:
	bool bInitialized = false;
	std::string Name;
	std::uint64_t TickCount = 0;
	std::vector<std::unique_ptr<FActor>> Actors;
};
