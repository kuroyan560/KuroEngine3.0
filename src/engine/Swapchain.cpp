#include "Swapchain.h"
#include"DescriptorHeap.h"
void Swapchain::SetMetadata()
{
	struct DisplayChromacities
	{
		float RedX;
		float RedY;
		float GreenX;
		float GreenY;
		float BlueX;
		float BlueY;
		float WhiteX;
		float WhiteY;
	} DisplayChromacityList[] = {
	  { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Rec709 
	  { 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Rec2020
	};
	int useIndex = 0;
	if (m_desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
	{
		useIndex = 1;
	}
	const auto& chroma = DisplayChromacityList[useIndex];
	DXGI_HDR_METADATA_HDR10 HDR10MetaData{};
	HDR10MetaData.RedPrimary[0] = UINT16(chroma.RedX * 50000.0f);
	HDR10MetaData.RedPrimary[1] = UINT16(chroma.RedY * 50000.0f);
	HDR10MetaData.GreenPrimary[0] = UINT16(chroma.GreenX * 50000.0f);
	HDR10MetaData.GreenPrimary[1] = UINT16(chroma.GreenY * 50000.0f);
	HDR10MetaData.BluePrimary[0] = UINT16(chroma.BlueX * 50000.0f);
	HDR10MetaData.BluePrimary[1] = UINT16(chroma.BlueY * 50000.0f);
	HDR10MetaData.WhitePoint[0] = UINT16(chroma.WhiteX * 50000.0f);
	HDR10MetaData.WhitePoint[1] = UINT16(chroma.WhiteY * 50000.0f);
	HDR10MetaData.MaxMasteringLuminance = UINT(1000.0f * 10000.0f);
	HDR10MetaData.MinMasteringLuminance = UINT(0.001f * 10000.0f);
	HDR10MetaData.MaxContentLightLevel = UINT16(2000.0f);
	HDR10MetaData.MaxFrameAverageLightLevel = UINT16(500.0f);
	m_swapchain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(HDR10MetaData), &HDR10MetaData);
}

Swapchain::Swapchain(const ComPtr<ID3D12Device>& Device, const ComPtr<IDXGISwapChain1>& Swapchain, DescriptorHeap_CBV_SRV_UAV& DescHeap_CBV_SRV_UAV, DescriptorHeap_RTV& DescHeapRTV, bool UseHDR, const Color& ClearValue)
{
	Swapchain.As(&m_swapchain);	//IDXGISwapChain4 取得
	m_swapchain->GetDesc1(&m_desc);

	//レンダーターゲットリソース設定
	CD3DX12_RESOURCE_DESC resDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		static_cast<UINT>(m_desc.Width),
		static_cast<UINT>(m_desc.Height),
		1,
		1,
		m_desc.Format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	);

	m_images.resize(m_desc.BufferCount);
	m_fences.resize(m_desc.BufferCount);
	m_fenceValues.resize(m_desc.BufferCount);
	m_waitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	HRESULT hr;
	for (UINT i = 0; i < m_desc.BufferCount; ++i)
	{
		//フェンスの生成
		hr = Device->CreateFence(
			0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_fences[i]));

		if (FAILED(hr))assert(0);   //CreateFenece失敗


		//バックバッファの取得
		ComPtr<ID3D12Resource1> buff;
		hr = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&buff));
		assert(SUCCEEDED(hr));


		//バックバッファ名設定
		std::wstring name = L"BackBuffer -";
		name += std::to_wstring(i);
		buff->SetName(name.c_str());

		//シェーダーリソースビュー作成
		DescHeap_CBV_SRV_UAV.CreateSRV(Device, buff, m_desc.Format);
		DescHandles srvHandles(DescHeap_CBV_SRV_UAV.GetCpuHandleTail(), DescHeap_CBV_SRV_UAV.GetGpuHandleTail());

		//レンダーターゲットビュー作成
		DescHeapRTV.CreateRTV(Device, buff);
		DescHandles rtvHandles(DescHeapRTV.GetCpuHandleTail(), DescHeapRTV.GetGpuHandleTail());


		//バックバッファデータの初期化
		m_images[i] = std::make_shared<RenderTarget>(buff, D3D12_RESOURCE_STATE_PRESENT, srvHandles, rtvHandles, resDesc, ClearValue);
	}

	// フォーマットに応じてカラースペースを設定.
	DXGI_COLOR_SPACE_TYPE colorSpace;
	switch (m_desc.Format)
	{
	default:
		colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
		break;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
		break;
	}
	m_swapchain->SetColorSpace1(colorSpace);

	if (UseHDR)
	{
		SetMetadata();
	}
}

void Swapchain::WaitPreviousFrame(const ComPtr<ID3D12CommandQueue>& CmdQueue, const int& FrameIdx)
{
	//コマンドリストの実行完了を待つ
	auto fence = m_fences[FrameIdx];
	auto value = ++m_fenceValues[FrameIdx];
	CmdQueue->Signal(fence.Get(),value);

	//コマンドリストの実行完了を待つ
	auto nextIdx = m_swapchain->GetCurrentBackBufferIndex();
	auto finishValue = m_fenceValues[nextIdx];
	fence = m_fences[nextIdx];
	value = fence->GetCompletedValue();

	if (value < finishValue)
	{
		HANDLE event = CreateEvent(nullptr, false, false, nullptr);
		auto hr = fence->SetEventOnCompletion(finishValue, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
}
