#pragma once
#include <ECS/World.h>
#include <Core/Sequencer/EngineExtension.h>
#include <string>
class FWorldLayer final : public Maho::FLayer
{
public:
	explicit FWorldLayer(std::string WorldName = "MainWorld");
	~FWorldLayer() override = default;
	virtual bool ExecuteStage(Maho::EEngineStage Stage) override;
	[[nodiscard]] Maho::FWorld& GetWorld() { return World; }
	[[nodiscard]] const Maho::FWorld& GetWorld() const { return World; }
private:
	std::string WorldName;
	Maho::FWorld World;
	bool bWorldReady = false;
};
