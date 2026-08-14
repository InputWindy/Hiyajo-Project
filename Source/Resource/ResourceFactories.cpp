#include "Resource/ResourceFactories.h"

#include "Resource/ResourceCasset.h"
#include "Resource/ResourceTypes.h"

#include <Core/Extension/Resource/ResourceSystem.h>

namespace Maho
{

void RegisterResourceFactories()
{
	FResourceSystem* R = Detail::GetResourceSystem();
	if (!R)
		return;

	RegisterCassetPackageCodec(*R);

	R->RegisterResourceFactory(EAssetType::Texture2D, [](const std::string& N, EAssetType T, const std::string& S){ return new FTexture2D(N, T, S); });
	R->RegisterResourceFactory(EAssetType::Texture3D, [](const std::string& N, EAssetType T, const std::string& S){ return new FTexture3D(N, T, S); });
	R->RegisterResourceFactory(EAssetType::TextureCube, [](const std::string& N, EAssetType T, const std::string& S){ return new FTextureCube(N, T, S); });
	R->RegisterResourceFactory(EAssetType::TextureCubeArray, [](const std::string& N, EAssetType T, const std::string& S){ return new FTextureCubeArray(N, T, S); });
	R->RegisterResourceFactory(EAssetType::Texture2DArray, [](const std::string& N, EAssetType T, const std::string& S){ return new FTexture2DArray(N, T, S); });
	R->RegisterResourceFactory(EAssetType::Mesh, [](const std::string& N, EAssetType T, const std::string& S){ return new FStaticMesh(N, T, S); });
	R->RegisterResourceFactory(EAssetType::Material, [](const std::string& N, EAssetType T, const std::string& S){ return new FMaterial(N, T, S); });
	R->RegisterResourceFactory(EAssetType::Skeleton, [](const std::string& N, EAssetType T, const std::string& S){ return new FSkeleton(N, T, S); });
	R->RegisterResourceFactory(EAssetType::Animation, [](const std::string& N, EAssetType T, const std::string& S){ return new FAnimation(N, T, S); });
	R->RegisterResourceFactory(EAssetType::AnimationGraph, [](const std::string& N, EAssetType T, const std::string& S){ return new FAnimationGraph(N, T, S); });
	R->RegisterResourceFactory(EAssetType::Prefab, [](const std::string& N, EAssetType T, const std::string& S){ return new FPrefab(N, T, S); });
}

namespace
{

struct FResourceFactoryRegistrar
{
	FResourceFactoryRegistrar()
	{
		RegisterResourceFactories();
	}
} GResourceFactoryRegistrar;

} // namespace

} // namespace Maho
