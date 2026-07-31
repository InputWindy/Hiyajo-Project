#pragma once

#include <Core/Sequencer/EngineExtension.h>

#include <string>

/**
 * Default game Layer created with the project.
 * Hand-written — do not put RegisterExtension lists here (those live in Source/Generated/*App.cpp).
 */
class FHiyajoProjectLayer final : public Maho::FLayer
{
public:
	explicit FHiyajoProjectLayer(std::string InName = "HiyajoProject")
		: Maho::FLayer(std::move(InName))
	{
	}

	bool ExecuteStage(Maho::EEngineStage Stage) override;
};
