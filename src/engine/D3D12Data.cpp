#include"D3D12Data.h"

void GPUResource::Mapping(const size_t& DataSize, const int& ElementNum, const void* SendData)
{
	//送るデータがnullなら何もしない
	KuroFunc::ErrorMessage(SendData == nullptr, "GPUResource", "Mapping", "データのマッピングに失敗、引数がnullptrです\n");

	//まだマッピングしていなければマッピング
	if (!m_mapped)
	{
		//マップ、アンマップのオーバーヘッドを軽減するためにはこのインスタンスが生きている間はUnmapしない
		auto hr = m_buff->Map(0, nullptr, (void**)&m_buffOnCPU);
		KuroFunc::ErrorMessage(FAILED(hr), "GPUResource", "Mapping", "データのマッピングに失敗\n");
		m_mapped = true;
	}

	memcpy(m_buffOnCPU, SendData, DataSize * ElementNum);
}

void GPUResource::ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& NewBarrier)
{
    //リソースバリア変化なし
    if (m_barrier == NewBarrier)return;

    //リソースバリア変更
	auto changeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_buff.Get(),
		m_barrier,
		NewBarrier);

    CmdList->ResourceBarrier(
        1,
        &changeBarrier);

    //リソースバリア状態記録
    m_barrier = NewBarrier;
}

void GPUResource::CopyGPUResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, GPUResource& CopySource)
{
	//コピー元のリソースの状態を記録
	auto oldBarrierCopySource = CopySource.m_barrier;
	//コピー元のリソースバリアをコピー元用に変更
	CopySource.ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

	//コピー先である自身のリソースの状態を記録
	auto oldBarrierThis = m_barrier;
	//コピー先である自身のリソースバリアをコピー先用に変更
	this->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

	//コピー実行
	CmdList->CopyResource(this->m_buff.Get(), CopySource.m_buff.Get());

	//コピー元を元のリソースバリアに戻す
	CopySource.ChangeBarrier(CmdList, oldBarrierCopySource);

	//コピー先である自身を元のリソースバリアに戻す
	this->ChangeBarrier(CmdList, oldBarrierThis);
}

void DescriptorData::SetGraphicsDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam)
{
	OnSetDescriptorBuffer(CmdList,Type);
	CmdList->SetGraphicsRootDescriptorTable(RootParam, m_handles.GetHandle(Type));
}

void DescriptorData::SetComputeDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam)
{
	OnSetDescriptorBuffer(CmdList,Type);
	CmdList->SetComputeRootDescriptorTable(RootParam, m_handles.GetHandle(Type));
}

void TextureBuffer::CopyTexResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, TextureBuffer* CopySource)
{
	m_resource->CopyGPUResource(CmdList, *CopySource->m_resource);
}

void RenderTarget::Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CmdList->ClearRenderTargetView(
		m_handles.GetHandle(RTV),
		&m_clearValue[0],
		0,
		nullptr);
}

void DepthStencil::Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	CmdList->ClearDepthStencilView(
		m_handles.GetHandle(DSV),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		m_clearValue,
		0,
		0,
		nullptr);
}

int GraphicsPipeline::s_pipelineNum = 0;

void GraphicsPipeline::SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	//パイプラインステートの設定
	CmdList->SetPipelineState(m_pipeline.Get());
	//ルートシグネチャの設定
	CmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	//プリミティブ形状を設定
	CmdList->IASetPrimitiveTopology(m_topology);
}

int ComputePipeline::s_pipelineNum = 0;

void ComputePipeline::SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	//パイプラインステートの設定
	CmdList->SetPipelineState(m_pipeline.Get());
	//ルートシグネチャの設定
	CmdList->SetComputeRootSignature(m_rootSignature.Get());
}

Microsoft::WRL::ComPtr<ID3D12Resource>IndirectDevice::s_countResetBuffer;

IndirectDevice::IndirectDevice(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12CommandSignature>& CmdSignature, const int& GPUBufferNum)
	:m_cmdSignature(CmdSignature), m_gpuBuffNum(GPUBufferNum)
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto hr = Device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_countBuffer));
	assert(SUCCEEDED(hr));

	heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
	hr = Device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&s_countResetBuffer));
	assert(SUCCEEDED(hr));

	UINT8* countResetMap = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	hr = s_countResetBuffer->Map(0, &readRange, reinterpret_cast<void**>(&countResetMap));
	assert(SUCCEEDED(hr));
	ZeroMemory(countResetMap, sizeof(UINT));
	s_countResetBuffer->Unmap(0, nullptr);
}

void IndirectDevice::Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList,
	int MaxCommandCount,
	ID3D12Resource* ArgBuffer, UINT ArgBufferOffset,
	bool UseCountBuffer)
{
	if (UseCountBuffer)
	{
		//カウントバッファのリセット
		CmdList->CopyBufferRegion(m_countBuffer.Get(), 0, s_countResetBuffer.Get(), 0, sizeof(UINT));

		//カウントバッファリソースバリア切り替え（前）
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_countBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CmdList->ResourceBarrier(1, &barrier);

		//実行（カウントバッファ有り）
		CmdList->ExecuteIndirect(
			m_cmdSignature.Get(),
			MaxCommandCount,
			ArgBuffer,
			ArgBufferOffset,
			m_countBuffer.Get(),
			0);

		//カウントバッファリソースバリア切り替え（後）
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_countBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
		CmdList->ResourceBarrier(1, &barrier);
	}
	else
	{
		//実行（カウントバッファ無し）
		CmdList->ExecuteIndirect(
			m_cmdSignature.Get(),
			MaxCommandCount,
			ArgBuffer,
			ArgBufferOffset,
			nullptr,
			0);
	}
}



