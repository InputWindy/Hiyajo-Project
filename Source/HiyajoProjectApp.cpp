#include <Maho.h>
#include <EntryPoint.h>

#include "Game/GameWorldLayer.h"
#include "Render/Forward/ForwardRendererFeature.h"
#include "Resource/ResourceFactories.h"

#if defined(GAME_WITH_EDITOR) && defined(MAHO_WITH_IMGUI)
#	include "Editor/EditorLayer.h"
#endif

#include <memory>

class FHiyajoProjectApp : public Maho::FApp
{
protected:
	virtual void Configure(Maho::FConfig& OutConfig) override
	{
		OutConfig.ApplicationName = "Hiyajo-Project";
		// Relative dirs — FPaths::Initialize turns them into absolute under Project/Engine roots.
		OutConfig.EngineShadersDir = "Engine/Shaders";
		OutConfig.ProjectShadersDir = "Shaders";
		OutConfig.EnginePluginsDir = "Engine/Plugins";
		OutConfig.ProjectPluginsDir = "Plugins";
		OutConfig.ProjectContentDir = "Content";
		OutConfig.CachedDir = "Cached";
		OutConfig.SavedDir = "Saved";
		OutConfig.ProjectConfigDir = "Config";
		OutConfig.ProjectScriptsDir = "Scripts";
	}

	virtual bool PreInitialize() override
	{
		using Maho::EExtensionPriority;

		RegisterExtension<Maho::FPlatformSystem>(EExtensionPriority::System);
		RegisterExtension<Maho::FRenderSystem>(EExtensionPriority::System);
		RegisterExtension<Maho::FResourceSystem>(EExtensionPriority::System);
		RegisterExtension<Maho::FScriptSystem>(EExtensionPriority::Overlay);
		RegisterExtension<GameWorldLayer>(EExtensionPriority::Layer);

#if defined(GAME_WITH_EDITOR) && defined(MAHO_WITH_IMGUI)
		RegisterExtension<Maho::FEditorLayer>(EExtensionPriority::Overlay);
#endif

		return true;
	}

	virtual bool PostInitialize() override
	{
		auto* RenderSystem = GetExtension<Maho::FRenderSystem>();
		if (RenderSystem)
		{
			RenderSystem->RegisterFeature<Maho::FForwardRendererFeature>();
		}
		Maho::RegisterResourceFactories();
		return true;
	}
};

Maho::FApp* Maho::CreateApplication()
{
	return new FHiyajoProjectApp();
}
