#include "Game/Systems/SceneGatherSystem.h"

#include <Core/Extension/World/ECS/World.h>
#include <Core/Extension/World/ECS/Query.h>
#include <Core/Application/App.h>
#include <Core/Extension/Render/Render.h>
#include <Render/RenderServer.h>
#include <Render/SceneUpdatePacket.h>
#include <Core/Extension/World/Components/TransformComponent.h>
#include "Game/Components/AllComponents.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

void FSceneGatherSystem::OnPreRender(Maho::FWorld& World)
{
	auto* RenderSystem = Maho::GApp ? Maho::GApp->GetExtension<Maho::FRenderSystem>() : nullptr;
	if (!RenderSystem)
	{
		return;
	}

	Maho::FSceneUpdatePacket Packet;

	// Draw items: every entity with a transform.
	{
		auto Query = World.Query<Maho::FTransformComponent>();
		Query.ForEach([&Packet](Maho::FEntityHandle /*Handle*/, Maho::FTransformComponent& Transform)
		{
			Transform.ComputeLocalToWorld();

			Maho::FSceneDrawItem Item;
			Item.Type = Maho::EScenePrimitiveType::ColoredTriangle;
			std::memcpy(Item.LocalToWorld, glm::value_ptr(Transform.LocalToWorld), sizeof(Item.LocalToWorld));
			Packet.Draws.push_back(Item);
		});
	}

	// Camera: main camera entity (transform + camera, bMainCamera flag).
	{
		auto Query = World.Query<Maho::FTransformComponent, Maho::FCameraComponent>();
		Query.ForEach([&Packet](Maho::FEntityHandle /*Handle*/, Maho::FTransformComponent& CamTrans, const Maho::FCameraComponent& Cam)
		{
			if (!Cam.bMainCamera)
			{
				return;
			}

			CamTrans.ComputeLocalToWorld();

			Maho::FCameraFrameData CamData;
			CamData.FOV = Cam.FOV;
			CamData.NearPlane = Cam.NearPlane;
			CamData.FarPlane = Cam.FarPlane;
			CamData.AspectRatio = Cam.AspectRatio;
			CamData.bOrthographic = Cam.bOrthographic;
			CamData.OrthoSize = Cam.OrthoSize;

			const glm::mat4 View = glm::inverse(CamTrans.LocalToWorld);
			std::memcpy(CamData.View, glm::value_ptr(View), sizeof(CamData.View));

			Packet.Camera = CamData;
		});
	}

	RenderSystem->GetRenderServer().SubmitSceneUpdate(std::move(Packet));
}
