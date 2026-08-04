#include "Game/World/WorldLayer.h"
#include "Game/World/Components/TransformComponent.h"
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
			ECSWorld.AddSystem(&MovementSystem);
			ECSWorld.AddSystem(&CameraSystem);

			auto& Manager = ECSWorld.GetEntityManager();

			// Spawn demo entity with a TransformComponent.
			Maho::ComponentMaskType Mask = Maho::MakeComponentMask<Maho::FTransformComponent>();
			Maho::FEntityHandle Handle = Manager.CreateEntity(Mask);
			Maho::FTransformComponent* Transform = Manager.GetComponent<Maho::FTransformComponent>(Handle);
			if (Transform != nullptr)
			{
				Transform->SetIdentity();
			}

			// ─── Persistent camera entity ───
			{
				Maho::ComponentMaskType CamMask = Maho::MakeComponentMask<Maho::FTransformComponent, Maho::FCameraComponent>();
				Maho::FEntityHandle CamHandle = ECSWorld.CreatePersistentEntity(CamMask);

				Maho::FTransformComponent* CamTransform = Manager.GetComponent<Maho::FTransformComponent>(CamHandle);
				if (CamTransform)
				{
					CamTransform->SetIdentity();
					// Move camera slightly back
					CamTransform->LocalToWorld[14] = -5.0f;
				}

				Maho::FCameraComponent* CamComp = Manager.GetComponent<Maho::FCameraComponent>(CamHandle);
				if (CamComp)
				{
					CamComp->bMainCamera = true;
					CamComp->FOV = 60.0f;
					CamComp->NearPlane = 0.1f;
					CamComp->FarPlane = 1000.0f;
					CamComp->AspectRatio = 16.0f / 9.0f;
				}
			}

			bWorldReady = true;
			MAHO_INFO("FWorldLayer: ECS world ready (\"{}\")", WorldName);
		}
		break;
	case Maho::EEngineStage::Detach:
		if (bWorldReady)
		{
			bWorldReady = false;
		}
		break;
	case Maho::EEngineStage::Update:
		if (bWorldReady && Maho::GApp)
		{
			ECSWorld.Tick(Maho::GApp->GetDeltaSeconds());
		}
		break;
	case Maho::EEngineStage::PreRender:
		if (bWorldReady && Maho::GApp)
		{
			auto* RenderSystem = Maho::GApp->GetExtension<Maho::FRenderSystem>();
			if (RenderSystem)
			{
				Maho::FSceneUpdatePacket Packet;
				{
				Maho::TComponentQuery<Maho::FTransformComponent> Query;
				Query.Gather(ECSWorld.GetEntityManager());
				Query.ForEach([&Packet](Maho::FEntityHandle Handle, const Maho::FTransformComponent& Transform)
					{
						Maho::FSceneDrawItem Item;
						Item.Type = Maho::EScenePrimitiveType::ColoredTriangle;
						std::memcpy(Item.LocalToWorld, Transform.LocalToWorld, sizeof(Item.LocalToWorld));
						Packet.Draws.push_back(Item);
					});
				}
				// Send camera data
				const Maho::FCameraComponent* Cam = ECSWorld.GetPersistentComponent<Maho::FCameraComponent>();
				const Maho::FTransformComponent* CamTrans = ECSWorld.GetPersistentComponent<Maho::FTransformComponent>();
				if (Cam && CamTrans)
				{
					Maho::FCameraFrameData CamData;
					CamData.FOV = Cam->FOV;
					CamData.NearPlane = Cam->NearPlane;
					CamData.FarPlane = Cam->FarPlane;
					CamData.AspectRatio = Cam->AspectRatio;
					CamData.bOrthographic = Cam->bOrthographic;
					CamData.OrthoSize = Cam->OrthoSize;
					// View = inverse of camera LocalToWorld (rigid‑body inverse)
					{
						const float* M = CamTrans->LocalToWorld;   // column‑major
						// Upper‑left 3x3 = rotation → transpose for inverse
						float Rinv[9] = {
							M[0], M[4], M[8],
							M[1], M[5], M[9],
							M[2], M[6], M[10],
						};
						float Tx = M[12], Ty = M[13], Tz = M[14];
						// T_inv = -R^T * T
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
	default:
		break;
	}
	return true;
}
