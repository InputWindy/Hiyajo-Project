#include "Game/World/Systems/ScriptExecutionSystem.h"

#include <ECS/World.h>
#include <ECS/Query.h>
#include <Core/Application/App.h>
#include "Game/System/Script/ScriptSystem.h"
#include "Game/World/Components/TransformComponent.h"
#include "Game/Components/AllComponents.h"

void FScriptExecutionSystem::OnUpdate(float DeltaTime, Maho::FWorld& World)
{
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

		Script->TickEntityScript(Handle, Component.ScriptPath, Transform, DeltaTime);
	});
}
