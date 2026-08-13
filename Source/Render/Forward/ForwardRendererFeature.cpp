#include "Render/Forward/ForwardRendererFeature.h"
#include "Render/MahoCommonUniforms.h"

#include <Core/Application/App.h>
#include <Core/Engine.h>
#include <Core/System/Log.h>
#include <Render/RDG/RDGBuilder.h>
#include <Render/RenderServer.h>
#include <Render/SceneUpdatePacket.h>
#include <Render/ShaderCompiler.h>

#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

namespace Maho
{

namespace
{

/** 24 cube vertices (pos + color), 36 indices. */
static const FGPUCubeVertex CubeVertices[] =
{
	// +X face
	{ { 0.5f, -0.5f, -0.5f }, { 1.f, 0.f, 0.f } },
	{ { 0.5f,  0.5f, -0.5f }, { 1.f, 0.f, 0.f } },
	{ { 0.5f,  0.5f,  0.5f }, { 1.f, 0.f, 0.f } },
	{ { 0.5f, -0.5f,  0.5f }, { 1.f, 0.f, 0.f } },
	// -X face
	{ { -0.5f, -0.5f,  0.5f }, { 0.f, 1.f, 0.f } },
	{ { -0.5f,  0.5f,  0.5f }, { 0.f, 1.f, 0.f } },
	{ { -0.5f,  0.5f, -0.5f }, { 0.f, 1.f, 0.f } },
	{ { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f, 0.f } },
	// +Y face
	{ { -0.5f,  0.5f, -0.5f }, { 0.f, 0.f, 1.f } },
	{ { -0.5f,  0.5f,  0.5f }, { 0.f, 0.f, 1.f } },
	{ {  0.5f,  0.5f,  0.5f }, { 0.f, 0.f, 1.f } },
	{ {  0.5f,  0.5f, -0.5f }, { 0.f, 0.f, 1.f } },
	// -Y face
	{ {  0.5f, -0.5f,  0.5f }, { 1.f, 1.f, 0.f } },
	{ {  0.5f, -0.5f, -0.5f }, { 1.f, 1.f, 0.f } },
	{ { -0.5f, -0.5f, -0.5f }, { 1.f, 1.f, 0.f } },
	{ { -0.5f, -0.5f,  0.5f }, { 1.f, 1.f, 0.f } },
	// +Z face
	{ {  0.5f,  0.5f,  0.5f }, { 0.f, 1.f, 1.f } },
	{ { -0.5f,  0.5f,  0.5f }, { 0.f, 1.f, 1.f } },
	{ { -0.5f, -0.5f,  0.5f }, { 0.f, 1.f, 1.f } },
	{ {  0.5f, -0.5f,  0.5f }, { 0.f, 1.f, 1.f } },
	// -Z face
	{ { -0.5f,  0.5f, -0.5f }, { 1.f, 0.f, 1.f } },
	{ {  0.5f,  0.5f, -0.5f }, { 1.f, 0.f, 1.f } },
	{ {  0.5f, -0.5f, -0.5f }, { 1.f, 0.f, 1.f } },
	{ { -0.5f, -0.5f, -0.5f }, { 1.f, 0.f, 1.f } },
};

static const std::uint32_t CubeIndices[] =
{
	0,1,2, 0,2,3,    // +X
	4,5,6, 4,6,7,    // -X
	8,9,10, 8,10,11, // +Y
	12,13,14, 12,14,15, // -Y
	16,17,18, 16,18,19, // +Z
	20,21,22, 20,22,23, // -Z
};

static std::string LoadTextFile(const std::string& Path)
{
	std::ifstream In(Path, std::ios::binary);
	if (!In) return {};
	std::ostringstream Ss;
	Ss << In.rdbuf();
	return Ss.str();
}

/** Gribb-Hartmann frustum plane extraction from ViewProj. */
static void ExtractFrustumPlanes(const float ViewProj[16], float OutPlanes[6][4])
{
	// ViewProj is column-major (M[col*4+row]).
	auto Row = [&](int R) { return std::array<float,4>{ ViewProj[R], ViewProj[4+R], ViewProj[8+R], ViewProj[12+R] }; };
	std::array<float,4> R0 = Row(0), R1 = Row(1), R2 = Row(2), R3 = Row(3);
	auto Normalize = [](std::array<float,4>& P) {
		float L = std::sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);
		if (L > 1e-8f) { P[0]/=L; P[1]/=L; P[2]/=L; P[3]/=L; }
	};
	auto Add = [](const std::array<float,4>& A, const std::array<float,4>& B) {
		return std::array<float,4>{ A[0]+B[0], A[1]+B[1], A[2]+B[2], A[3]+B[3] };
	};
	auto Sub = [](const std::array<float,4>& A, const std::array<float,4>& B) {
		return std::array<float,4>{ A[0]-B[0], A[1]-B[1], A[2]-B[2], A[3]-B[3] };
	};

