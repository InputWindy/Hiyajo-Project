#pragma once

#include <Render/Sequencer/RenderFeature.h>
#include <Render/RenderPipelineStage.h>
#include <Render/UI/ImGuiSystem.h>
#include <Render/ShaderCompiler.h>

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
class FRHIGraphicsPipeline;
class FRHITexture;
class FRHITextureView;
class FRHIDescriptorPool;
class FRHIDescriptorSet;

/**
 * BasePass feature — fully declarative RDG with dynamic rendering.
 *
 * Shader compilation is lazy: shaders are compiled on first use in
 * BuildRenderGraph, not during registration. This supports real-time
 * shader editing in the editor without restarting the engine.
 */
class FTriangleBasePassFeature final : public TRenderFeatureBase<FTriangleBasePassFeature>,
	public TFeatureDependsPack<
		TFeatureDependsOn<ERenderPipelineStage::BasePass>
	>
{
public:
	FTriangleBasePassFeature();
	~FTriangleBasePassFeature() override;

	bool OnRegister(FRenderServer& RenderServer) override;
	void OnUnregister(FRenderServer& RenderServer) override;
	void ExecuteStage(ERenderPipelineStage Stage, FRDGBuilder& GB) override;

	/** Mark shader dirty so next frame recompiles it (hot-reload from editor). */
	void MarkShaderDirty() { Ptr->bShaderReady = false; }

private:
	/** Per‑hash pass resources (pipeline + descriptor sets — lifetime stable). */
	struct FBatchResources
	{
		FRHIShaderModule* VertexShader = nullptr;
		FRHIShaderModule* FragmentShader = nullptr;
		FRHIDescriptorSetLayout* FrameSetLayout = nullptr;
		FRHIDescriptorSetLayout* ObjectSetLayout = nullptr;
		FRHIPipelineLayout* PipelineLayout = nullptr;
		FRHIGraphicsPipeline* Pipeline = nullptr;
		FRHIDescriptorPool* DescPool = nullptr;
		FRHIDescriptorSet* FrameDescSet = nullptr;
		FRHIDescriptorSet* ObjectDescSet = nullptr;
		const FShaderPassCompiled* PassDesc = nullptr;
	};

	struct FImpl
	{
		bool bInitialized = false;
		bool bShaderReady = false;   // true after first lazy compile + batch creation
		IRHI* RHI = nullptr;
		FRenderServer* RenderServer = nullptr;

		// ── Persistent GPU resources (cross‑frame lifetime) ──
		FRHIBuffer* TriangleVBO = nullptr;
		FRHIBuffer* FrameUniformBuf = nullptr;         // set=0 – CPUToGPU, uploaded each frame
		FRHIBuffer* ObjectUniformBuf = nullptr;        // set=1 – CPUToGPU, uploaded each frame
		FRHITexture* ViewportTex = nullptr;
		FRHITextureView* ViewportTexView = nullptr;
		FImGuiTextureHandle GameViewImGuiTexture;

		std::unique_ptr<FShaderDatabase> ShaderDb;
		std::unordered_map<std::uint64_t, FBatchResources> Batches;

		std::uint32_t VpWidth = 600;
		std::uint32_t VpHeight = 400;
	};

	std::unique_ptr<FImpl> Ptr;

	bool SetupPersistentResources(FRenderServer& RenderServer);
	bool EnsureShaderReady();
	FBatchResources CreateBatchResources(const FShaderPassCompiled& Pass);
	void DestroyBatchResources(FBatchResources& B);
	void DestroyShaderResources();
};

} // namespace Maho
