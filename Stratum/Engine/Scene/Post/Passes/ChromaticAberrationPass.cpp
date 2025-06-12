#include "ChromaticAberrationPass.h"

using namespace ENGINE_NAMESPACE;

Render::ChromaticAberrationPass::ChromaticAberrationPass()
{
	ComputePipelineDesc desc{};

	desc.path = "shaders/compute/compute_chromatic_aberration.cso";
	desc.addBindingItem(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(glm::uvec2)));

	CsChAberration = CreateRef<ComputePipeline>(desc);
}

std::vector<Render::PassDependency> Render::ChromaticAberrationPass::GetDependencies()
{
	return {};
}

void Render::ChromaticAberrationPass::Init(glm::ivec2 Resolution)
{
	ImageDescription desc{};

	desc.Width = Resolution.x;
	desc.Height = Resolution.y;
	desc.AllowComputeResourceUsage = true;
	desc.Format = ImageFormat::R11G11B10_FLOAT;

	Output = CreateRef<ImageResource>(desc);
}

void Render::ChromaticAberrationPass::PreRender(const PostProcessingParameters& parameters)
{
	auto cmd = parameters.cCommandBuffer;

	cmd->RequireTextureState(Output.get(), ResourceState::ShaderResource, ResourceState::UnorderedAccess);

	glm::ivec2 res = parameters.Resolution;

	cmd->SetTextureResource(parameters.pColorSampler, 0);
	cmd->SetTextureCompute(Output.get(), 0);
	cmd->SetComputePipeline(CsChAberration.get());
	cmd->PushConstants(&res, sizeof(glm::uvec2));
	parameters.DispatchScreenSize(cmd, 16, parameters.Resolution);
}

void Render::ChromaticAberrationPass::OnFirstUse(const PostProcessingParameters& parameters)
{
	auto cmd = parameters.cCommandBuffer;
	cmd->RequireTextureState(Output.get(), ResourceState::UnorderedAccess, ResourceState::ShaderResource);
}
