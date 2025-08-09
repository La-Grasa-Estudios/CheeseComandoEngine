#include "PostProcessingPass.h"

using namespace ENGINE_NAMESPACE;

std::vector<Render::PassDependency> Render::PostProcessingPass::GetDependencies()
{
    return std::vector<PassDependency>();
}

void Render::PostProcessingPass::GetOutputs(std::vector<Ref<ImageResource>>& outputs, std::vector<std::string>& names)
{
}

void Render::PostProcessingPass::SetInput(Ref<ImageResource> input, const std::string& name)
{
}

void Render::PostProcessingPass::Init(glm::ivec2 Resolution)
{
}

void Render::PostProcessingPass::PreRender(const PostProcessingParameters& parameters)
{
}

void Render::PostProcessingPass::Render(const PostProcessingParameters& parameters)
{
}

void Render::PostProcessingPass::OnFirstUse(const PostProcessingParameters& parameters)
{
}
