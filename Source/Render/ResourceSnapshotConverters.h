#pragma once

#include <Render/ResourceSnapshots.h>

namespace Maho
{

class FTexture;
class FStaticMesh;
class FSkeleton;
class FAnimation;

[[nodiscard]] bool TryBuildTextureCpuSnapshot(const FTexture& Texture, FTextureCpuSnapshot& Out);
[[nodiscard]] bool TryBuildMeshCpuSnapshot(const FStaticMesh& Mesh, FMeshCpuSnapshot& Out);
[[nodiscard]] bool TryBuildSkeletonCpuSnapshot(const FSkeleton& Skeleton, FSkeletonCpuSnapshot& Out);
[[nodiscard]] bool TryBuildAnimationCpuSnapshot(const FAnimation& Animation, FAnimationCpuSnapshot& Out);

} // namespace Maho
