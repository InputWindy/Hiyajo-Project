#include "Game/World/WorldLayer.h"
#include "Game/World/Components/TransformComponent.h"
#include "Game/World/Systems/MovementSystem.h"
#include "Game/World/Systems/CameraSystem.h"
#include "Game/World/Systems/SceneGatherSystem.h"
#include "Game/Components/AllComponents.h"
#include "Game/System/Script/ScriptSystem.h"

#include <Core/Application/App.h>
#include <Core/System/Log.h>
#include <ECS/Query.h>

#include <glm/glm.hpp>

#include <utility>

namespace
{

/** EEngineStage → per-entity script hook name (nullptr = no hook for this stage). */
[[nodiscard]] const char* GetScriptHookForStage(Maho::EEngineStage Stage)
{
	switch (Stage)
	{
	case Maho::EEngineStage::BeginFrame: return "OnBeginFrame";
	case Maho::EEngineStage::ProcessInput: return "OnProcessInput";
	case Maho::EEngineStage::FixedUpdate: return "OnFixedUpdate";
	case Maho::EEngineStage::Update: return "OnUpdate";
	case Maho::EEngineStage::LateUpdate: return "OnLateUpdate";
	case Maho::EEngineStage::EndFrame: return "OnEndFrame";
	case Maho::EEngineStage::PreRender: return "OnPreRender";
	case Maho::EEngineStage::PostRender: return "OnPostRender";
	default: return nullptr;
	}
}

} // namespace

FWorldLayer::FWorldLayer(std::string InWorldName)
	: Maho::FLayer("WorldLayer")
	, WorldName(std::move(InWorldName))
{
}

void FWorldLayer::DispatchScriptStage(Maho::EEngineStage Stage, float DeltaTime)
{
	const char* Hook = GetScriptHookForStage(Stage);
	if (!Hook)
	{
		return;
	}

	Maho::FScriptSystem* Script = Maho::GApp ? Maho::GApp->GetExtension<Maho::FScriptSystem>() : nullptr;
	if (!Script || !Script->IsLuaInitialized())
	{
		return;
	}

	auto Query = World.Query<Maho::FScriptComponent>();
	Query.ForEach([&](Maho::FEntityHandle Handle, Maho::FScriptComponent& Component)
	{
		if (!Component.bEnabled || !Component.IsValid())
		{
			return;
		}

		Maho::FTransformComponent* Transform =
			World.GetEntityManager().GetComponent<Maho::FTransformComponent>(Handle);

		Script->DispatchEntityScript(Handle, Component.ScriptPath, Transform, DeltaTime, Hook);
	});
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
			DispatchScriptStage(Stage, 0.0f);
		}
		break;

	case Maho::EEngineStage::ProcessInput:
		if (bWorldReady)
		{
			DispatchScriptStage(Stage, 0.0f);
		}
		break;

	case Maho::EEngineStage::FixedUpdate:
		if (bWorldReady && Maho::GApp)
		{
			const float FixedDt = Maho::GApp->GetFixedDeltaSeconds();
			RootGroup.OnFixedUpdate(FixedDt, World);
			DispatchScriptStage(Stage, FixedDt);
		}
		break;

	case Maho::EEngineStage::Update:
		if (bWorldReady && Maho::GApp)
		{
			const float Dt = Maho::GApp->GetDeltaSeconds();
			RootGroup.OnUpdate(Dt, World);
			DispatchScriptStage(Stage, Dt);
			World.GetEntityManager().EndFrame();
		}
		break;

	case Maho::EEngineStage::LateUpdate:
		if (bWorldReady && Maho::GApp)
		{
			const float Dt = Maho::GApp->GetDeltaSeconds();
			RootGroup.OnLateUpdate(Dt, World);
			DispatchScriptStage(Stage, Dt);
		}
		break;

	case Maho::EEngineStage::EndFrame:
		if (bWorldReady)
		{
			RootGroup.OnEndFrame(World);
			DispatchScriptStage(Stage, 0.0f);
		}
		break;

	case Maho::EEngineStage::PreRender:
		if (bWorldReady)
		{
			RootGroup.OnPreRender(World);
			DispatchScriptStage(Stage, 0.0f);
		}
		break;

	case Maho::EEngineStage::PostRender:
		if (bWorldReady)
		{
			RootGroup.OnPostRender(World);
			DispatchScriptStage(Stage, 0.0f);
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
