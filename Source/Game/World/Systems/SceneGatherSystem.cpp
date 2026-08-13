#include "Game/World/Systems/SceneGatherSystem.h"

#include <ECS/World.h>
#include <ECS/Query.h>
#include <Core/Application/App.h>
#include <Core/Extension/Render/Render.h>
#include <Render/RenderServer.h>
#include <Render/SceneUpdatePacket.h>
#include "Game/World/Components/TransformComponent.h"
#include "Game/Components/AllComponents.h"

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
		Query.ForEach([&Packet](Maho::FEntityHandle /*Handle*/, const Maho::FTransformComponent& Transform)
		{
			Maho::FSceneDrawItem Item;
			Item.Type = Maho::EScenePrimitiveType::ColoredTriangle;
			std::memcpy(Item.LocalToWorld, Transform.LocalToWorld, sizeof(Item.LocalToWorld));
			Packet.Draws.push_back(Item);
		});
	}

	// Camera: the entity carrying FMainCameraTag + transform + camera.
	{
		auto Query = World.Query<Maho::FTransformComponent, Maho::FCameraComponent>();
		Query.ForEach([&Packet](Maho::FEntityHandle Handle, const Maho::FTransformComponent& CamTrans, const Maho::FCameraComponent& Cam)
		{
			if (!Cam.bMainCamera)
			{
				return;
			}

			Maho::FCameraFrameData CamData;
			CamData.FOV = Cam.FOV;
			CamData.NearPlane = Cam.NearPlane;
			CamData.FarPlane = Cam.FarPlane;
			CamData.AspectRatio = Cam.AspectRatio;
			CamData.bOrthographic = Cam.bOrthographic;
			CamData.OrthoSize = Cam.OrthoSize;

			const float* M = CamTrans.LocalToWorld;
			float Rinv[9] = {
				M[0], M[4], M[8],
				M[1], M[5], M[9],
				M[2], M[6], M[10],
			};
			float Tx = M[12], Ty = M[13], Tz = M[14];
			float TinvX = -(Rinv[0] * Tx + Rinv[3] * Ty + Rinv[6] * Tz);
			float TinvY = -(Rinv[1] * Tx + Rinv[4] * Ty + Rinv[7] * Tz);
			float TinvZ = -(Rinv[2] * Tx + Rinv[5] * Ty + Rinv[8] * Tz);
			float ViewM[16] = {
				Rinv[0], Rinv[1], Rinv[2], 0,
				Rinv[3], Rinv[4], Rinv[5], 0,
				Rinv[6], Rinv[7], Rinv[8], 0,
				TinvX,   TinvY,   TinvZ,   1,
			};
			std::memcpy(CamData.View, ViewM, sizeof(CamData.View));

			Packet.Camera = CamData;
		});
	}

	RenderSystem->GetRenderServer().SubmitSceneUpdate(std::move(Packet));
}
