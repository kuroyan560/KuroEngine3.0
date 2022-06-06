#include"D3D12Data.h"

void GPUResource::Mapping(const size_t& DataSize, const int& ElementNum, const void* SendData)
{
	//送るデータがnullなら何もしない
	KuroFunc::ErrorMessage(SendData == nullptr, "GPUResource", "Mapping", "データのマッピングに失敗、引数がnullptrです\n");

	//まだマッピングしていなければマッピング
	if (!mapped)
	{
		//マップ、アンマップのオーバーヘッドを軽減するためにはこのインスタンスが生きている間はUnmapしない
		auto hr = buff->Map(0, nullptr, (void**)&buffOnCPU);
		KuroFunc::ErrorMessage(FAILED(hr), "GPUResource", "Mapping", "データのマッピングに失敗\n");
		mapped = true;
	}

	memcpy(buffOnCPU, SendData, DataSize * ElementNum);
}

void GPUResource::ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& NewBarrier)
{
    //リソースバリア変化なし
    if (barrier == NewBarrier)return;

    //リソースバリア変更
	auto changeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		buff.Get(),
		barrier,
		NewBarrier);

    CmdList->ResourceBarrier(
        1,
        &changeBarrier);

    //リソースバリア状態記録
    barrier = NewBarrier;
}

void GPUResource::CopyGPUResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, GPUResource& CopySource)
{
	//コピー元のリソースの状態を記録
	auto oldBarrierCopySource = CopySource.barrier;
	//コピー元のリソースバリアをコピー元用に変更
	CopySource.ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

	//コピー先である自身のリソースの状態を記録
	auto oldBarrierThis = barrier;
	//コピー先である自身のリソースバリアをコピー先用に変更
	this->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

	//コピー実行
	CmdList->CopyResource(this->buff.Get(), CopySource.buff.Get());

	//コピー元を元のリソースバリアに戻す
	CopySource.ChangeBarrier(CmdList, oldBarrierCopySource);

	//コピー先である自身を元のリソースバリアに戻す
	this->ChangeBarrier(CmdList, oldBarrierThis);
}

void DescriptorData::SetGraphicsDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam)
{
	OnSetDescriptorBuffer(CmdList,Type);
	CmdList->SetGraphicsRootDescriptorTable(RootParam, handles.GetHandle(Type));
}

void DescriptorData::SetComputeDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam)
{
	OnSetDescriptorBuffer(CmdList,Type);
	CmdList->SetComputeRootDescriptorTable(RootParam, handles.GetHandle(Type));
}

void TextureBuffer::CopyTexResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, TextureBuffer* CopySource)
{
	resource->CopyGPUResource(CmdList, *CopySource->resource);
}

void RenderTarget::Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CmdList->ClearRenderTargetView(
		handles.GetHandle(RTV),
		&clearValue[0],
		0,
		nullptr);
}

void DepthStencil::Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	CmdList->ClearDepthStencilView(
		handles.GetHandle(DSV),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		clearValue,
		0,
		0,
		nullptr);
}

int GraphicsPipeline::PIPELINE_NUM = 0;

void GraphicsPipeline::SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	//パイプラインステートの設定
	CmdList->SetPipelineState(pipeline.Get());
	//ルートシグネチャの設定
	CmdList->SetGraphicsRootSignature(rootSignature.Get());
	//プリミティブ形状を設定
	CmdList->IASetPrimitiveTopology(topology);
}

void ComputePipeline::SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	//パイプラインステートの設定
	CmdList->SetPipelineState(pipeline.Get());
	//ルートシグネチャの設定
	CmdList->SetComputeRootSignature(rootSignature.Get());
}

