#include "Game/World/WorldLayer.h"

#include <Core/Application/App.h>
#include <Core/Extension/Render/Render.h>
#include <Core/System/Log.h>

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
		// Render features are now auto‑registered by code‑gen in PostInitialize().
		break;
	case Maho::EEngineStage::Attach:
		if (!bWorldReady)
		{
			if (!World.Initialize(WorldName))
			{
				MAHO_ERROR("FWorldLayer Attach: FWorld failed");
			}
			else
			{
				bWorldReady = true;
			}
		}
		break;
	case Maho::EEngineStage::Detach:
		if (bWorldReady)
		{
			World.Shutdown();
			bWorldReady = false;
		}
		break;
	case Maho::EEngineStage::Update:
		if (bWorldReady && Maho::GApp)
		{
			World.Tick(Maho::GApp->GetDeltaSeconds());
		}
		break;
	case Maho::EEngineStage::PreRender:
		if (bWorldReady && Maho::GApp)
		{
			auto* RenderSystem = Maho::GApp->GetExtension<Maho::FRenderSystem>();
			if (RenderSystem)
			{
				RenderSystem->GetRenderServer().SubmitSceneUpdate(World.BuildSceneUpdatePacket());
			}
		}
		break;
	default:
		break;
	}
	return true;
}