	std::array<float,4> P[6] = { Sub(R3,R0), Add(R3,R0), Sub(R3,R1), Add(R3,R1), Sub(R3,R2), Add(R3,R2) };
	for (int I = 0; I < 6; ++I)
	{
		Normalize(P[I]);
		OutPlanes[I][0] = P[I][0];
		OutPlanes[I][1] = P[I][1];
		OutPlanes[I][2] = P[I][2];
		OutPlanes[I][3] = P[I][3];
	}
}

} // namespace

FForwardRendererFeature::FForwardRendererFeature()
	: TRenderFeatureBase("ForwardRenderer")
	, Ptr(std::make_unique<FImpl>())
{
}

FForwardRendererFeature::~FForwardRendererFeature() = default;

bool FForwardRendererFeature::OnRegister(FRenderServer& RenderServer)
{
	if (!SetupPersistentResources(RenderServer))
	{
		MAHO_CORE_ERROR("ForwardRenderer: persistent resource setup failed");
		return false;
	}
	MAHO_CORE_INFO("ForwardRenderer: registered (shader lazy)");
	return true;
}

void FForwardRendererFeature::OnUnregister(FRenderServer& RenderServer)
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
	if (S.CubeIBO)           { RHI->DestroyBuffer(S.CubeIBO); S.CubeIBO = nullptr; }
	if (S.CubeVBO)           { RHI->DestroyBuffer(S.CubeVBO); S.CubeVBO = nullptr; }
	if (S.IndirectArgsBuf)   { RHI->DestroyBuffer(S.IndirectArgsBuf); S.IndirectArgsBuf = nullptr; }
	if (S.SceneInstanceBuf)  { RHI->DestroyBuffer(S.SceneInstanceBuf); S.SceneInstanceBuf = nullptr; }

	S.bInitialized = false;
}

void FForwardRendererFeature::BuildCubeGeometry()
{
	auto& S = *Ptr;
	IRHI* RHI = S.RHI;

	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(CubeVertices);
		Desc.Usage = ERHIBufferUsage::Vertex;
		Desc.MemoryUsage = ERHIMemoryUsage::GPUOnly;
		S.CubeVBO = RHI->CreateBuffer(Desc);
	}
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(CubeIndices);
		Desc.Usage = ERHIBufferUsage::Index;
		Desc.MemoryUsage = ERHIMemoryUsage::GPUOnly;
		S.CubeIBO = RHI->CreateBuffer(Desc);
		S.CubeIndexCount = static_cast<std::uint32_t>(sizeof(CubeIndices) / sizeof(std::uint32_t));
	}

	// Staging upload.
	{
		FRHIBufferDesc StageDesc;
		StageDesc.Size = sizeof(CubeVertices) + sizeof(CubeIndices);
		StageDesc.Usage = ERHIBufferUsage::TransferSrc;
		StageDesc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		FRHIBuffer* Staging = RHI->CreateBuffer(StageDesc);
		if (Staging)
		{
			std::vector<std::uint8_t> Data(StageDesc.Size);
			std::memcpy(Data.data(), CubeVertices, sizeof(CubeVertices));
			std::memcpy(Data.data() + sizeof(CubeVertices), CubeIndices, sizeof(CubeIndices));
			RHI->UpdateBuffer(Staging, 0, Data.size(), Data.data());

			FRHICommandList* Cmd = RHI->CreateCommandList(ERHICommandListType::Graphics);
			Cmd->Begin();
			Cmd->CopyBuffer(Staging, 0, S.CubeVBO, 0, sizeof(CubeVertices));
			Cmd->CopyBuffer(Staging, sizeof(CubeVertices), S.CubeIBO, 0, sizeof(CubeIndices));
			Cmd->End();
			RHI->GetGraphicsQueue().Submit(&Cmd, 1, nullptr, 0, nullptr, 0, nullptr);
			RHI->DestroyCommandList(Cmd);
			RHI->DestroyBuffer(Staging);
		}
	}
}

