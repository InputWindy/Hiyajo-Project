#include "Render/TriangleBasePassFeature.h"
#include <Render/SceneUpdatePacket.h>

#include <cstring>
#include <vector>

#include <Core/Application/App.h>
#include <Core/Engine.h>
#include <Core/System/Log.h>
#include <Render/MahoCommonUniforms.h>
#include <Render/RDG/RDGBuilder.h>
#include <Render/RenderServer.h>
#include <Render/ShaderCache.h>
#include <Render/ShaderCompiler.h>
#include <Render/ShaderLoader.h>

#include "Render/RHI/VulkanCommandList.h"
#include "Render/RHI/VulkanRHI.h"
#include "Render/RHI/VulkanResources.h"

namespace Maho
{

namespace
{

struct FSimpleVertex
{
	float Pos[3];
	float Col[3];
};

static const FSimpleVertex TriangleVertices[] =
{
	{ {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
};

} // namespace

FTriangleBasePassFeature::FTriangleBasePassFeature()
	: TRenderFeatureBase("TriangleBasePass")
	, Ptr(std::make_unique<FImpl>())
{
}

FTriangleBasePassFeature::~FTriangleBasePassFeature() = default;

// ââ OnRegister / OnUnregister âââââââââââââââââââââââââââââââââââââ

bool FTriangleBasePassFeature::OnRegister(FRenderServer& RenderServer)
{
	return Initialize(RenderServer);
}

void FTriangleBasePassFeature::OnUnregister(FRenderServer& RenderServer)
{
	auto& S = *Ptr;
	if (!S.bInitialized)
	{
		return;
	}

	if (S.GameViewImGuiTexture.IsValid())
	{
		RenderServer.SetGameViewImGuiTexture({});
		RenderServer.SetGameViewExtent(0, 0);
		RenderServer.GetImGui().DestroyTexture(RenderServer.GetRHIServer(), S.GameViewImGuiTexture);
	}

	IRHI* RHI = RenderServer.GetRHIServer().GetRHI();
	if (RHI == nullptr)
	{
		S.bInitialized = false;
		return;
	}

	if (S.Pipeline)          { RHI->DestroyGraphicsPipeline(S.Pipeline); S.Pipeline = nullptr; }
	if (S.OffscreenFB)       { RHI->DestroyFramebuffer(S.OffscreenFB); S.OffscreenFB = nullptr; }
	if (S.OffscreenPass)     { RHI->DestroyRenderPass(S.OffscreenPass); S.OffscreenPass = nullptr; }
	if (S.ViewportTexView)   { RHI->DestroyTextureView(S.ViewportTexView); S.ViewportTexView = nullptr; }
	if (S.ViewportTex)       { RHI->DestroyTexture(S.ViewportTex); S.ViewportTex = nullptr; }
	if (S.DescPool)
	{
		if (S.FrameDescSet)  { RHI->FreeDescriptorSet(S.DescPool, S.FrameDescSet); S.FrameDescSet = nullptr; }
		if (S.ObjectDescSet) { RHI->FreeDescriptorSet(S.DescPool, S.ObjectDescSet); S.ObjectDescSet = nullptr; }
		RHI->DestroyDescriptorPool(S.DescPool);
		S.DescPool = nullptr;
	}
	if (S.ObjectSetLayout)   { RHI->DestroyDescriptorSetLayout(S.ObjectSetLayout); S.ObjectSetLayout = nullptr; }
	if (S.FrameSetLayout)    { RHI->DestroyDescriptorSetLayout(S.FrameSetLayout); S.FrameSetLayout = nullptr; }
	if (S.PipelineLayout)    { RHI->DestroyPipelineLayout(S.PipelineLayout); S.PipelineLayout = nullptr; }
	if (S.ObjectUniformBuf)  { RHI->DestroyBuffer(S.ObjectUniformBuf); S.ObjectUniformBuf = nullptr; }
	if (S.FrameUniformBuf)   { RHI->DestroyBuffer(S.FrameUniformBuf); S.FrameUniformBuf = nullptr; }
	if (S.TriangleVBO)       { RHI->DestroyBuffer(S.TriangleVBO); S.TriangleVBO = nullptr; }
	if (S.FragmentShader)    { RHI->DestroyShaderModule(S.FragmentShader); S.FragmentShader = nullptr; }
	if (S.VertexShader)      { RHI->DestroyShaderModule(S.VertexShader); S.VertexShader = nullptr; }

	S.ShaderLoader.reset();
	S.ShaderCache.reset();
	S.bViewportShaderResource = false;
	S.bInitialized = false;
}

// ââ BuildRenderGraph ââââââââââââââââââââââââââââââââââââââââââââââ

void FTriangleBasePassFeature::BuildRenderGraph(FRDGBuilder& GB, FRenderServer& Server)
{
	auto& S = *Ptr;
	if (!S.bInitialized)
	{
		return;
	}

	const FSceneUpdatePacket& Scene = Server.GetCurrentScene();
	if (Scene.Draws.empty())
	{
		return;
	}
	const std::vector<FSceneDrawItem>& DrawItems = Scene.Draws;

	{
		FFrameUniforms Uni{};
		Uni.View[0] = Uni.View[5] = Uni.View[10] = Uni.View[15] = 1.0f;
		Uni.Proj[0] = Uni.Proj[5] = Uni.Proj[10] = Uni.Proj[15] = 1.0f;
		Uni.ViewProj[0] = Uni.ViewProj[5] = Uni.ViewProj[10] = Uni.ViewProj[15] = 1.0f;
		S.RHI->UpdateBuffer(S.FrameUniformBuf, 0, sizeof(FFrameUniforms), &Uni);
	}
	{
		FObjectUniforms Uni{};
		std::memcpy(Uni.LocalToWorld, DrawItems[0].LocalToWorld, sizeof(Uni.LocalToWorld));
		std::memcpy(Uni.LocalToWorldInverseTranspose, DrawItems[0].LocalToWorld, sizeof(Uni.LocalToWorldInverseTranspose));
		S.RHI->UpdateBuffer(S.ObjectUniformBuf, 0, sizeof(FObjectUniforms), &Uni);
	}

	auto* RDGViewport = GB.RegisterExternalTexture(
		S.ViewportTex,
		S.bViewportShaderResource ? ERHIResourceState::ShaderResource : ERHIResourceState::Common,
		"ViewportTex");
	auto* RDGVBO = GB.RegisterExternalBuffer(
		S.TriangleVBO, ERHIResourceState::VertexBuffer, "TriVBO");
	auto* RDGFrameUBO = GB.RegisterExternalBuffer(
		S.FrameUniformBuf, ERHIResourceState::UniformBuffer, "FrameUBO");
	auto* RDGObjUBO = GB.RegisterExternalBuffer(
		S.ObjectUniformBuf, ERHIResourceState::UniformBuffer, "ObjUBO");

	auto& DrawPass = GB.AddRasterPass("DrawTriangleActors");

	// Write: RDG inserts current -> RenderTarget before execute.
	GB.Write(DrawPass, RDGViewport, ERHIResourceState::RenderTarget);
	GB.Read(DrawPass, RDGVBO,      ERHIResourceState::VertexBuffer);
	GB.Read(DrawPass, RDGFrameUBO, ERHIResourceState::UniformBuffer);
	GB.Read(DrawPass, RDGObjUBO,   ERHIResourceState::UniformBuffer);

	DrawPass.SetExecute([this, Draws = std::move(DrawItems)](FRHICommandList& Cmd) mutable
	{
		auto& S = *Ptr;

		auto* VkCmdList = static_cast<FVulkanCommandList*>(&Cmd);
		VkCommandBuffer VkBuf = VkCmdList->GetVkCommandBuffer();

		auto* VkPass  = static_cast<FVulkanRenderPass*>(S.OffscreenPass);
		auto* VkFB    = static_cast<FVulkanFramebuffer*>(S.OffscreenFB);
		auto* VkDesc0 = static_cast<FVulkanDescriptorSet*>(S.FrameDescSet);
		auto* VkDesc1 = static_cast<FVulkanDescriptorSet*>(S.ObjectDescSet);

		{
			VkRenderPassBeginInfo Info{};
			Info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			Info.renderPass = VkPass->GetVkPass();
			Info.framebuffer = VkFB->GetVkFramebuffer();
			Info.renderArea.extent = { S.VpWidth, S.VpHeight };
			VkClearValue CV{};
			CV.color = { { 0.12f, 0.18f, 0.28f, 1.0f } };
			Info.clearValueCount = 1;
			Info.pClearValues = &CV;
			vkCmdBeginRenderPass(VkBuf, &Info, VK_SUBPASS_CONTENTS_INLINE);
		}

		Cmd.BindGraphicsPipeline(S.Pipeline);

		{
			VkDescriptorSet Sets[] = { VkDesc0->GetVkSet(), VkDesc1->GetVkSet() };
			vkCmdBindDescriptorSets(VkBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
				static_cast<FVulkanPipelineLayout*>(S.PipelineLayout)->GetVkLayout(),
				0, 2, Sets, 0, nullptr);
		}

		Cmd.BindVertexBuffer(0, S.TriangleVBO);
		Cmd.SetViewport(0.0f, 0.0f, static_cast<float>(S.VpWidth), static_cast<float>(S.VpHeight));
		Cmd.SetScissor(0, 0, S.VpWidth, S.VpHeight);

		for (const FSceneDrawItem& Item : Draws)
		{
			FObjectUniforms Uni{};
			std::memcpy(Uni.LocalToWorld, Item.LocalToWorld, sizeof(Uni.LocalToWorld));
			std::memcpy(Uni.LocalToWorldInverseTranspose, Item.LocalToWorld, sizeof(Uni.LocalToWorldInverseTranspose));
			S.RHI->UpdateBuffer(S.ObjectUniformBuf, 0, sizeof(FObjectUniforms), &Uni);
			Cmd.Draw(3, 1, 0, 0);
		}

		vkCmdEndRenderPass(VkBuf);

		Cmd.TransitionTexture(
			S.ViewportTex,
			ERHIResourceState::RenderTarget,
			ERHIResourceState::ShaderResource);
		S.bViewportShaderResource = true;
	});
}

// ââ Initialize ââââââââââââââââââââââââââââââââââââââââââââââââââââ

bool FTriangleBasePassFeature::Initialize(FRenderServer& RenderServer)
{
	auto& S = *Ptr;
	S.RHI = RenderServer.GetRHIServer().GetRHI();
	if (S.RHI == nullptr)
	{
		return false;
	}

	auto* VkRHI = RenderServer.GetVulkanRHI();
	if (VkRHI == nullptr)
	{
		return false;
	}

	if (!FShaderCompiler::Initialize())
	{
		MAHO_CORE_ERROR("TriangleBasePass: shader compiler init failed");
		return false;
	}

	if (!GApp)
	{
		MAHO_CORE_ERROR("TriangleBasePass: GApp missing (need Config shader paths)");
		return false;
	}

	const FConfig& Config = GApp->GetConfig();
	const std::string EngineCommon = Config.EngineShadersDir + "/Common";
	const std::string ProjectCommon = Config.ProjectShadersDir + "/Common";

	S.ShaderCache = std::make_unique<FShaderCache>(Config.CachedDir);
	S.ShaderLoader = std::make_unique<FShaderLoader>(
		*S.ShaderCache,
		std::vector<std::string>{ Config.EngineShadersDir, Config.ProjectShadersDir },
		std::vector<std::string>{ EngineCommon, ProjectCommon });

	FShaderPackage Package = S.ShaderLoader->LoadShader("Triangle.shader");
	if (!Package.Vertex.bSuccess || !Package.Fragment.bSuccess)
	{
		MAHO_CORE_ERROR(
			"TriangleBasePass: shader load failed (EngineShadersDir='{}')\nVS: {}\nFS: {}",
			Config.EngineShadersDir,
			Package.Vertex.ErrorLog,
			Package.Fragment.ErrorLog);
		return false;
	}

	// Shader modules
	{
		FRHIShaderModuleDesc VsDesc;
		VsDesc.Stage = ERHIShaderStage::Vertex;
		VsDesc.Bytecode = Package.Vertex.Bytecode.data();
		VsDesc.BytecodeSize = Package.Vertex.Bytecode.size() * sizeof(std::uint32_t);
		S.VertexShader = S.RHI->CreateShaderModule(VsDesc);

		FRHIShaderModuleDesc FsDesc;
		FsDesc.Stage = ERHIShaderStage::Fragment;
		FsDesc.Bytecode = Package.Fragment.Bytecode.data();
		FsDesc.BytecodeSize = Package.Fragment.Bytecode.size() * sizeof(std::uint32_t);
		S.FragmentShader = S.RHI->CreateShaderModule(FsDesc);
	}

	if (S.VertexShader == nullptr || S.FragmentShader == nullptr)
	{
		return false;
	}

	// VBO (upload via staging)
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(TriangleVertices);
		Desc.Usage = ERHIBufferUsage::Vertex;
		Desc.MemoryUsage = ERHIMemoryUsage::GPUOnly;
		S.TriangleVBO = S.RHI->CreateBuffer(Desc);

		FRHIBufferDesc StageDesc;
		StageDesc.Size = sizeof(TriangleVertices);
		StageDesc.Usage = ERHIBufferUsage::TransferSrc;
		StageDesc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		FRHIBuffer* Staging = S.RHI->CreateBuffer(StageDesc);
		if (Staging)
		{
			S.RHI->UpdateBuffer(Staging, 0, sizeof(TriangleVertices), TriangleVertices);
			FRHICommandList* Cmd = S.RHI->CreateCommandList(ERHICommandListType::Graphics);
			Cmd->Begin();
			Cmd->CopyBuffer(Staging, 0, S.TriangleVBO, 0, sizeof(TriangleVertices));
			Cmd->End();
			S.RHI->GetGraphicsQueue().Submit(&Cmd, 1, nullptr, 0, nullptr, 0, nullptr);
			S.RHI->DestroyCommandList(Cmd);
			S.RHI->DestroyBuffer(Staging);
		}
	}

	// Uniform buffers
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(FFrameUniforms);
		Desc.Usage = ERHIBufferUsage::Uniform;
		Desc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		S.FrameUniformBuf = S.RHI->CreateBuffer(Desc);
	}
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(FObjectUniforms);
		Desc.Usage = ERHIBufferUsage::Uniform;
		Desc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		S.ObjectUniformBuf = S.RHI->CreateBuffer(Desc);
	}

