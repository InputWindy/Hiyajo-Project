#include "Game/System/Resource/ResourceSystem.h"
#include "ResourceSnapshotConverters.h"
#include <Core/Server/TransferHandle.h>
#include <Render/RenderServer.h>
#include <Render/RenderFramePacket.h>

namespace Maho
{

// ---------------------------------------------------------------------------
// TRenderResourceExporter specializations (Game-thread Submit)
// ---------------------------------------------------------------------------

template <>
struct TRenderResourceExporter<FTexture>
{
	static FTransferHandle Submit(FRenderServer& Server, const FTexture& Texture)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FTexture& Texture)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingTextureDestroy(Texture.GetSourcePath(), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<FTexture2D>
{
	static FTransferHandle Submit(FRenderServer& Server, const FTexture2D& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FTexture2D& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTexture3D>
{
	static FTransferHandle Submit(FRenderServer& Server, const FTexture3D& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FTexture3D& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTextureCube>
{
	static FTransferHandle Submit(FRenderServer& Server, const FTextureCube& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FTextureCube& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTextureCubeArray>
{
	static FTransferHandle Submit(FRenderServer& Server, const FTextureCubeArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FTextureCubeArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FTexture2DArray>
{
	static FTransferHandle Submit(FRenderServer& Server, const FTexture2DArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FTexture2DArray& Texture)
	{
		return TRenderResourceExporter<FTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<FStaticMesh>
{
	static FTransferHandle Submit(FRenderServer& Server, const FStaticMesh& Mesh)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FStaticMesh& Mesh)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingMeshDestroy(Mesh.GetSourcePath(), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<FSkeleton>
{
	static FTransferHandle Submit(FRenderServer& Server, const FSkeleton& Skeleton)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FSkeleton& Skeleton)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingSkeletonDestroy(Skeleton.GetSourcePath(), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<FAnimation>
{
	static FTransferHandle Submit(FRenderServer& Server, const FAnimation& Animation)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const FAnimation& Animation)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingAnimationDestroy(Animation.GetSourcePath(), Handle);
		return Handle;
	}
};

template <typename TResource>
FTransferHandle FRenderServer::QueueResourceUpload(const TResource& Resource)
{
	static_assert(std::is_base_of_v<FResource, TResource>,
		"QueueResourceUpload requires TResource : FResource");
	return TRenderResourceExporter<TResource>::Submit(*this, Resource);
}

template <typename TResource>
FTransferHandle FRenderServer::RequestResourceDestroy(const TResource& Resource)
{
	static_assert(std::is_base_of_v<FResource, TResource>,
		"RequestResourceDestroy requires TResource : FResource");
	return TRenderResourceExporter<TResource>::SubmitDestroy(*this, Resource);
}

template FTransferHandle FRenderServer::QueueResourceUpload<FTexture>(const FTexture&);
template FTransferHandle FRenderServer::QueueResourceUpload<FTexture2D>(const FTexture2D&);
template FTransferHandle FRenderServer::QueueResourceUpload<FTexture3D>(const FTexture3D&);
template FTransferHandle FRenderServer::QueueResourceUpload<FTextureCube>(const FTextureCube&);
template FTransferHandle FRenderServer::QueueResourceUpload<FTextureCubeArray>(const FTextureCubeArray&);
template FTransferHandle FRenderServer::QueueResourceUpload<FTexture2DArray>(const FTexture2DArray&);
template FTransferHandle FRenderServer::QueueResourceUpload<FStaticMesh>(const FStaticMesh&);
template FTransferHandle FRenderServer::QueueResourceUpload<FSkeleton>(const FSkeleton&);
template FTransferHandle FRenderServer::QueueResourceUpload<FAnimation>(const FAnimation&);

template FTransferHandle FRenderServer::RequestResourceDestroy<FTexture>(const FTexture&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FTexture2D>(const FTexture2D&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FTexture3D>(const FTexture3D&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FTextureCube>(const FTextureCube&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FTextureCubeArray>(const FTextureCubeArray&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FTexture2DArray>(const FTexture2DArray&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FStaticMesh>(const FStaticMesh&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FSkeleton>(const FSkeleton&);
template FTransferHandle FRenderServer::RequestResourceDestroy<FAnimation>(const FAnimation&);

// ---------------------------------------------------------------------------
// FRenderServer
// ---------------------------------------------------------------------------



} // namespace Maho