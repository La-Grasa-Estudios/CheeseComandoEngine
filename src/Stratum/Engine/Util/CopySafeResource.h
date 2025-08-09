#pragma once

#include "znmsp.h"
#include "Core/Ref.h"

#include "Renderer/RendererContext.h"

BEGIN_ENGINE

template<typename T>
class CopySafeResource
{
public:

	CopySafeResource()
	{
		for (int i = 0; i < Render::MaxInFlightFrames; i++)
		{
			m_RscArray[i] = NULL;
		}
	}
	template<typename ... Args>
	CopySafeResource(Args&& ... args)
	{
		for (int i = 0; i < Render::MaxInFlightFrames; i++)
		{
			m_RscArray[i] = CreateRef<T>(std::forward<Args>(args)...);
		}
	}

	T* GetPointer()
	{
		return m_RscArray[Render::RendererContext::s_Context->FrameIndex].get();
	}
	Ref<T> GetRef()
	{
		return m_RscArray[Render::RendererContext::s_Context->FrameIndex];
	}

	T* operator ->()
	{
		return m_RscArray[Render::RendererContext::s_Context->FrameIndex].get();
	}

	explicit operator bool() const
	{
		return m_RscArray[Render::RendererContext::s_Context->FrameIndex].get() != NULL;
	}

private:
	Ref<T> m_RscArray[Render::MaxInFlightFrames];
};

END_ENGINE