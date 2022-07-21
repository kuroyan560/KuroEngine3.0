#pragma once
#include"D3D12Data.h"

class DescriptorHeap_RTV;
class DescriptorHeap_CBV_SRV_UAV;
//スワップチェイン
class Swapchain
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ComPtr<IDXGISwapChain4> m_swapchain;
	std::vector<std::shared_ptr<RenderTarget>> m_images;

	std::vector<UINT64> m_fenceValues;
	std::vector<ComPtr<ID3D12Fence1>> m_fences;

	DXGI_SWAP_CHAIN_DESC1 m_desc;

	HANDLE m_waitEvent;
	void SetMetadata();
public:
	Swapchain(const ComPtr<ID3D12Device>& Device,
		const ComPtr<IDXGISwapChain1>& Swapchain,
		DescriptorHeap_CBV_SRV_UAV& DescHeap_CBV_SRV_UAV,
		DescriptorHeap_RTV& DescHeapRTV,
		bool UseHDR,
		const Color& ClearValue);

	//スワップチェインゲッタ
	const ComPtr<IDXGISwapChain4>& GetSwapchain() { return m_swapchain; }

	//次のコマンドが積めるようになるまで待機
	void WaitPreviousFrame(const ComPtr<ID3D12CommandQueue>& CmdQueue, const int& FrameIdx);

	//現在のバックバッファのレンダーターゲット
	const std::shared_ptr<RenderTarget>& GetBackBufferRenderTarget() { return m_images[m_swapchain->GetCurrentBackBufferIndex()]; }
};