bool FForwardRendererFeature::SetupPersistentResources(FRenderServer& RenderServer)
{
	auto& S = *Ptr;
	if (S.bInitialized) return true;

	S.RHI = RenderServer.GetRHIServer().GetRHI();
	S.RenderServer = &RenderServer;
	if (!S.RHI) return false;

	// ── GPU scene buffers ──
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(FGPUSceneInstance) * GPUSceneMaxInstances;
		Desc.Usage = ERHIBufferUsage::Storage;
		Desc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		S.SceneInstanceBuf = S.RHI->CreateBuffer(Desc);
	}
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(FDrawIndexedIndirectArgs) * GPUSceneMaxDraws;
		Desc.Usage = ERHIBufferUsage::Storage | ERHIBufferUsage::Indirect;
		Desc.MemoryUsage = ERHIMemoryUsage::GPUOnly;
		S.IndirectArgsBuf = S.RHI->CreateBuffer(Desc);
	}

	BuildCubeGeometry();

	// ── Frame UBO ──
	{
		FRHIBufferDesc Desc;
		Desc.Size = sizeof(FFrameUniforms);
		Desc.Usage = ERHIBufferUsage::Uniform;
		Desc.MemoryUsage = ERHIMemoryUsage::CPUToGPU;
		S.FrameUniformBuf = S.RHI->CreateBuffer(Desc);
	}

	// ── Viewport texture ──
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
	MAHO_CORE_INFO("ForwardRenderer: persistent resources ready ({}x{})", S.VpWidth, S.VpHeight);
	return true;
}

