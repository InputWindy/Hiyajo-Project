#include "Render/TriangleBasePassFeature.h"
#include <Render/SceneUpdatePacket.h>

#include <cmath>
#include <cstring>
#include <vector>

#include <Core/Application/App.h>
#include <Core/Engine.h>
#include <Core/System/Log.h>
#include <Render/MahoCommonUniforms.h>
#include <Render/RDG/RDGBuilder.h>
#include <Render/RenderServer.h>
#include <Render/ShaderCompiler.h>

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

/** Compute a right‑handed perspective projection matrix (Vulkan clip space, z∈[0,1]). */
static void BuildPerspProj(float FOV, float Aspect, float Near, float Far, float Out[16])
{
	float F = 1.0f / std::tan((FOV * 0.5f) * 3.14159265f / 180.0f);
	float M[16] = {
		F / Aspect, 0, 0, 0,
		0, F, 0, 0,
		0, 0, Far / (Far - Near), 1,
		0, 0, -(Near * Far) / (Far - Near), 0,
	};
	std::memcpy(Out, M, sizeof(M));
}

} // namespace

FTriangleBasePassFeature::FTriangleBasePassFeature()
	: TRenderFeatureBase("TriangleBasePass")
	, Ptr(std::make_unique<FImpl>())
{
}

FTriangleBasePassFeature::~FTriangleBasePassFeature() = default;

// ═══════════════════════════════════════════
// OnRegister / OnUnregister
// ═══════════════════════════════════════════

bool FTriangleBasePassFeature::OnRegister(FRenderServer& RenderServer)
{
	if (!SetupPersistentResources(RenderServer))
	{
		MAHO_CORE_ERROR("TriangleBasePass: persistent resource setup failed");
		return false;
	}

	// Shader compilation is deferred until first BuildRenderGraph (lazy-init).
	MAHO_CORE_INFO("TriangleBasePass: registered (shader lazy)");
	return true;
}

void FTriangleBasePassFeature::OnUnregister(FRenderServer& RenderServer)
{
	auto& S = *Ptr;
	if (!S.bInitialized) return;

	if (S.GameViewImGuiTexture.IsValid())
	{
		RenderServer.SetGameViewImGuiTexture({});
		RenderServer.SetGameViewExtent(0, 0);
		RenderServer.GetImGui().DestroyTexture(RenderServer.GetRHIServer(), S.GameViewImGuiTexture);
	}

	IRHI* RHI = RenderServer.GetRHIServer().GetRHI();
	if (!RHI) { S.bInitialized = false; return; }

	DestroyShaderResources();

	if (S.ViewportTexView)   { RHI->DestroyTextureView(S.ViewportTexView); S.ViewportTexView = nullptr; }
	if (S.ViewportTex)       { RHI->DestroyTexture(S.ViewportTex); S.ViewportTex = nullptr; }
	if (S.FrameUniformBuf)   { RHI->DestroyBuffer(S.FrameUniformBuf); S.FrameUniformBuf = nullptr; }
	if (S.ObjectUniformBuf)  { RHI->DestroyBuffer(S.ObjectUniformBuf); S.ObjectUniformBuf = nullptr; }
	if (S.TriangleVBO)       { RHI->DestroyBuffer(S.TriangleVBO); S.TriangleVBO = nullptr; }

	S.ShaderDb.reset();
	S.bInitialized = false;
}

// ═══════════════════════════════════════════
// BuildRenderGraph  —  declarative RDG + dynamic rendering
// ═══════════════════════════════════════════

