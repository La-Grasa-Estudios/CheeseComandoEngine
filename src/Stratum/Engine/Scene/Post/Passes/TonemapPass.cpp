#include "TonemapPass.h"
#include "Outputs.h"

#include "Asset/TextureLoader.h"
#include "Core/VarRegistry.h"

using namespace ENGINE_NAMESPACE;

ConsoleVar* cvar_ca_intensity;
ConsoleVar* cvar_ca_enabled;

Render::TonemapPass::TonemapPass()
{
	PostFxRS.DepthTest = false;
	PostFxRS.CullMode = RCullMode::NOT;
	PostFxRS.EnableScissor = false;

	bool IsTransparent = false;
	DirtLensImage = TextureLoader::LoadFileToImage("textures/dirt_lens.png", &IsTransparent);

	cvar_ca_intensity = VarRegistry::RegisterConsoleVar("r", "post_ca_intensity", VarType::Float)->set(1.0f);
	cvar_ca_enabled = VarRegistry::RegisterConsoleVar("r", "post_ca_enabled", VarType::Bool)->set(false);
}

std::vector<Render::PassDependency> Render::TonemapPass::GetDependencies()
{
	return { 
		//Render::PassDependency { .name = LUMINANCE_PASS_OUTPUT, .Optional = false }, 
		Render::PassDependency{ .name = BLOOM_PASS_OUTPUT, .Optional = true } 
	};
}

void Render::TonemapPass::SetInput(Ref<ImageResource> input, const std::string& name)
{
	if (name.compare(LUMINANCE_PASS_OUTPUT) == 0)
	{
		LuminanceImage = input;
	}
	if (name.compare(BLOOM_PASS_OUTPUT) == 0)
	{
		BloomImage = input;
	}
}

void Render::TonemapPass::Init(glm::ivec2 Resolution)
{
	PipelineDescription shaderDesc;
	shaderDesc.ShaderPath = "shaders/tone_map.cso";

	shaderDesc.RequirePermutation("LUMINANCE");

	if (BloomImage)
	{
		shaderDesc.RequirePermutation("BLOOM");
	}

	shaderDesc.RasterizerState.DepthTest = false;
	shaderDesc.StencilState.DepthEnable = false;

	shaderDesc.BindingItems.push_back(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(uint32_t) * 2));

	ToneMapShader = CreateRef<GraphicsPipeline>(shaderDesc);
}

void Render::TonemapPass::Render(const PostProcessingParameters& parameters)
{

	if (!ToneMapShader->ShaderDesc.RenderTarget)
	{
		ToneMapShader->ShaderDesc.RenderTarget = parameters.pOutputFramebuffer;
	}

	struct ToneMapParams
	{
		uint32_t ChromaticAberrationEnabled;
		float ChromaticAberrationIntensity;
	} params;

	params.ChromaticAberrationEnabled = cvar_ca_enabled->asBool();
	params.ChromaticAberrationIntensity = cvar_ca_intensity->asFloat();

	parameters.gCommandBuffer->SetTextureResource(parameters.pColorSampler, 0);
	parameters.gCommandBuffer->SetTextureResource(BloomImage.get(), 2);
	parameters.gCommandBuffer->SetTextureResource(DirtLensImage.get(), 3);
	parameters.gCommandBuffer->SetTextureSampler(parameters.pBilinearTextureSampler, 0);

	Viewport outViewport{};
	outViewport.width = parameters.OutputResolution.x;
	outViewport.height = parameters.OutputResolution.y;

	parameters.gCommandBuffer->SetViewport(&outViewport);
	parameters.gCommandBuffer->SetFramebuffer(parameters.pOutputFramebuffer);

	parameters.gCommandBuffer->SetPipeline(ToneMapShader.get());

	parameters.gCommandBuffer->PushConstants(&params, sizeof(ToneMapParams));
	parameters.gCommandBuffer->Draw(3, 0);
}
