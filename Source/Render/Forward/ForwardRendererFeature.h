#pragma once

#include <Render/Sequencer/RenderFeature.h>
#include <Render/RenderPipelineStage.h>
#include <Render/UI/ImGuiSystem.h>
#include <Render/ShaderCompiler.h>

#include "GPUScene.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Maho
{

class FRenderServer;
class IRHI;
class FShaderDatabase;
struct FShaderPassCompiled;

class FRHIBuffer;
class FRHIShaderModule;
class FRHIDescriptorSetLayout;
class FRHIPipelineLayout;
class FRHIComputePipeline;
class FRHIGraphicsPipeline;
class FRHITexture;
class FRHITextureView;
class FRHIDescriptorPool;
class FRHIDescriptorSet;

/**
 * GPU-driven forward renderer.
 *
 * Frame flow (BasePass stage):
 *   1. Upload scene instances to a storage buffer (CPU → GPU).
 *   2. Fill the indirect args buffer with empty (InstanceCount=0) commands.
 *   3. Compute pass: frustum-cull instances, write visible draw commands.
 *   4. Raster pass: vkCmdDrawIndexedIndirect draws visible instances.
 *   5. Vertex shader reads per-instance LocalToWorld / Color from the scene SSBO.
 */
class FForwardRendererFeature final : public TRenderFeatureBase<FForwardRendererFeature>,
	public TFeatureDependsPack<
			TFeatureDependsOn<ERenderPipelineStage::BasePass>
	>
{
public:
	FForwardRendererFeature();
	~FForwardRendererFeature() override;

	bool OnRegister(FRenderServer& RenderServer) override;
	void OnUnregister(FRenderServer& RenderServer) override;
	void ExecuteStage(ERenderPipelineStage Stage, FRDGBuilder& GB) override;

	/** Mark shader dirty so next frame recompiles (hot-reload from editor). */
	void MarkShaderDirty() { Ptr->bShaderReady = false; }

private:
	struct FImpl
	{
		bool bInitialized = false;
		bool bShaderReady = false;
		IRHI* RHI = nullptr;
		FRenderServer* RenderServer = nullptr;

		// ── GPU Scene ──
		FRHIBuffer* SceneInstanceBuf = nullptr;   // storage, CPUToGPU upload each frame
		FRHIBuffer* IndirectArgsBuf = nullptr;    // storage + indirect, compute writes
		FRHIBuffer* CubeVBO = nullptr;            // static unit-cube vertices
		FRHIBuffer* CubeIBO = nullptr;            // static unit-cube indices
		FRHIBuffer* FrameUniformBuf = nullptr;    // view/proj UBO

		std::uint32_t CubeIndexCount = 0;

		// ── Viewport ──
		FRHITexture* ViewportTex = nullptr;
		FRHITextureView* ViewportTexView = nullptr;
		FImGuiTextureHandle GameViewImGuiTexture;

		// ── Pipelines + descriptors ──
		std::unique_ptr<FShaderDatabase> ShaderDb;
		std::unordered_map<std::uint64_t, struct FBatchResources*> Batches;
		FRHIComputePipeline* CullPipeline = nullptr;
		FRHIGraphicsPipeline* DrawPipeline = nullptr;
		FRHIDescriptorSetLayout* SceneSetLayout = nullptr;
		FRHIDescriptorSetLayout* FrameSetLayout = nullptr;
		FRHIPipelineLayout* CullLayout = nullptr;
		FRHIPipelineLayout* DrawLayout = nullptr;
		FRHIDescriptorPool* DescPool = nullptr;
		FRHIDescriptorSet* CullDescSet = nullptr;   // scene SSBO + indirect SSBO
		FRHIDescriptorSet* DrawDescSet = nullptr;   // scene SSBO
		FRHIDescriptorSet* FrameDescSet = nullptr;  // frame UBO

		std::uint32_t VpWidth = 600;
		std::uint32_t VpHeight = 400;
	};

	std::unique_ptr<FImpl> Ptr;

	bool SetupPersistentResources(FRenderServer& RenderServer);
	bool EnsureShaderReady();
	void DestroyShaderResources();
	void BuildCubeGeometry();
	void ComputeFrustumPlanes(const struct FCameraFrameData& Camera, float Aspect, FGPUCullParams& OutParams);
};

} // namespace Maho