	// Descriptor set layouts
	{
		FRHIDescriptorSetLayoutDesc Desc;
		FRHIDescriptorBinding B;
		B.Binding = 0;
		B.Type = ERHIDescriptorType::UniformBuffer;
		B.Count = 1;
		B.Stages = ERHIShaderStage::AllGraphics;
		Desc.Bindings.push_back(B);
		S.FrameSetLayout = S.RHI->CreateDescriptorSetLayout(Desc);
		S.ObjectSetLayout = S.RHI->CreateDescriptorSetLayout(Desc);
	}

	// Pipeline layout
	{
		FRHIPipelineLayoutDesc Desc;
		Desc.SetLayouts = { S.FrameSetLayout, S.ObjectSetLayout };
		S.PipelineLayout = S.RHI->CreatePipelineLayout(Desc);
	}

	// Offscreen render pass
	{
		FRHIRenderPassDesc Desc;
		FRHIRenderPassAttachment Att;
		Att.Format = ERHIFormat::B8G8R8A8_UNORM;
		Att.LoadOp = ERHILoadOp::Clear;
		Att.StoreOp = ERHIStoreOp::Store;
		Desc.ColorAttachments.push_back(Att);
		S.OffscreenPass = S.RHI->CreateRenderPass(Desc);
	}