bool FForwardRendererFeature::EnsureShaderReady()
{
	auto& S = *Ptr;
	if (S.bShaderReady) return true;
	if (!S.RHI || !GApp) return false;

	if (!FShaderCompiler::Initialize())
	{
		MAHO_CORE_ERROR("ForwardRenderer: shader compiler init failed");
		return false;
	}

	DestroyShaderResources();

	const FConfig& Config = GApp->GetConfig();

	// ── Compile compute (culling) shader ──
	const std::string CullSrc = LoadTextFile(Config.ProjectShadersDir + "/Forward/ForwardCulling.comp");
	if (CullSrc.empty())
	{
		MAHO_CORE_ERROR("ForwardRenderer: missing Forward/ForwardCulling.comp");
		return false;
	}

	FShaderCompileDesc CullDesc;
	CullDesc.Source = CullSrc;
	FShaderCompileResult CullResult = FShaderCompiler::CompileStage(CullDesc, ERHIShaderStage::Compute, "main");
	if (!CullResult.bSuccess)
	{
		MAHO_CORE_ERROR("ForwardRenderer: cull shader compile failed: {}", CullResult.ErrorLog);
		return false;
	}

	// ── Compile vertex shader ──
	const std::string VertSrc = LoadTextFile(Config.ProjectShadersDir + "/Forward/Forward.vert");
	if (VertSrc.empty())
	{
		MAHO_CORE_ERROR("ForwardRenderer: missing Forward/Forward.vert");
		return false;
	}
	FShaderCompileDesc VertDesc;
	VertDesc.Source = VertSrc;
	FShaderCompileResult VertResult = FShaderCompiler::CompileStage(VertDesc, ERHIShaderStage::Vertex, "main");
	if (!VertResult.bSuccess)
	{
		MAHO_CORE_ERROR("ForwardRenderer: vert shader compile failed: {}", VertResult.ErrorLog);
		return false;
	}

	// ── Compile fragment shader ──
	const std::string FragSrc = LoadTextFile(Config.ProjectShadersDir + "/Forward/Forward.frag");
	if (FragSrc.empty())
	{
		MAHO_CORE_ERROR("ForwardRenderer: missing Forward/Forward.frag");
		return false;
	}
	FShaderCompileDesc FragDesc;
	FragDesc.Source = FragSrc;
	FShaderCompileResult FragResult = FShaderCompiler::CompileStage(FragDesc, ERHIShaderStage::Fragment, "main");
	if (!FragResult.bSuccess)
	{
		MAHO_CORE_ERROR("ForwardRenderer: frag shader compile failed: {}", FragResult.ErrorLog);
		return false;
	}

	IRHI* RHI = S.RHI;

	// ── Shader modules ──
	FRHIShaderModuleDesc CsDesc;
	CsDesc.Stage = ERHIShaderStage::Compute;
	CsDesc.Bytecode = CullResult.Bytecode.data();
	CsDesc.BytecodeSize = CullResult.Bytecode.size() * sizeof(std::uint32_t);
	FRHIShaderModule* CsModule = RHI->CreateShaderModule(CsDesc);

	FRHIShaderModuleDesc VsDesc;
	VsDesc.Stage = ERHIShaderStage::Vertex;
	VsDesc.Bytecode = VertResult.Bytecode.data();
	VsDesc.BytecodeSize = VertResult.Bytecode.size() * sizeof(std::uint32_t);
	FRHIShaderModule* VsModule = RHI->CreateShaderModule(VsDesc);

	FRHIShaderModuleDesc FsDesc;
	FsDesc.Stage = ERHIShaderStage::Fragment;
	FsDesc.Bytecode = FragResult.Bytecode.data();
	FsDesc.BytecodeSize = FragResult.Bytecode.size() * sizeof(std::uint32_t);
	FRHIShaderModule* FsModule = RHI->CreateShaderModule(FsDesc);

	// ── Descriptor set layouts ──
	// Cull set: [0]=scene SSBO(read), [1]=indirect SSBO(write)
	FRHIDescriptorSetLayout* CullSetLayout;
	{
		FRHIDescriptorSetLayoutDesc Desc;
		FRHIDescriptorBinding B0;
		B0.Binding = 0; B0.Type = ERHIDescriptorType::StorageBuffer; B0.Count = 1; B0.Stages = ERHIShaderStage::Compute;
		Desc.Bindings.push_back(B0);
		FRHIDescriptorBinding B1;
		B1.Binding = 1; B1.Type = ERHIDescriptorType::StorageBuffer; B1.Count = 1; B1.Stages = ERHIShaderStage::Compute;
		Desc.Bindings.push_back(B1);
		CullSetLayout = RHI->CreateDescriptorSetLayout(Desc);
	}

	// Draw set: [0]=scene SSBO(read) + frame UBO in a second set
	FRHIDescriptorSetLayout* DrawSceneSetLayout;
	FRHIDescriptorSetLayout* FrameSetLayout;
	{
		FRHIDescriptorSetLayoutDesc Desc;
		FRHIDescriptorBinding B0;
		B0.Binding = 0; B0.Type = ERHIDescriptorType::StorageBuffer; B0.Count = 1; B0.Stages = ERHIShaderStage::Vertex;
		Desc.Bindings.push_back(B0);
		DrawSceneSetLayout = RHI->CreateDescriptorSetLayout(Desc);
	}
	{
		FRHIDescriptorSetLayoutDesc Desc;
		FRHIDescriptorBinding B0;
		B0.Binding = 0; B0.Type = ERHIDescriptorType::UniformBuffer; B0.Count = 1; B0.Stages = ERHIShaderStage::AllGraphics;
		Desc.Bindings.push_back(B0);
		FrameSetLayout = RHI->CreateDescriptorSetLayout(Desc);
	}

	// ── Pipeline layouts ──
	{
		FRHIPipelineLayoutDesc Desc;
		Desc.SetLayouts = { CullSetLayout };
		FRHIPushConstantRange PC;
		PC.Stages = ERHIShaderStage::Compute;
		PC.Offset = 0;
		PC.Size = sizeof(FGPUCullParams);
		Desc.PushConstants.push_back(PC);
		S.CullLayout = RHI->CreatePipelineLayout(Desc);
	}
	{
		FRHIPipelineLayoutDesc Desc;
		Desc.SetLayouts = { FrameSetLayout, DrawSceneSetLayout };
		S.DrawLayout = RHI->CreatePipelineLayout(Desc);
	}

	// ── Compute pipeline (culling) ──
	{
		FRHIComputePipelineDesc Desc;
		Desc.ComputeShader = CsModule;
		Desc.Layout = S.CullLayout;
		S.CullPipeline = RHI->CreateComputePipeline(Desc);
	}

	// ── Graphics pipeline (forward) ──
	{
		FRHIGraphicsPipelineDesc Desc;
		Desc.VertexShader = VsModule;
		Desc.FragmentShader = FsModule;
		Desc.Layout = S.DrawLayout;
		Desc.Topology = ERHIPrimitiveTopology::TriangleList;
		Desc.VertexStride = sizeof(FGPUCubeVertex);
		Desc.CullMode = ERHICullMode::Back;

		FRHIVertexAttribute Pos;
		Pos.Location = 0; Pos.Format = ERHIFormat::R32G32B32_SFLOAT; Pos.Offset = 0;
		Desc.Attributes.push_back(Pos);
		FRHIVertexAttribute Col;
		Col.Location = 5; Col.Format = ERHIFormat::R32G32B32_SFLOAT; Col.Offset = 12;
		Desc.Attributes.push_back(Col);

		Desc.ColorFormat = ERHIFormat::B8G8R8A8_UNORM;
		Desc.SampleCount = 1;
		S.DrawPipeline = RHI->CreateGraphicsPipeline(Desc);
	}

	// ── Descriptor pool + sets ──
	{
		FRHIDescriptorPoolDesc Desc;
		Desc.MaxSets = 3;
		FRHIDescriptorPoolSize Sz1; Sz1.Type = ERHIDescriptorType::StorageBuffer; Sz1.Count = 3;
		Desc.PoolSizes.push_back(Sz1);
		FRHIDescriptorPoolSize Sz2; Sz2.Type = ERHIDescriptorType::UniformBuffer; Sz2.Count = 1;
		Desc.PoolSizes.push_back(Sz2);
		S.DescPool = RHI->CreateDescriptorPool(Desc);
	}
	S.CullDescSet = RHI->AllocateDescriptorSet(S.DescPool, CullSetLayout);
	S.DrawDescSet = RHI->AllocateDescriptorSet(S.DescPool, DrawSceneSetLayout);
	S.FrameDescSet = RHI->AllocateDescriptorSet(S.DescPool, FrameSetLayout);

	{
		FRHIDescriptorWrite W{};
		W.Set = S.CullDescSet; W.Binding = 0; W.Type = ERHIDescriptorType::StorageBuffer;
		W.Buffer = S.SceneInstanceBuf; W.Range = sizeof(FGPUSceneInstance) * GPUSceneMaxInstances;
		RHI->UpdateDescriptorSets(&W, 1);
		FRHIDescriptorWrite W1{};
		W1.Set = S.CullDescSet; W1.Binding = 1; W1.Type = ERHIDescriptorType::StorageBuffer;
		W1.Buffer = S.IndirectArgsBuf; W1.Range = sizeof(FDrawIndexedIndirectArgs) * GPUSceneMaxDraws;
		RHI->UpdateDescriptorSets(&W1, 1);
	}
	{
		FRHIDescriptorWrite W{};
		W.Set = S.DrawDescSet; W.Binding = 0; W.Type = ERHIDescriptorType::StorageBuffer;
		W.Buffer = S.SceneInstanceBuf; W.Range = sizeof(FGPUSceneInstance) * GPUSceneMaxInstances;
		RHI->UpdateDescriptorSets(&W, 1);
	}
	{
		FRHIDescriptorWrite W{};
		W.Set = S.FrameDescSet; W.Binding = 0; W.Type = ERHIDescriptorType::UniformBuffer;
		W.Buffer = S.FrameUniformBuf; W.Range = sizeof(FFrameUniforms);
		RHI->UpdateDescriptorSets(&W, 1);
	}

	S.bShaderReady = true;
	MAHO_CORE_INFO("ForwardRenderer: shaders compiled + pipelines ready");
	return true;
}

