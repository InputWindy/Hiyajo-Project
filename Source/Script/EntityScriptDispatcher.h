#pragma once

#include <Core/Extension/Script/ScriptSystem.h>
#include <Core/Extension/World/ECS/EntityHandle.h>

#include "Game/Components/TransformComponent.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace Maho
{

/**
 * Project-side entity-script dispatcher.
 * Caches one script prototype per path and one Lua instance table per entity,
 * then mounts the entity's FTransformComponent as a typed sol2 usertype pointer.
 */
class FEntityScriptDispatcher
{
public:
	explicit FEntityScriptDispatcher(FScriptSystem& Script);
	~FEntityScriptDispatcher();

	void Dispatch(FEntityHandle Handle, const char* ScriptPath, FTransformComponent* Transform, float DeltaTime, const char* HookName);
	void Destroy(FEntityHandle Handle);

private:
	struct FImpl;
	std::unique_ptr<FImpl> Impl;
};

} // namespace Maho
