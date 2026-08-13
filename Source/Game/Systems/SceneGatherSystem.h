#pragma once

#include <Core/Extension/World/ECS/System.h>

/**
 * Gathers scene draw items + camera from the ECS world and submits
 * them to FRenderServer as a FSceneUpdatePacket (PreRender stage).
 */
class FSceneGatherSystem final : public Maho::ISystem
{
public:
	[[nodiscard]] const char* GetName() const override { return "SceneGatherSystem"; }
	static const char* StaticName() { return "SceneGatherSystem"; }

	void OnPreRender(Maho::FWorld& World) override;
};