void FForwardRendererFeature::DestroyShaderResources()
{
	auto& S = *Ptr;
	IRHI* RHI = S.RHI;
	if (!RHI) return;

	if (S.DescPool)      { RHI->DestroyDescriptorPool(S.DescPool); S.DescPool = nullptr; }
	S.CullDescSet = nullptr; S.DrawDescSet = nullptr; S.FrameDescSet = nullptr;
	if (S.DrawPipeline)  { RHI->DestroyGraphicsPipeline(S.DrawPipeline); S.DrawPipeline = nullptr; }
	if (S.CullPipeline)  { RHI->DestroyComputePipeline(S.CullPipeline); S.CullPipeline = nullptr; }
	if (S.DrawLayout)    { RHI->DestroyPipelineLayout(S.DrawLayout); S.DrawLayout = nullptr; }
	if (S.CullLayout)    { RHI->DestroyPipelineLayout(S.CullLayout); S.CullLayout = nullptr; }
	S.bShaderReady = false;
}

void FForwardRendererFeature::ComputeFrustumPlanes(const FCameraFrameData& Camera, float Aspect, FGPUCullParams& OutParams)
{
	float FOV = Camera.FOV;
	float Near = Camera.NearPlane;
	float Far = Camera.FarPlane;
	float F = 1.0f / std::tan((FOV * 0.5f) * 3.14159265f / 180.0f);
	float Proj[16] = {
		F / Aspect, 0, 0, 0,
		0, F, 0, 0,
		0, 0, Far / (Far - Near), 1,
		0, 0, -(Near * Far) / (Far - Near), 0,
	};
	float ViewProj[16];
	for (int Col = 0; Col < 4; ++Col)
		for (int Row = 0; Row < 4; ++Row)
		{
			float Sum = 0.0f;
			for (int K = 0; K < 4; ++K)
				Sum += Proj[K * 4 + Row] * Camera.View[Col * 4 + K];
			ViewProj[Col * 4 + Row] = Sum;
		}
	ExtractFrustumPlanes(ViewProj, OutParams.FrustumPlanes);
}

