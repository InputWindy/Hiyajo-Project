#pragma once

#include <Render/Sequencer/RenderFeature.h>
#include <Render/RenderPipelineStage.h>
#include <Render/UI/ImGuiSystem.h>

#include <cstdint>
#include <memory>

namespace Maho
{

class FRenderServer;
class IRHI;
class FShaderCache;
class FShaderLoader;

class FRHIBuffer;
class FRHIShaderModule;
class FRHIDescriptorSetLayout;
class FRHIPipelineLayout;
class FRHIRenderPass;
class FRHIFramebuffer;
class FRHIGraphicsPipeline;
class FRHITexture;
class FRHITextureView;
class FRHIDescriptorPool;
class FRHIDescriptorSet;

/**
 * BasePass feature that draws ColoredTriangle scene primitives.
 * GPU resources stay here; transforms come from FSceneUpdatePacket (World actors).
 *
 * Must use TRenderFeatureBase<Self> (not FRenderFeature) so ParticipatesInStage
 * resolves this type's FDependsPack / TFeatureDependsPack inheritance.
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
	void BuildRenderGraph(FRDGBuilder& GB, FRenderServer& Server) override;

private:
	struct FImpl
	{
		bool bInitialized = false;
		IRHI* RHI = nullptr;

		FRHIShaderModule* VertexShader = nullptr;
		FRHIShaderModule* FragmentShader = nullptr;
		FRHIDescriptorSetLayout* FrameSetLayout = nullptr;
		FRHIDescriptorSetLayout* ObjectSetLayout = nullptr;
		FRHIPipelineLayout* PipelineLayout = nullptr;
		FRHIRenderPass* OffscreenPass = nullptr;
		FRHIFramebuffer* OffscreenFB = nullptr;
		FRHIGraphicsPipeline* Pipeline = nullptr;
		FRHIBuffer* TriangleVBO = nullptr;
		FRHIBuffer* FrameUniformBuf = nullptr;
		FRHIBuffer* ObjectUniformBuf = nullptr;
		FRHITexture* ViewportTex = nullptr;
		FRHITextureView* ViewportTexView = nullptr;
		FRHIDescriptorPool* DescPool = nullptr;
		FRHIDescriptorSet* FrameDescSet = nullptr;
		FRHIDescriptorSet* ObjectDescSet = nullptr;
		FImGuiTextureHandle GameViewImGuiTexture;
		bool bViewportShaderResource = false;

		std::unique_ptr<FShaderCache> ShaderCache;
		std::unique_ptr<FShaderLoader> ShaderLoader;

		std::uint32_t VpWidth = 600;
		std::uint32_t VpHeight = 400;
	};

	std::unique_ptr<FImpl> Ptr;

	bool Initialize(FRenderServer& RenderServer);
};

void RegisterTriangleBasePassFeature(FRenderServer& Server);

} // namespace Maho