void FTriangleBasePassFeature::BuildRenderGraph(FRDGBuilder& GB, FRenderServer& Server)
{
	auto& S = *Ptr;
	if (!S.bInitialized) return;

	// Lazy shader compilation — first frame only, or after MarkShaderDirty().
	if (!EnsureShaderReady())
	{
		MAHO_CORE_ERROR("TriangleBasePass: shader not ready, skipping frame");
		return;
	}

	const FSceneUpdatePacket& Scene = Server.GetCurrentScene();
	if (Scene.Draws.empty()) return;

	// ── CPU‑side upload (host‑visible persistent buffers) ──

	FFrameUniforms FrameUni{};
	std::memcpy(FrameUni.View, Scene.Camera.View, sizeof(FrameUni.View));
	BuildPerspProj(Scene.Camera.FOV, Scene.Camera.AspectRatio,
	               Scene.Camera.NearPlane, Scene.Camera.FarPlane, FrameUni.Proj);
	// ViewProj = Proj * View (column-major mat4 multiply)
	for (int Col = 0; Col < 4; ++Col)
	{
		for (int Row = 0; Row < 4; ++Row)
		{
			float Sum = 0.0f;
			for (int K = 0; K < 4; ++K)
				Sum += FrameUni.Proj[K * 4 + Row] * FrameUni.View[Col * 4 + K];
			FrameUni.ViewProj[Col * 4 + Row] = Sum;
		}
	}
	S.RHI->UpdateBuffer(S.FrameUniformBuf, 0, sizeof(FrameUni), &FrameUni);

	std::vector<FObjectUniforms> ObjectData;
	ObjectData.reserve(Scene.Draws.size());
	for (const auto& Item : Scene.Draws)
	{
		FObjectUniforms Uni{};
		std::memcpy(Uni.LocalToWorld, Item.LocalToWorld, sizeof(Uni.LocalToWorld));
		std::memcpy(Uni.LocalToWorldInverseTranspose, Item.LocalToWorld,
		            sizeof(Uni.LocalToWorldInverseTranspose));
		ObjectData.push_back(Uni);
	}
	if (!ObjectData.empty())
		S.RHI->UpdateBuffer(S.ObjectUniformBuf, 0,
		                    ObjectData.size() * sizeof(FObjectUniforms), ObjectData.data());

	// ── RDG resource registration ──

	FRDGTexture* RDGViewport = GB.RegisterExternalTexture(
		S.ViewportTex, ERHIResourceState::Common, "ViewportTex");
	FRDGBuffer* RDGVBO = GB.RegisterExternalBuffer(
		S.TriangleVBO, ERHIResourceState::VertexBuffer, "TriVBO");
	FRDGBuffer* RDGFrameUBO = GB.RegisterExternalBuffer(
		S.FrameUniformBuf, ERHIResourceState::Common, "FrameUBO");
	FRDGBuffer* RDGObjUBO = GB.RegisterExternalBuffer(
		S.ObjectUniformBuf, ERHIResourceState::Common, "ObjectUBO");

	// ── Draw pass (per batch, parameter‑based with dynamic rendering) ──

	for (auto& Pair : S.Batches)
	{
		FBatchResources& Batch = Pair.second;

		auto& Params = GB.AllocateParameters();
		Params.Reads = {
			{RDGVBO,       ERHIResourceState::VertexBuffer},
			{RDGFrameUBO,  ERHIResourceState::UniformBuffer},
			{RDGObjUBO,    ERHIResourceState::UniformBuffer},
		};
		Params.Writes = {
			{RDGViewport,  ERHIResourceState::RenderTarget},
		};
		FRDGPassParameters::FRenderTargetBinding RT{};
		RT.Texture = RDGViewport;
		RT.View = S.ViewportTexView;
		RT.LoadOp = ERHILoadOp::Clear;
		RT.StoreOp = ERHIStoreOp::Store;
		RT.ClearColor[0] = 0.2f;
		RT.ClearColor[1] = 0.2f;
		RT.ClearColor[2] = 0.2f;
		RT.ClearColor[3] = 1.0f;
		Params.RenderTargets = { RT };

		GB.AddRasterPass("DrawTriangles_Batch",
			Params,
			[VpW = S.VpWidth, VpH = S.VpHeight,
			 Pipeline = Batch.Pipeline,
			 FrameSet = Batch.FrameDescSet,
			 ObjSet = Batch.ObjectDescSet,
			 VBO = S.TriangleVBO,
			 NumDraws = static_cast<std::size_t>(Scene.Draws.size())]
			 (FRHICommandList& Cmd) mutable
			{
				Cmd.BindGraphicsPipeline(Pipeline);
				FRHIDescriptorSet* Sets[] = { FrameSet, ObjSet };
				Cmd.BindDescriptorSets(0, Sets, 2);
				Cmd.BindVertexBuffer(0, VBO);
				Cmd.SetViewport(0.0f, 0.0f, static_cast<float>(VpW), static_cast<float>(VpH));
				Cmd.SetScissor(0, 0, VpW, VpH);
				for (std::size_t I = 0; I < NumDraws; ++I)
					Cmd.Draw(3, 1, 0, 0);
			});
	}
}

