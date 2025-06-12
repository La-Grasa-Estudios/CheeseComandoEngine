#pragma once

#include "znmsp.h"
#include "Scene/Post/PostProcessingPass.h"

BEGIN_ENGINE

namespace Render
{
	class ChromaticAberrationPass : public PostProcessingPass
	{

	public:

		ChromaticAberrationPass();

		std::vector<PassDependency> GetDependencies() override;

		void Init(glm::ivec2 Resolution) override;

		void PreRender(const PostProcessingParameters& parameters) override;
		void OnFirstUse(const PostProcessingParameters& parameters) override;

		Ref<ComputePipeline> CsChAberration;

		Ref<ImageResource> Output;

	};
}

END_ENGINE