void FForwardRendererFeature::ExecuteStage(ERenderPipelineStage Stage, FRDGBuilder& GB)
{
	if (!Ptr->bInitialized) return;

	switch (Stage)
	{
	case ERenderPipelineStage::BeginFrame: ExecuteBeginFrame(GB); break;
	case ERenderPipelineStage::BasePass:   ExecuteBasePass(GB);   break;
	default: break;
	}
}

void FForwardRendererFeature::ExecuteBeginFrame(FRDGBuilder& GB)
{
	auto& S = *Ptr;
	if (!S.RenderServer) return;
	if (!EnsureShaderReady()) return;

	const FSceneUpdatePacket& Scene = S.RenderServer->GetCurrentScene();
	if (Scene.Draws.empty()) return;

	const std::uint32_t NumInstances = static_cast<std::uint32_t>(Scene.Draws.size());
	if (NumInstances > GPUSceneMaxInstances) return;

	// ── Build GPU scene instances ──
	std::vector<FGPUSceneInstance> Instances(NumInstances);
	for (std::uint32_t I = 0; I < NumInstances; ++I)
	{
		const FSceneDrawItem& Item = Scene.Draws[I];
		std::memcpy(Instances[I].LocalToWorld, Item.LocalToWorld, sizeof(Item.LocalToWorld));
		Instances[I].Color[0] = 0.8f;
		Instances[I].Color[1] = 0.6f;
		Instances[I].Color[2] = 0.3f;
		Instances[I].Color[3] = 1.0f;
	}

	// ── Empty indirect commands (InstanceCount=0) — invisible draws are no-ops ──
	std::vector<FDrawIndexedIndirectArgs> Empty(GPUSceneMaxDraws);
	for (auto& E : Empty)
	{
		E.IndexCount = S.CubeIndexCount;
		E.InstanceCount = 0;
		E.FirstIndex = 0;
		E.VertexOffset = 0;
		E.FirstInstance = 0;
	}

	// ── Frame uniforms ──
	FFrameUniforms FrameUni{};
	std::memcpy(FrameUni.View, Scene.Camera.View, sizeof(FrameUni.View));
	{
		float F = 1.0f / std::tan((Scene.Camera.FOV * 0.5f) * 3.14159265f / 180.0f);
		float A = Scene.Camera.AspectRatio, N = Scene.Camera.NearPlane, Ff = Scene.Camera.FarPlane;
		float P[16] = { F/A, 0, 0, 0,  0, F, 0, 0,  0, 0, Ff/(Ff-N), 1,  0, 0, -(N*Ff)/(Ff-N), 0 };
		std::memcpy(FrameUni.Proj, P, sizeof(P));
		for (int Col = 0; Col < 4; ++Col)
			for (int Row = 0; Row < 4; ++Row)
			{
				float Sum = 0.0f;
				for (int K = 0; K < 4; ++K)
					Sum += FrameUni.Proj[K*4+Row] * FrameUni.View[Col*4+K];
				FrameUni.ViewProj[Col*4+Row] = Sum;
			}
	}

	// ── Declarative uploads (staging + copy passes, barriers auto-derived) ──
	FRDGBuffer* RDGScene = GB.RegisterExternalBuffer(S.SceneInstanceBuf, ERHIResourceState::Common, "GPUSceneInstances");
	FRDGBuffer* RDGIndirect = GB.RegisterExternalBuffer(S.IndirectArgsBuf, ERHIResourceState::Common, "IndirectArgs");
	FRDGBuffer* RDGFrameUBO = GB.RegisterExternalBuffer(S.FrameUniformBuf, ERHIResourceState::Common, "FrameUBO");
	GB.UploadBuffer(RDGScene, Instances.data(), Instances.size() * sizeof(FGPUSceneInstance));
	GB.UploadBuffer(RDGIndirect, Empty.data(), Empty.size() * sizeof(FDrawIndexedIndirectArgs));
	GB.UploadBuffer(RDGFrameUBO, &FrameUni, sizeof(FrameUni));
}