// ═══════════════════════════════════════════
// SetupPersistentResources (no shader compilation)
// ═══════════════════════════════════════════

bool FTriangleBasePassFeature::SetupPersistentResources(FRenderServer& RenderServer)
{
	auto& S = *Ptr;
	if (S.bInitialized) return true;

	S.RHI = RenderServer.GetRHIServer().GetRHI();
	if (!S.RHI) return false;

	// VBO (persistent, uploaded once at init)
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

	// Frame UBO — host‑visible, updated via UpdateBuffer each frame
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(FFrameUniforms);
		Desc.Usage = ERHIBufferUsage::Uniform;
		Desc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		S.FrameUniformBuf = S.RHI->CreateBuffer(Desc);
	}

	// Object UBO — host‑visible, filled in bulk before RDG
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(FObjectUniforms) * 128;
		Desc.Usage = ERHIBufferUsage::Uniform;
		Desc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		S.ObjectUniformBuf = S.RHI->CreateBuffer(Desc);
	}

	// Viewport texture (dynamic rendering — no render pass / framebuffer needed)
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
	}

	if (RenderServer.GetImGui().IsInitialized())
	{
		FImGuiTextureHandle Handle;
		if (RenderServer.GetImGui().RegisterExternalSampledTexture(
			RenderServer.GetRHIServer(), S.ViewportTexView, Handle))
		{
			S.GameViewImGuiTexture = Handle;
			RenderServer.SetGameViewImGuiTexture(Handle);
			RenderServer.SetGameViewExtent(S.VpWidth, S.VpHeight);
		}
	}

	S.bInitialized = true;
	MAHO_CORE_INFO("TriangleBasePass: persistent resources ready ({}x{})", S.VpWidth, S.VpHeight);
	return true;
}

// ═══════════════════════════════════════════
// Lazy shader compilation + batch creation
// ═══════════════════════════════════════════

bool FTriangleBasePassFeature::EnsureShaderReady()
{
	auto& S = *Ptr;
	if (S.bShaderReady) return true;
	if (!S.RHI) return false;

	// Init glslang once (idempotent).
	if (!FShaderCompiler::Initialize())
	{
		MAHO_CORE_ERROR("TriangleBasePass: shader compiler init failed");
		return false;
	}

	if (!GApp)
	{
		MAHO_CORE_ERROR("TriangleBasePass: GApp missing for shader compile");
		return false;
	}

	const FConfig& Config = GApp->GetConfig();
	const std::string EngineCommon = Config.EngineShadersDir + "/Common";
	const std::string ProjectCommon = Config.ProjectShadersDir + "/Common";

	MAHO_CORE_INFO("TriangleBasePass: search paths: ProjectShadersDir='{}' EngineShadersDir='{}'",
	               Config.ProjectShadersDir, Config.EngineShadersDir);

	// Destroy old batches if recompiling (hot-reload).
	DestroyShaderResources();
	if (!S.ShaderDb)
		S.ShaderDb = std::make_unique<FShaderDatabase>();

	MAHO_CORE_INFO("TriangleBasePass: compiling shader...");

	if (!S.ShaderDb->LoadShader("Triangle.shader",
		{ Config.ProjectShadersDir, Config.EngineShadersDir },
		{ ProjectCommon, EngineCommon },
		Config.CachedDir))
	{
		MAHO_CORE_ERROR("TriangleBasePass: shader load/compile failed");
		return false;
	}

	const auto& Passes = S.ShaderDb->GetAllPasses();
	if (Passes.empty())
	{
		MAHO_CORE_ERROR("TriangleBasePass: no passes in compiled shader");
		return false;
	}

	for (const auto& Pass : Passes)
	{
		if (S.Batches.find(Pass.BytecodeHash) != S.Batches.end()) continue;
		S.Batches[Pass.BytecodeHash] = CreateBatchResources(Pass);
	}

	S.bShaderReady = true;
	MAHO_CORE_INFO("TriangleBasePass: shader compiled, {} passes -> {} batches",
	               Passes.size(), S.Batches.size());
	return true;
}

