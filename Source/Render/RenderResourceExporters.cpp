#include <Core/Extension/Resource/ResourceSystem.h>
#include "ResourceSnapshotConverters.h"
#include <Core/Server/TransferHandle.h>
#include <Render/RenderSystem.h>
#include <Render/RenderFramePacket.h>

namespace Maho
{

// ---------------------------------------------------------------------------
// TRenderResourceExporter specializations (Game-thread Submit)
// ---------------------------------------------------------------------------

template <>
struct TRenderResourceExporter<FTexture>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FTexture& Texture)
	{
		FTextureCpuSnapshot Snap;
		if (!TryBuildTextureCpuSnapshot(Texture, Snap))
		{
			return AllocateTransferHandle(ETransferState::Failed);
		}
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingTextureUpload(std::move(Snap), Handle);
		return Handle;
	}

	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FTexture& Texture)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingTextureDestroy(Texture.GetSourcePath(), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<FTexture2D>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FTexture2D& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FTexture2D& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTexture3D>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FTexture3D& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FTexture3D& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTextureCube>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FTextureCube& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FTextureCube& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTextureCubeArray>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FTextureCubeArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FTextureCubeArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTexture2DArray>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FTexture2DArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FTexture2DArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FStaticMesh>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FStaticMesh& Mesh)
	{
		FMeshCpuSnapshot Snap;
		if (!TryBuildMeshCpuSnapshot(Mesh, Snap))
		{
			return AllocateTransferHandle(ETransferState::Failed);
		}
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingMeshUpload(std::move(Snap), Handle);
		return Handle;
	}

	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FStaticMesh& Mesh)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingMeshDestroy(Mesh.GetSourcePath(), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<FSkeleton>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FSkeleton& Skeleton)
	{
		FSkeletonCpuSnapshot Snap;
		if (!TryBuildSkeletonCpuSnapshot(Skeleton, Snap))
		{
			return AllocateTransferHandle(ETransferState::Failed);
		}
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingSkeletonUpload(std::move(Snap), Handle);
		return Handle;
	}

	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FSkeleton& Skeleton)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingSkeletonDestroy(Skeleton.GetSourcePath(), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<FAnimation>
{
	static FTransferHandle Submit(FRenderSystem& Server, const FAnimation& Animation)
	{
		FAnimationCpuSnapshot Snap;
		if (!TryBuildAnimationCpuSnapshot(Animation, Snap))
		{
			return AllocateTransferHandle(ETransferState::Failed);
		}
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingAnimationUpload(std::move(Snap), Handle);
		return Handle;
	}

	static FTransferHandle SubmitDestroy(FRenderSystem& Server, const FAnimation& Animation)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingAnimationDestroy(Animation.GetSourcePath(), Handle);
		return Handle;
	}
};

template <typename TResource>
FTransferHandle FRenderSystem::QueueResourceUpload(const TResource& Resource)
{
	static_assert(std::is_base_of_v<FResource, TResource>,
		"QueueResourceUpload requires TResource : FResource");
	return TRenderResourceExporter<TResource>::Submit(*this, Resource);
}

template <typename TResource>
FTransferHandle FRenderSystem::RequestResourceDestroy(const TResource& Resource)
{
	static_assert(std::is_base_of_v<FResource, TResource>,
		"RequestResourceDestroy requires TResource : FResource");
	return TRenderResourceExporter<TResource>::SubmitDestroy(*this, Resource);
}

template FTransferHandle FRenderSystem::QueueResourceUpload<FTexture>(const FTexture&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FTexture2D>(const FTexture2D&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FTexture3D>(const FTexture3D&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FTextureCube>(const FTextureCube&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FTextureCubeArray>(const FTextureCubeArray&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FTexture2DArray>(const FTexture2DArray&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FStaticMesh>(const FStaticMesh&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FSkeleton>(const FSkeleton&);
template FTransferHandle FRenderSystem::QueueResourceUpload<FAnimation>(const FAnimation&);

template FTransferHandle FRenderSystem::RequestResourceDestroy<FTexture>(const FTexture&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FTexture2D>(const FTexture2D&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FTexture3D>(const FTexture3D&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FTextureCube>(const FTextureCube&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FTextureCubeArray>(const FTextureCubeArray&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FTexture2DArray>(const FTexture2DArray&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FStaticMesh>(const FStaticMesh&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FSkeleton>(const FSkeleton&);
template FTransferHandle FRenderSystem::RequestResourceDestroy<FAnimation>(const FAnimation&);

// ---------------------------------------------------------------------------
// FRenderSystem
// ---------------------------------------------------------------------------



} // namespace Maho