	// Offscreen texture + view + framebuffer
	{
		FRHITextureDesc Desc;
		Desc.Format = ERHIFormat::B8G8R8A8_UNORM;
		Desc.Dimension = ERHITextureDimension::Tex2D;
		Desc.Extent = { S.VpWidth, S.VpHeight, 1 };
		Desc.Usage = ERHITextureUsage::ColorAttachment | ERHITextureUsage::Sampled;
		S.ViewportTex = S.RHI->CreateTexture(Desc);

		FRHITextureViewDesc ViewDesc;
		ViewDesc.Texture = S.ViewportTex;
		ViewDesc.Format = ERHIFormat::B8G8R8A8_UNORM;
		S.ViewportTexView = S.RHI->CreateTextureView(ViewDesc);
		// Fresh VkImage is UNDEFINED — first BasePass registers it as Common.
		S.bViewportShaderResource = false;
	}
	{
		FRHIFramebufferDesc Desc;
		Desc.RenderPass = S.OffscreenPass;
		Desc.Attachments = { S.ViewportTexView };
		Desc.Width = S.VpWidth;
		Desc.Height = S.VpHeight;
		S.OffscreenFB = S.RHI->CreateFramebuffer(Desc);
	}

	// Graphics pipeline
	{
		FRHIGraphicsPipelineDesc Desc;
		Desc.VertexShader = S.VertexShader;
		Desc.FragmentShader = S.FragmentShader;
		Desc.Layout = S.PipelineLayout;
		Desc.RenderPass = S.OffscreenPass;
		Desc.Topology = ERHIPrimitiveTopology::TriangleList;
		Desc.VertexStride = sizeof(FSimpleVertex);

		FRHIVertexAttribute Pos;
		Pos.Location = 0;
		Pos.Format = ERHIFormat::R32G32B32_SFLOAT;
		Pos.Offset = 0;
		Desc.Attributes.push_back(Pos);

		FRHIVertexAttribute Col;
		Col.Location = 1;
		Col.Format = ERHIFormat::R32G32B32_SFLOAT;
		Col.Offset = 12;
		Desc.Attributes.push_back(Col);

		Desc.CullMode = ERHICullMode::None;
		Desc.ColorFormat = ERHIFormat::B8G8R8A8_UNORM;

		FRHIAttachmentBlend Blend;
		Desc.AttachmentBlends.push_back(Blend);

		S.Pipeline = S.RHI->CreateGraphicsPipeline(Desc);
	}

