#include "Game/System/Resource/ResourceSystem.h"

namespace Maho
{

// ── FResource ──────────────────────────────────────────────────

FResource::FResource(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: Name(std::move(InName))
	, Type(InType)
	, SourcePath(std::move(InSourcePath))
	, LoadState(EAssetLoadState::Pending)
{
}

// ── FTexture ───────────────────────────────────────────────────

FTexture::FTexture(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FResource(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Texture2D : InType,
		std::move(InSourcePath))
{
}

void FTexture::SetCpuImage(
	ETextureDimension InDimension,
	ETexturePixelFormat InFormat,
	std::uint32_t InWidth,
	std::uint32_t InHeight,
	std::uint32_t InDepth,
	std::uint32_t InArrayLayers,
	std::uint32_t InMipCount,
	bool bInSRGB,
	std::vector<std::uint8_t> InPixels)
{
	Dimension = InDimension;
	PixelFormat = InFormat;
	Width = InWidth;
	Height = InHeight;
	Depth = InDepth == 0 ? 1 : InDepth;
	ArrayLayers = InArrayLayers == 0 ? 1 : InArrayLayers;
	MipCount = InMipCount == 0 ? 1 : InMipCount;
	bSRGB = bInSRGB;
	Pixels = std::move(InPixels);
	++ContentGeneration;
}

void FTexture::SetSerializedSource(std::string Hint, std::vector<std::uint8_t> Bytes)
{
	SerializedSourceHint = std::move(Hint);
	SerializedSourceBytes = std::move(Bytes);
}

void FTexture::ClearSerializedSource()
{
	SerializedSourceHint.clear();
	SerializedSourceBytes.clear();
}

// ── FTexture2D ─────────────────────────────────────────────────

FTexture2D::FTexture2D(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FTexture(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Texture2D : InType,
		std::move(InSourcePath))
{
	Dimension = ETextureDimension::Tex2D;
}

// ── FTexture3D ─────────────────────────────────────────────────

FTexture3D::FTexture3D(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FTexture(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Texture3D : InType,
		std::move(InSourcePath))
{
	Dimension = ETextureDimension::Tex3D;
}

// ── FTextureCube ───────────────────────────────────────────────

FTextureCube::FTextureCube(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FTexture(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::TextureCube : InType,
		std::move(InSourcePath))
{
	Dimension = ETextureDimension::Cube;
	ArrayLayers = 6;
}

// ── FTextureCubeArray ──────────────────────────────────────────

FTextureCubeArray::FTextureCubeArray(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FTexture(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::TextureCubeArray : InType,
		std::move(InSourcePath))
{
	Dimension = ETextureDimension::CubeArray;
}

// ── FTexture2DArray ────────────────────────────────────────────

FTexture2DArray::FTexture2DArray(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FTexture(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Texture2DArray : InType,
		std::move(InSourcePath))
{
	Dimension = ETextureDimension::Tex2DArray;
}

// ── FMaterial ──────────────────────────────────────────────────

FMaterial::FMaterial(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FResource(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Material : InType,
		std::move(InSourcePath))
{
}

std::vector<std::string> FMaterial::GetReferencePaths() const
{
	std::vector<std::string> Refs;
	if (!BaseColorPath.empty()) Refs.push_back(BaseColorPath);
	if (!NormalPath.empty()) Refs.push_back(NormalPath);
	if (!MetallicRoughnessPath.empty()) Refs.push_back(MetallicRoughnessPath);
	if (!OcclusionPath.empty()) Refs.push_back(OcclusionPath);
	if (!EmissivePath.empty()) Refs.push_back(EmissivePath);
	return Refs;
}

// ── FStaticMesh ────────────────────────────────────────────────

FStaticMesh::FStaticMesh(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FResource(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Mesh : InType,
		std::move(InSourcePath))
{
}

void FStaticMesh::SetCpuGeometry(
	std::vector<float> InPositions,
	std::vector<float> InNormals,
	std::vector<float> InUVs,
	std::vector<std::uint32_t> InIndices)
{
	Positions = std::move(InPositions);
	Normals = std::move(InNormals);
	UVs = std::move(InUVs);
	Indices = std::move(InIndices);
	++ContentGeneration;
}

std::vector<std::string> FStaticMesh::GetReferencePaths() const
{
	std::vector<std::string> Refs;
	if (!MaterialPath.empty()) Refs.push_back(MaterialPath);
	return Refs;
}

// ── FSkeleton ──────────────────────────────────────────────────

FSkeleton::FSkeleton(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FResource(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Skeleton : InType,
		std::move(InSourcePath))
{
}

void FSkeleton::SetBones(std::vector<FSkeletonBone> InBones)
{
	Bones = std::move(InBones);
	++ContentGeneration;
}

// ── FAnimation ─────────────────────────────────────────────────

FAnimation::FAnimation(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FResource(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Animation : InType,
		std::move(InSourcePath))
{
}

void FAnimation::SetSkeleton(std::string Path)
{
	SkeletonPath = std::move(Path);
	++ContentGeneration;
}

void FAnimation::SetDurationSeconds(float Seconds)
{
	DurationSeconds = Seconds;
	++ContentGeneration;
}

void FAnimation::SetTracks(std::vector<FAnimationTrack> InTracks)
{
	Tracks = std::move(InTracks);
	++ContentGeneration;
}

std::vector<std::string> FAnimation::GetReferencePaths() const
{
	std::vector<std::string> Refs;
	if (!SkeletonPath.empty()) Refs.push_back(SkeletonPath);
	return Refs;
}

// ── FAnimationGraph ────────────────────────────────────────────

FAnimationGraph::FAnimationGraph(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FResource(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::AnimationGraph : InType,
		std::move(InSourcePath))
{
}

// ── FPrefab ────────────────────────────────────────────────────

FPrefab::FPrefab(
	std::string InName,
	EAssetType InType,
	std::string InSourcePath)
	: FResource(
		std::move(InName),
		InType == EAssetType::Unknown ? EAssetType::Prefab : InType,
		std::move(InSourcePath))
{
}

} // namespace Maho