void FTriangleBasePassFeature::DestroyShaderResources()
{
	auto& S = *Ptr;
	IRHI* RHI = S.RHI;
	if (!RHI) return;

	for (auto& Pair : S.Batches)
		DestroyBatchResources(Pair.second);
	S.Batches.clear();
	S.ShaderDb.reset();
	S.bShaderReady = false;
}

// ═══════════════════════════════════════════
// Batch resources
// ═══════════════════════════════════════════

FTriangleBasePassFeature::FBatchResources
FTriangleBasePassFeature::CreateBatchResources(const FShaderPassCompiled& Pass)
{
	IRHI* RHI = Ptr->RHI;
	FBatchResources B;
	B.PassDesc = &Pass;

	{
		FRHIShaderModuleDesc VsDesc;
		VsDesc.Stage = ERHIShaderStage::Vertex;
		VsDesc.Bytecode = Pass.VertexBytecode.data();
		VsDesc.BytecodeSize = Pass.VertexBytecode.size() * sizeof(std::uint32_t);
		B.VertexShader = RHI->CreateShaderModule(VsDesc);

		FRHIShaderModuleDesc FsDesc;
		FsDesc.Stage = ERHIShaderStage::Fragment;
		FsDesc.Bytecode = Pass.FragmentBytecode.data();
		FsDesc.BytecodeSize = Pass.FragmentBytecode.size() * sizeof(std::uint32_t);
		B.FragmentShader = RHI->CreateShaderModule(FsDesc);
	}

	{
		FRHIDescriptorSetLayoutDesc Desc;
		FRHIDescriptorBinding FB;
		FB.Binding = 0;
		FB.Type = ERHIDescriptorType::UniformBuffer;
		FB.Count = 1;
		FB.Stages = ERHIShaderStage::AllGraphics;
		Desc.Bindings.push_back(FB);
		B.FrameSetLayout = RHI->CreateDescriptorSetLayout(Desc);
		B.ObjectSetLayout = RHI->CreateDescriptorSetLayout(Desc);
	}

	{
		FRHIPipelineLayoutDesc Desc;
		Desc.SetLayouts = { B.FrameSetLayout, B.ObjectSetLayout };
		B.PipelineLayout = RHI->CreatePipelineLayout(Desc);
	}

	{
		FRHIGraphicsPipelineDesc Desc;
		Desc.VertexShader = B.VertexShader;
		Desc.FragmentShader = B.FragmentShader;
		Desc.Layout = B.PipelineLayout;
		Desc.RenderPass = nullptr;
		Desc.Topology = ERHIPrimitiveTopology::TriangleList;
		Desc.VertexStride = sizeof(FSimpleVertex);

		FRHIVertexAttribute Pos;
		Pos.Location = 0;
		Pos.Format = ERHIFormat::R32G32B32_SFLOAT;
		Pos.Offset = 0;
		Desc.Attributes.push_back(Pos);

		FRHIVertexAttribute Col;
		Col.Location = 5;
		Col.Format = ERHIFormat::R32G32B32_SFLOAT;
		Col.Offset = 12;
		Desc.Attributes.push_back(Col);

		Desc.CullMode = Pass.RenderState.CullMode;
		Desc.ColorFormat = ERHIFormat::B8G8R8A8_UNORM;
		Desc.SampleCount = 1;

		FRHIAttachmentBlend Blend;
		if (Pass.RenderState.bBlendEnabled)
		{
			Blend.bBlend = true;
			Blend.SrcColorFactor = Pass.RenderState.SrcBlend;
			Blend.DstColorFactor = Pass.RenderState.DstBlend;
			Blend.SrcAlphaFactor = Pass.RenderState.SrcAlphaBlend;
			Blend.DstAlphaFactor = Pass.RenderState.DstAlphaBlend;
		}
		Desc.AttachmentBlends.push_back(Blend);

		B.Pipeline = RHI->CreateGraphicsPipeline(Desc);
	}

	{
		FRHIDescriptorPoolDesc Desc;
		Desc.MaxSets = 2;
		FRHIDescriptorPoolSize Sz;
		Sz.Type = ERHIDescriptorType::UniformBuffer;
		Sz.Count = 2;
		Desc.PoolSizes.push_back(Sz);
		B.DescPool = RHI->CreateDescriptorPool(Desc);
	}

	B.FrameDescSet  = RHI->AllocateDescriptorSet(B.DescPool, B.FrameSetLayout);
	B.ObjectDescSet = RHI->AllocateDescriptorSet(B.DescPool, B.ObjectSetLayout);

	{
		FRHIDescriptorWrite W{};
		W.Set = B.FrameDescSet;
		W.Binding = 0;
		W.Type = ERHIDescriptorType::UniformBuffer;
		W.Buffer = Ptr->FrameUniformBuf;
		W.Range = sizeof(FFrameUniforms);
		RHI->UpdateDescriptorSets(&W, 1);
	}
	{
		FRHIDescriptorWrite W{};
		W.Set = B.ObjectDescSet;
		W.Binding = 0;
		W.Type = ERHIDescriptorType::UniformBuffer;
		W.Buffer = Ptr->ObjectUniformBuf;
		W.Range = sizeof(FObjectUniforms) * 128;
		RHI->UpdateDescriptorSets(&W, 1);
	}

	return B;
}