	// Descriptor pool + sets
	{
		FRHIDescriptorPoolDesc Desc;
		Desc.MaxSets = 2;
		FRHIDescriptorPoolSize Sz;
		Sz.Type = ERHIDescriptorType::UniformBuffer;
		Sz.Count = 2;
		Desc.PoolSizes.push_back(Sz);
		S.DescPool = S.RHI->CreateDescriptorPool(Desc);
	}
	S.FrameDescSet = S.RHI->AllocateDescriptorSet(S.DescPool, S.FrameSetLayout);
	S.ObjectDescSet = S.RHI->AllocateDescriptorSet(S.DescPool, S.ObjectSetLayout);

	{
		FRHIDescriptorWrite W{};
		W.Set = S.FrameDescSet;
		W.Binding = 0;
		W.Type = ERHIDescriptorType::UniformBuffer;
		W.Buffer = S.FrameUniformBuf;
		W.Range = sizeof(FFrameUniforms);
		S.RHI->UpdateDescriptorSets(&W, 1);
	}
	{
		FRHIDescriptorWrite W{};
		W.Set = S.ObjectDescSet;
		W.Binding = 0;
		W.Type = ERHIDescriptorType::UniformBuffer;
		W.Buffer = S.ObjectUniformBuf;
		W.Range = sizeof(FObjectUniforms);
		S.RHI->UpdateDescriptorSets(&W, 1);
	}

	if (RenderServer.GetImGui().IsInitialized())
	{
		FImGuiTextureHandle Handle;
		if (RenderServer.GetImGui().RegisterExternalSampledTexture(
			RenderServer.GetRHIServer(),
			S.ViewportTexView,
			Handle))
		{
			S.GameViewImGuiTexture = Handle;
			RenderServer.SetGameViewImGuiTexture(Handle);
			RenderServer.SetGameViewExtent(S.VpWidth, S.VpHeight);
		}
		else
		{
			MAHO_CORE_ERROR("TriangleBasePass: failed to register game-view ImGui texture");
		}
	}

	S.bInitialized = true;
	MAHO_CORE_INFO("TriangleBasePass: RDG-native ({}x{})", S.VpWidth, S.VpHeight);
	return true;
}

void RegisterTriangleBasePassFeature(FRenderServer& Server)
{
	Server.RegisterFeature<FTriangleBasePassFeature>();
}

} // namespace Maho
