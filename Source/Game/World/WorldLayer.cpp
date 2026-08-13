#include "Game/World/WorldLayer.h"
#include "Game/World/Components/TransformComponent.h"
#include "Game/World/Systems/MovementSystem.h"
#include "Game/World/Systems/CameraSystem.h"
#include "Game/World/Systems/SceneGatherSystem.h"
#include "Game/World/Systems/ScriptExecutionSystem.h"
#include "Game/Components/AllComponents.h"

#include <Core/Application/App.h>
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
			// Build the system tree: Initialization → Simulation.
			auto* SimGroup = RootGroup.AddGroup<Maho::FSimulationSystemGroup>();
			SimGroup->AddSystem<FMovementSystem>();
			SimGroup->AddSystem<FCameraSystem>();
			SimGroup->AddSystem<FSceneGatherSystem>();
			SimGroup->AddSystem<FScriptExecutionSystem>();

			// Spawn demo entity with a TransformComponent.
			{
				Maho::FEntityHandle Handle = World.CreateEntity();
				Maho::FTransformComponent Transform;
				Transform.SetIdentity();
				World.SetComponent<Maho::FTransformComponent>(Handle, Transform);
			}

			// Main camera entity (engine-owned, tagged for editor hiding).
			{
				Maho::FEntityHandle CamHandle = World.CreateEntity();
				World.AddTag<Maho::FMainCameraTag>(CamHandle);

				Maho::FTransformComponent CamTransform;
				CamTransform.SetIdentity();
				CamTransform.Position = glm::vec3(0.0f, 0.0f, -5.0f);
				World.SetComponent<Maho::FTransformComponent>(CamHandle, CamTransform);

				Maho::FCameraComponent CamComp;
				CamComp.bMainCamera = true;
				CamComp.FOV = 60.0f;
				CamComp.NearPlane = 0.1f;
				CamComp.FarPlane = 1000.0f;
				CamComp.AspectRatio = 16.0f / 9.0f;
				World.SetComponent<Maho::FCameraComponent>(CamHandle, CamComp);
			}

			RootGroup.OnCreate(World);
			bWorldReady = true;
			MAHO_INFO("FWorldLayer: ECS world ready (\"{}\")", WorldName);
		}
		break;

	case Maho::EEngineStage::Detach:
		if (bWorldReady)
		{
			RootGroup.OnDestroy(World);
			bWorldReady = false;
		}
		break;

	case Maho::EEngineStage::BeginFrame:
		if (bWorldReady)
		{
			RootGroup.OnBeginFrame(World);
		}
		break;

	case Maho::EEngineStage::FixedUpdate:
		if (bWorldReady && Maho::GApp)
		{
			RootGroup.OnFixedUpdate(Maho::GApp->GetFixedDeltaSeconds(), World);
		}
		break;

	case Maho::EEngineStage::Update:
		if (bWorldReady && Maho::GApp)
		{
			RootGroup.OnUpdate(Maho::GApp->GetDeltaSeconds(), World);
			World.GetEntityManager().EndFrame();
		}
		break;

	case Maho::EEngineStage::LateUpdate:
		if (bWorldReady && Maho::GApp)
		{
			RootGroup.OnLateUpdate(Maho::GApp->GetDeltaSeconds(), World);
		}
		break;

	case Maho::EEngineStage::EndFrame:
		if (bWorldReady)
		{
			RootGroup.OnEndFrame(World);
		}
		break;

	case Maho::EEngineStage::PreRender:
		if (bWorldReady)
		{
			RootGroup.OnPreRender(World);
		}
		break;

	case Maho::EEngineStage::PostRender:
		if (bWorldReady)
		{
			RootGroup.OnPostRender(World);
		}
		break;

	case Maho::EEngineStage::PrepareExit:
	case Maho::EEngineStage::Shutdown:
		if (bWorldReady)
		{
			RootGroup.OnDestroy(World);
			bWorldReady = false;
		}
		break;

	default:
		break;
	}
	return true;
}
