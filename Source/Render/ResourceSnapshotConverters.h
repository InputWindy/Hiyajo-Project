#pragma once

#include <Render/ResourceSnapshots.h>

namespace Maho
{

class UTexture;
class UStaticMesh;
class USkeleton;
class UAnimation;

[[nodiscard]] bool TryBuildTextureCpuSnapshot(const UTexture& Texture, FTextureCpuSnapshot& Out);
[[nodiscard]] bool TryBuildMeshCpuSnapshot(const UStaticMesh& Mesh, FMeshCpuSnapshot& Out);
[[nodiscard]] bool TryBuildSkeletonCpuSnapshot(const USkeleton& Skeleton, FSkeletonCpuSnapshot& Out);
[[nodiscard]] bool TryBuildAnimationCpuSnapshot(const UAnimation& Animation, FAnimationCpuSnapshot& Out);

} // namespace Maho
