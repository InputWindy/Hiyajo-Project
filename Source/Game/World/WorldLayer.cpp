#include "Game/World/WorldLayer.h"
#include "Game/World/Components/TransformComponent.h"
#include "Game/World/Systems/MovementSystem.h"
#include "Game/World/Systems/CameraSystem.h"
#include "Game/Components/AllComponents.h"

#include <Core/Application/App.h>
#include <Core/Extension/Render/Render.h>
#include <Core/System/Log.h>
#include <ECS/Query.h>

#include <utility>

FWorldLayer::FWorldLayer(std::string InWorldName)
	: Maho::FLayer("WorldLayer")
	, WorldName(std::move(InWorldName))
{
}

bool FWorldLayer::ExecuteStage(Maho::EEngineStage Stage)
{
	switch (Stage)
	{
	case Maho::EEngineStage::Init:
		break;

	case Maho::EEngineStage::Attach:
		if (!bWorldReady)
		{
			// Set up simulation group with systems
			auto& SimGroup = World.GetOrCreateSystemGroup<Maho::FSimulationSystemGroup>();
			SimGroup.AddSystem<FMovementSystem>();
			SimGroup.AddSystem<FCameraSystem>();

			// Spawn demo entity with a TransformComponent.
			Maho::FEntityHandle Handle = World.CreateEntity();
			Maho::FTransformComponent Transform;
			Transform.SetIdentity();
			World.SetComponent<Maho::FTransformComponent>(Handle, Transform);

			// ─── Persistent camera entity ───
			{
				Maho::ComponentMaskType CamMask = Maho::MakeComponentMask<Maho::FTransformComponent, Maho::FCameraComponent>();
				Maho::FEntityHandle CamHandle = World.CreatePersistentEntity(CamMask);

				Maho::FTransformComponent CamTransform;
				CamTransform.SetIdentity();
				CamTransform.LocalToWorld[14] = -5.0f;
				World.GetEntityManager().SetComponent<Maho::FTransformComponent>(CamHandle, CamTransform);

				Maho::FCameraComponent CamComp;
				CamComp.bMainCamera = true;
				CamComp.FOV = 60.0f;
				CamComp.NearPlane = 0.1f;
				CamComp.FarPlane = 1000.0f;
				CamComp.AspectRatio = 16.0f / 9.0f;
				World.GetEntityManager().SetComponent<Maho::FCameraComponent>(CamHandle, CamComp);
			}

			bWorldReady = true;
			World.TickCreate();
			MAHO_INFO("FWorldLayer: ECS world ready (\"{}\")", WorldName);
		}
		break;

	case Maho::EEngineStage::Detach:
		if (bWorldReady)
		{
			World.TickDestroy();
			bWorldReady = false;
		}
		break;

	case Maho::EEngineStage::BeginFrame:
		if (bWorldReady && Maho::GApp)
		{
			World.TickBeginFrame();
		}
		break;

	case Maho::EEngineStage::FixedUpdate:
		if (bWorldReady && Maho::GApp)
		{
			World.TickFixedUpdate(Maho::GApp->GetFixedDeltaSeconds());
		}
		break;

	case Maho::EEngineStage::Update:
		if (bWorldReady && Maho::GApp)
		{
			World.TickUpdate(Maho::GApp->GetDeltaSeconds());
		}
		break;

	case Maho::EEngineStage::LateUpdate:
		if (bWorldReady && Maho::GApp)
		{
			World.TickLateUpdate(Maho::GApp->GetDeltaSeconds());
		}
		break;

	case Maho::EEngineStage::EndFrame:
		if (bWorldReady && Maho::GApp)
		{
			World.TickEndFrame();
		}
		break;

	case Maho::EEngineStage::PreRender:
		if (bWorldReady && Maho::GApp)
		{
			World.TickPreRender();

			auto* RenderSystem = Maho::GApp->GetExtension<Maho::FRenderSystem>();
			if (RenderSystem)
			{
				Maho::FSceneUpdatePacket Packet;
				{
					auto Query = World.Query<Maho::FTransformComponent>();
					Query.ForEach([&Packet](Maho::FEntityHandle Handle, const Maho::FTransformComponent& Transform)
					{
						Maho::FSceneDrawItem Item;
						Item.Type = Maho::EScenePrimitiveType::ColoredTriangle;
						std::memcpy(Item.LocalToWorld, Transform.LocalToWorld, sizeof(Item.LocalToWorld));
						Packet.Draws.push_back(Item);
					});
				}
				// Send camera data
				const Maho::FCameraComponent* Cam = World.GetPersistentComponent<Maho::FCameraComponent>();
				const Maho::FTransformComponent* CamTrans = World.GetPersistentComponent<Maho::FTransformComponent>();
				if (Cam && CamTrans)
				{
					Maho::FCameraFrameData CamData;
					CamData.FOV = Cam->FOV;
					CamData.NearPlane = Cam->NearPlane;
					CamData.FarPlane = Cam->FarPlane;
					CamData.AspectRatio = Cam->AspectRatio;
					CamData.bOrthographic = Cam->bOrthographic;
					CamData.OrthoSize = Cam->OrthoSize;
					{
						const float* M = CamTrans->LocalToWorld;
						float Rinv[9] = {
							M[0], M[4], M[8],
							M[1], M[5], M[9],
							M[2], M[6], M[10],
						};
						float Tx = M[12], Ty = M[13], Tz = M[14];
						float TinvX = -(Rinv[0]*Tx + Rinv[3]*Ty + Rinv[6]*Tz);
						float TinvY = -(Rinv[1]*Tx + Rinv[4]*Ty + Rinv[7]*Tz);
						float TinvZ = -(Rinv[2]*Tx + Rinv[5]*Ty + Rinv[8]*Tz);
						float ViewM[16] = {
							Rinv[0], Rinv[1], Rinv[2], 0,
							Rinv[3], Rinv[4], Rinv[5], 0,
							Rinv[6], Rinv[7], Rinv[8], 0,
							TinvX,   TinvY,   TinvZ,   1,
						};
						std::memcpy(CamData.View, ViewM, sizeof(CamData.View));
					}
					Packet.Camera = CamData;
				}
				RenderSystem->GetRenderServer().SubmitSceneUpdate(std::move(Packet));
			}
		}
		break;

	case Maho::EEngineStage::PostRender:
		if (bWorldReady && Maho::GApp)
		{
			World.TickPostRender();
		}
		break;

	case Maho::EEngineStage::PrepareExit:
	case Maho::EEngineStage::Shutdown:
		if (bWorldReady)
		{
			World.TickDestroy();
			bWorldReady = false;
		}
		break;

	default:
		break;
	}
	return true;
}