void FTriangleBasePassFeature::DestroyBatchResources(FBatchResources& B)
{
	IRHI* RHI = Ptr->RHI;
	if (!RHI) return;

	if (B.DescPool)
	{
		if (B.FrameDescSet)  { RHI->FreeDescriptorSet(B.DescPool, B.FrameDescSet); B.FrameDescSet = nullptr; }
		if (B.ObjectDescSet) { RHI->FreeDescriptorSet(B.DescPool, B.ObjectDescSet); B.ObjectDescSet = nullptr; }
		RHI->DestroyDescriptorPool(B.DescPool);
		B.DescPool = nullptr;
	}
	if (B.ObjectSetLayout) { RHI->DestroyDescriptorSetLayout(B.ObjectSetLayout); B.ObjectSetLayout = nullptr; }
	if (B.FrameSetLayout)  { RHI->DestroyDescriptorSetLayout(B.FrameSetLayout); B.FrameSetLayout = nullptr; }
	if (B.PipelineLayout)  { RHI->DestroyPipelineLayout(B.PipelineLayout); B.PipelineLayout = nullptr; }
	if (B.Pipeline)        { RHI->DestroyGraphicsPipeline(B.Pipeline); B.Pipeline = nullptr; }
	if (B.FragmentShader)  { RHI->DestroyShaderModule(B.FragmentShader); B.FragmentShader = nullptr; }
	if (B.VertexShader)    { RHI->DestroyShaderModule(B.VertexShader); B.VertexShader = nullptr; }
}

} // namespace Maho