void FForwardRendererFeature::ExecuteBasePass(FRDGBuilder& GB)
{
	auto& S = *Ptr;
	if (!S.RenderServer) return;
	if (!EnsureShaderReady()) return;

	const FSceneUpdatePacket& Scene = S.RenderServer->GetCurrentScene();
	if (Scene.Draws.empty()) return;

	// ── Cull params (push constants) ──
	FGPUCullParams CullParams{};
	ComputeFrustumPlanes(Scene.Camera, Scene.Camera.AspectRatio, CullParams);
	CullParams.InstanceCount = static_cast<std::uint32_t>(Scene.Draws.size());

	// ── RDG resources ──
	FRDGBuffer* RDGScene = GB.RegisterExternalBuffer(S.SceneInstanceBuf, ERHIResourceState::Common, "GPUSceneInstances");
	FRDGBuffer* RDGIndirect = GB.RegisterExternalBuffer(S.IndirectArgsBuf, ERHIResourceState::Common, "IndirectArgs");
	FRDGBuffer* RDGFrameUBO = GB.RegisterExternalBuffer(S.FrameUniformBuf, ERHIResourceState::Common, "FrameUBO");
	FRDGTexture* RDGViewport = GB.RegisterExternalTexture(S.ViewportTex, ERHIResourceState::Common, "ViewportTex");

	// ── Compute pass: culling → indirect args ──
	{
		auto& Params = GB.AllocateParameters();
		Params.Reads = { { RDGScene, ERHIResourceState::UnorderedAccess } };
		Params.Writes = { { RDGIndirect, ERHIResourceState::UnorderedAccess } };
		GB.AddComputePass("ForwardCull", Params,
			[&S, CullParams, NumInstances = CullParams.InstanceCount](FRHICommandList& Cmd) mutable
			{
				Cmd.BindComputePipeline(S.CullPipeline);
				FRHIDescriptorSet* Sets[] = { S.CullDescSet };
				Cmd.BindDescriptorSets(0, Sets, 1);
				Cmd.PushConstants(ERHIShaderStage::Compute, 0, sizeof(CullParams), &CullParams);
				std::uint32_t Groups = (NumInstances + 63) / 64;
				Cmd.Dispatch(Groups, 1, 1);
			});
	}

	// ── Raster pass: indirect draw ──
	{
		auto& Params = GB.AllocateParameters();
		Params.Reads = {
			{ RDGIndirect, ERHIResourceState::UnorderedAccess },
			{ RDGScene, ERHIResourceState::UnorderedAccess },
			{ RDGFrameUBO, ERHIResourceState::UniformBuffer },
		};
		Params.Writes = { { RDGViewport, ERHIResourceState::RenderTarget } };
		FRDGPassParameters::FRenderTargetBinding RT{};
		RT.Texture = RDGViewport;
		RT.View = S.ViewportTexView;
		RT.LoadOp = ERHILoadOp::Clear;
		RT.StoreOp = ERHIStoreOp::Store;
		RT.ClearColor[0] = 0.1f;
		RT.ClearColor[1] = 0.12f;
		RT.ClearColor[2] = 0.16f;
		RT.ClearColor[3] = 1.0f;
		Params.RenderTargets = { RT };

		GB.AddRasterPass("ForwardDraw", Params,
			[&S, MaxDraws = GPUSceneMaxDraws](FRHICommandList& Cmd) mutable
			{
				Cmd.BindGraphicsPipeline(S.DrawPipeline);
				FRHIDescriptorSet* Sets[] = { S.FrameDescSet, S.DrawDescSet };
				Cmd.BindDescriptorSets(0, Sets, 2);
				Cmd.BindVertexBuffer(0, S.CubeVBO);
				Cmd.BindIndexBuffer(S.CubeIBO, 0, true);
				Cmd.SetViewport(0.0f, 0.0f, static_cast<float>(S.VpWidth), static_cast<float>(S.VpHeight));
				Cmd.SetScissor(0, 0, S.VpWidth, S.VpHeight);
				Cmd.DrawIndexedIndirect(S.IndirectArgsBuf, 0, MaxDraws, sizeof(FDrawIndexedIndirectArgs));
			});
	}
}

} // namespace Maho
