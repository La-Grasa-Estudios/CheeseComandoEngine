#include "ImageResource.h"
#include "RendererContext.h"
#include "CopyEngine.h"

using namespace ENGINE_NAMESPACE;

Render::ImageResource::ImageResource(const ImageDescription& desc)
{
	ImageDesc = desc;

	auto textureDesc = nvrhi::TextureDesc();

	textureDesc.setWidth(desc.Width);
	textureDesc.setHeight(desc.Height);
	textureDesc.setMipLevels(desc.MipLevels);
	textureDesc.setArraySize(desc.ArraySize);

	textureDesc.setFormat(FormatUtil::ConvertEngineFormatToNV(desc.Format));

	textureDesc.setSampleCount(1);
	textureDesc.setSampleQuality(0);

	textureDesc.setIsRenderTarget(desc.AllowFramebufferUsage);

	textureDesc.setInitialState(nvrhi::ResourceStates::Common);
	textureDesc.setKeepInitialState(false);

	textureDesc.setDebugName("Image");

	if (textureDesc.isRenderTarget)
	{
		textureDesc.setDebugName("Image Render Target");

		textureDesc.setClearValue(nvrhi::Color(desc.ClearValue.r, desc.ClearValue.g, desc.ClearValue.b, desc.ClearValue.a));
		textureDesc.setUseClearValue(true);

		if (desc.IsDepthFormat())
		{
			textureDesc.setIsTypeless(true);
			//textureDesc.setInitialState(nvrhi::ResourceStates::DepthWrite);
			textureDesc.setDebugName("Image Depth Target");
		}
		else
		{
			//textureDesc.setInitialState(nvrhi::ResourceStates::RenderTarget);
		}
	}

	textureDesc.setDimension(nvrhi::TextureDimension::Texture2D);

	if (textureDesc.arraySize > 1)
	{
		textureDesc.setDimension(nvrhi::TextureDimension::Texture2DArray);
	}

	if (desc.Type == ImageType::CUBEMAP_TEXTURE)
	{
		assert(desc.ArraySize % 6 == 0);
		textureDesc.setDimension(desc.ArraySize > 6 ? nvrhi::TextureDimension::TextureCube : nvrhi::TextureDimension::TextureCubeArray);
	}

	textureDesc.setIsUAV(desc.AllowComputeResourceUsage);

	if (desc.Immutable)
	{
		textureDesc.setInitialState(nvrhi::ResourceStates::ShaderResource);
	}
	if (RendererContext::get_api() == RendererAPI::VULKAN)
		textureDesc.setInitialState(nvrhi::ResourceStates::ShaderResource);

	Handle = RendererContext::GetDevice()->createTexture(textureDesc);

	// Vulkan hack
	// Who the fuck thinks about creating an API so f'ing strict that you cannot create resources with a default state & w/o 
	// layout decay/promotion modern GPU's don't require that and usually don't get penalized performance
	// It's been 8 hours since i started trying to implement a vulkan backend and i only managed to get the opening video working
	// What the fuck Khrono now i see why almost nobody in the AAA industry uses your fucking API
	// Now i see why most directx 12 games don't get native ports over to linux
	if (RendererContext::get_api() == RendererAPI::VULKAN)
	{
		auto cmdList = RendererContext::GetDevice()->createCommandList();
		cmdList->open();
		cmdList->beginTrackingTextureState(Handle, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
		cmdList->setTextureState(Handle, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
		cmdList->close();
		RendererContext::GetDevice()->executeCommandList(cmdList);
		RendererContext::GetDevice()->waitForIdle();
	}

	if (!desc.DefaultData.empty())
	{
		auto cmd = CopyEngine::WriteTexture(Handle, &mResourceReady);

		cmd.commandList->setTextureState(Handle, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);

		for (uint32_t i = 0; i < desc.DefaultData.size() && i < desc.MipLevels; i++)
		{
			ImageResourceData rsc = desc.DefaultData[i];
			cmd.commandList->writeTexture(Handle, 0, i, rsc.pSysMem, rsc.MemPitch);
		}

		if (desc.Immutable)
		{
			cmd.commandList->setTextureState(Handle, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
			cmd.commandList->setPermanentTextureState(Handle, nvrhi::ResourceStates::ShaderResource);
			cmd.commandList->commitBarriers();
		}

		CopyEngine::Submit(cmd);

	}
	else
	{
		mResourceReady = true;
	}

	size_t size = 0;
	for (uint32_t i = 0; i < desc.MipLevels; i++)
	{
		size += RendererContext::GetSizeForFormat(ImageDesc.Width / (i + 1), ImageDesc.Height / (i + 1), (uint32_t)ImageDesc.Format);
	}
	RendererContext::VideoMemoryAdd(size);
}

Render::ImageResource::~ImageResource()
{
	size_t size = 0;
	for (uint32_t i = 0; i < ImageDesc.MipLevels; i++)
	{
		size += RendererContext::GetSizeForFormat(ImageDesc.Width / (i + 1), ImageDesc.Height / (i + 1), (uint32_t)ImageDesc.Format);
	}
	RendererContext::VideoMemorySub(size);
}

glm::ivec2 Render::ImageResource::GetSize()
{
	return glm::ivec2(ImageDesc.Width, ImageDesc.Height);
}

bool Render::ImageResource::IsResourceReady()
{
	return mResourceReady;
}
