#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <glm/ext.hpp>

#include "znmsp.h"

#include "Core/Ref.h"
#include "RenderCommands.h"


BEGIN_ENGINE
namespace Render {

	using StaticBindingTable = nvrhi::static_vector<nvrhi::BindingSetItem, 64>;

	enum class VertexType {

		INT,
		SHORT,
		BYTE,

		INT4_16_NORM,
		INT2_16_NORM,
		INT16_NORM,
		INT4_8_NORM,
		INT2_8_NORM,
		INT8_NORM,

		UINT4_16_NORM,
		UINT2_16_NORM,
		UINT16_NORM,
		UINT4_8_NORM,
		UINT2_8_NORM,
		UINT8_NORM,

		UBYTE4,

		UINT,
		USHORT,
		UBYTE,

		FLOAT_32,
		FLOAT2_32,
		FLOAT3_32,
		FLOAT4_32,

		FLOAT_16,
		FLOAT2_16,
		FLOAT4_16,

		FLOAT3_11_11_10,

	};

	enum class VertexInputRate {
		PER_VERTEX,
		PER_INSTANCE,
	};

	struct VertexAttribute {

		VertexType Type;
		VertexInputRate InputRate = VertexInputRate::PER_VERTEX;
		uint32_t Offset;
		uint32_t Index;
		uint32_t BufferIndex = 0;
		bool Normalized;

	};

	struct ShaderVertexLayout {
		std::vector<VertexAttribute> VertexAttributes;
		size_t Stride;
	};

	struct PipelineDescription {
		std::string ShaderPath;

		BlendState BlendState;
		RasterizerState RasterizerState;
		StencilState StencilState;
		ShaderVertexLayout VertexLayout;
		uint32_t NumRenderTargets;
		Framebuffer* RenderTarget = NULL;
		ImageFormat DepthTargetFormat = ImageFormat::DEPTH16;

		bool UseStaticBinding = false;
		bool UseBindless = false;

		std::vector<nvrhi::BindingLayoutItem> BindingItems;
		std::vector<nvrhi::BindingLayoutItem> BindlessItems;

		uint32_t BindlessCapacity = 4096;

		nvrhi::static_vector<nvrhi::BindingSetItem, 64> StaticBindingItems;

		void RequirePermutation(const std::string& key);
		void SetStaticBinding(const nvrhi::static_vector<nvrhi::BindingSetItem, 64>& set);

		void AddBindlessItem(const nvrhi::BindingLayoutItem& item);

		std::string PermutationKey;

		PipelineDescription();

		//void FromFile(std::string_view path);
	};


	struct PipelineCreationDescription {
		std::vector<uint8_t> SourceVertex;
		std::vector<uint8_t> SourcePixel;
		std::vector<uint8_t> SourceGeometry;
		PipelineDescription Description;
	};

    class GraphicsPipeline
    {
    public:

        PipelineDescription ShaderDesc;

        GraphicsPipeline(PipelineDescription desc);
        ~GraphicsPipeline();
		
		void UpdatePipeline();

		void SetRenderTarget(Ref<Framebuffer> rt);
		void UpdateStaticBinding(const nvrhi::static_vector<nvrhi::BindingSetItem, 64>& setDesc);

		nvrhi::GraphicsPipelineHandle PipelineHandle;
		nvrhi::BindingLayoutHandle BindingLayout;
		nvrhi::BindingLayoutHandle BindlessLayout;

		nvrhi::BindingSetHandle StaticBindingSet;

	private:

		nvrhi::ShaderHandle mVertexShader;
		nvrhi::ShaderHandle mPixelShader;

    };
}
END_ENGINE