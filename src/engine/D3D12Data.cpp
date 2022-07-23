#include"D3D12Data.h"

void GPUResource::Mapping(const size_t& DataSize, const int& ElementNum, const void* SendData)
{
	//送るデータがnullなら何もしない
	assert(SendData);

	//まだマッピングしていなければマッピング
	if (!m_mapped)
	{
		//マップ、アンマップのオーバーヘッドを軽減するためにはこのインスタンスが生きている間はUnmapしない
		auto hr = m_buff->Map(0, nullptr, (void**)&m_buffOnCPU);
		assert(SUCCEEDED(hr));
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

void IndirectCommandBuffer::ResetCounterBuffer()
{
	assert(m_counterBuffer);
	UINT num = 0;
	m_counterBuffer->Mapping(sizeof(UINT), 1, &num);
}

void IndirectDevice::Execute(const ComPtr<ID3D12GraphicsCommandList>& CmdList, std::shared_ptr<IndirectCommandBuffer> CmdBuff, UINT ArgBufferOffset)
{
	//コマンドシグネチャとバッファの Indirect 形式が一致
	assert(m_indirectType == CmdBuff->m_indirectType);

	CmdBuff->GetBuff()->GetResource()->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	//実行
	CmdList->ExecuteIndirect(
		m_cmdSignature.Get(),
		CmdBuff->m_maxCommandCount,
		CmdBuff->m_buff->GetResource()->GetBuff().Get(),
		ArgBufferOffset,
		CmdBuff->m_counterBuffer ? CmdBuff->m_counterBuffer->GetBuff().Get() : nullptr,
		0);
}
