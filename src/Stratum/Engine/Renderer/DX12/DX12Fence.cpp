#include "DX12Fence.h"
#include "GlobalSharedData.h"

DX12::DX12Fence::DX12Fence()
{
	m_FenceValue = 0;
	for (int i = 0; i < DX12::s_MaxInFlightFrames; i++)
		m_FenceEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	dxSharedData->Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
}

DX12::DX12Fence::~DX12Fence()
{
}

uint64_t DX12::DX12Fence::Signal(ID3D12CommandQueue* pQueue, uint64_t frameIndex)
{
	if (frameIndex == UINT64_MAX)
	{
		frameIndex = dxSharedData->FrameIndex;
	}

	m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent[frameIndex]);
	pQueue->Signal(m_Fence.Get(), m_FenceValue);
	m_FrameFenceValues[frameIndex] = m_FenceValue;
	m_FenceValue = dxSharedData->FrameCount;

	return m_FenceValue;
}

void DX12::DX12Fence::WaitForSingleObject(uint64_t value, uint64_t frameIndex)
{
	if (frameIndex == UINT64_MAX)
	{
		frameIndex = dxSharedData->FrameIndex;
	}
	if (value == UINT64_MAX)
	{
		value = m_FrameFenceValues[frameIndex];
	}	
	::WaitForSingleObject(m_FenceEvent[frameIndex], 7000);
}

bool DX12::DX12Fence::HasFenceReachedValue(uint64_t value)
{
	return m_Fence->GetCompletedValue() >= value;
}

void DX12::DX12Fence::ResetValues()
{
	for (int i = 0; i < DX12::s_MaxInFlightFrames; i++)
	{
		m_FrameFenceValues[i] = m_FrameFenceValues[dxSharedData->FrameIndex];
	}
}
