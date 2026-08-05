#include "Game/System/Resource/ResourceSystem.h"
#include "Game/Object/SoftObjectPath.h"
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
struct TRenderResourceExporter<UTexture>
{
	static FTransferHandle Submit(FRenderServer& Server, const UTexture& Texture)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UTexture& Texture)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingTextureDestroy(FResourceSystem::MakeResourceCatalogKey(Texture), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<UTexture2D>
{
	static FTransferHandle Submit(FRenderServer& Server, const UTexture2D& Texture)
	{
		return TRenderResourceExporter<UTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UTexture2D& Texture)
	{
		return TRenderResourceExporter<UTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<UTexture3D>
{
	static FTransferHandle Submit(FRenderServer& Server, const UTexture3D& Texture)
	{
		return TRenderResourceExporter<UTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UTexture3D& Texture)
	{
		return TRenderResourceExporter<UTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<UTextureCube>
{
	static FTransferHandle Submit(FRenderServer& Server, const UTextureCube& Texture)
	{
		return TRenderResourceExporter<UTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UTextureCube& Texture)
	{
		return TRenderResourceExporter<UTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<UTextureCubeArray>
{
	static FTransferHandle Submit(FRenderServer& Server, const UTextureCubeArray& Texture)
	{
		return TRenderResourceExporter<UTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UTextureCubeArray& Texture)
	{
		return TRenderResourceExporter<UTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<UTexture2DArray>
{
	static FTransferHandle Submit(FRenderServer& Server, const UTexture2DArray& Texture)
	{
		return TRenderResourceExporter<UTexture>::Submit(Server, Texture);
	}
	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UTexture2DArray& Texture)
	{
		return TRenderResourceExporter<UTexture>::SubmitDestroy(Server, Texture);
	}
};

template <>
struct TRenderResourceExporter<UStaticMesh>
{
	static FTransferHandle Submit(FRenderServer& Server, const UStaticMesh& Mesh)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UStaticMesh& Mesh)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingMeshDestroy(FResourceSystem::MakeResourceCatalogKey(Mesh), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<USkeleton>
{
	static FTransferHandle Submit(FRenderServer& Server, const USkeleton& Skeleton)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const USkeleton& Skeleton)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingSkeletonDestroy(FResourceSystem::MakeResourceCatalogKey(Skeleton), Handle);
		return Handle;
	}
};

template <>
struct TRenderResourceExporter<UAnimation>
{
	static FTransferHandle Submit(FRenderServer& Server, const UAnimation& Animation)
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

	static FTransferHandle SubmitDestroy(FRenderServer& Server, const UAnimation& Animation)
	{
		FTransferHandle Handle = AllocateTransferHandle(ETransferState::InProgress);
		Server.PushPendingAnimationDestroy(FResourceSystem::MakeResourceCatalogKey(Animation), Handle);
		return Handle;
	}
};

template <typename TResource>
FTransferHandle FRenderServer::QueueResourceUpload(const TResource& Resource)
{
	static_assert(std::is_base_of_v<UResource, TResource>,
		"QueueResourceUpload requires TResource : UResource");
	return TRenderResourceExporter<TResource>::Submit(*this, Resource);
}

template <typename TResource>
FTransferHandle FRenderServer::RequestResourceDestroy(const TResource& Resource)
{
	static_assert(std::is_base_of_v<UResource, TResource>,
		"RequestResourceDestroy requires TResource : UResource");
	return TRenderResourceExporter<TResource>::SubmitDestroy(*this, Resource);
}

template FTransferHandle FRenderServer::QueueResourceUpload<UTexture>(const UTexture&);
template FTransferHandle FRenderServer::QueueResourceUpload<UTexture2D>(const UTexture2D&);
template FTransferHandle FRenderServer::QueueResourceUpload<UTexture3D>(const UTexture3D&);
template FTransferHandle FRenderServer::QueueResourceUpload<UTextureCube>(const UTextureCube&);
template FTransferHandle FRenderServer::QueueResourceUpload<UTextureCubeArray>(const UTextureCubeArray&);
template FTransferHandle FRenderServer::QueueResourceUpload<UTexture2DArray>(const UTexture2DArray&);
template FTransferHandle FRenderServer::QueueResourceUpload<UStaticMesh>(const UStaticMesh&);
template FTransferHandle FRenderServer::QueueResourceUpload<USkeleton>(const USkeleton&);
template FTransferHandle FRenderServer::QueueResourceUpload<UAnimation>(const UAnimation&);

template FTransferHandle FRenderServer::RequestResourceDestroy<UTexture>(const UTexture&);
template FTransferHandle FRenderServer::RequestResourceDestroy<UTexture2D>(const UTexture2D&);
template FTransferHandle FRenderServer::RequestResourceDestroy<UTexture3D>(const UTexture3D&);
template FTransferHandle FRenderServer::RequestResourceDestroy<UTextureCube>(const UTextureCube&);
template FTransferHandle FRenderServer::RequestResourceDestroy<UTextureCubeArray>(const UTextureCubeArray&);
template FTransferHandle FRenderServer::RequestResourceDestroy<UTexture2DArray>(const UTexture2DArray&);
template FTransferHandle FRenderServer::RequestResourceDestroy<UStaticMesh>(const UStaticMesh&);
template FTransferHandle FRenderServer::RequestResourceDestroy<USkeleton>(const USkeleton&);
template FTransferHandle FRenderServer::RequestResourceDestroy<UAnimation>(const UAnimation&);

// ---------------------------------------------------------------------------
// FRenderServer
// ---------------------------------------------------------------------------



} // namespace Maho