#include "Game/System/Resource/ResourceSystem.h"
#include <Render/ResourceSnapshots.h>
#include <Render/TextureRenderProxy.h>
#include <Render/MeshRenderProxy.h>
#include <Render/SkeletonRenderProxy.h>
#include <Render/AnimationRenderProxy.h>

namespace Maho
{

// Moved from engine TextureRenderProxy.cpp — needs FTexture internals
bool TryBuildTextureCpuSnapshot(const FTexture& Texture, FTextureCpuSnapshot& Out)
{
	Out.CatalogKey = Texture.GetSourcePath();
	Out.Dimension = Texture.GetDimension();
	Out.PixelFormat = Texture.GetPixelFormat();
	Out.Width = Texture.GetWidth();
	Out.Height = Texture.GetHeight();
	Out.Depth = Texture.GetDepth();
	Out.ArrayLayers = Texture.GetArrayLayers();
	Out.MipCount = Texture.GetMipCount();
	Out.bSRGB = Texture.IsSRGB();
	Out.Pixels = Texture.GetPixels();
	return true;
}

bool TryBuildMeshCpuSnapshot(const FStaticMesh& Mesh, FMeshCpuSnapshot& Out)
{
	Out.CatalogKey = Mesh.GetSourcePath();
	// Mesh CpuSnapshot not yet fully implemented — stub
	return true;
}

bool TryBuildSkeletonCpuSnapshot(const FSkeleton& Skeleton, FSkeletonCpuSnapshot& Out)
{
	Out.CatalogKey = Skeleton.GetSourcePath();
	// Skeleton CpuSnapshot not yet fully implemented — stub
	return true;
}

bool TryBuildAnimationCpuSnapshot(const FAnimation& Animation, FAnimationCpuSnapshot& Out)
{
	Out.CatalogKey = Animation.GetSourcePath();
	// Animation CpuSnapshot not yet fully implemented — stub
	return true;
}

} // namespace Maho