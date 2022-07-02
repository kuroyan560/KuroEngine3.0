#include "GraphicsManager.h"

void GraphicsManager::SetRenderTargetsCommand::Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
	for (auto ptr : renderTargets)
	{
		rtvs.emplace_back(ptr.lock()->AsRTV(CmdList));
	}

	const Vec2<float> targetSize = renderTargets[0].lock()->GetGraphSize().Float();
	//ビューポート設定
	auto viewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, targetSize.x, targetSize.y);
	CmdList->RSSetViewports(1, &viewPort);

	//シザー矩形設定
	auto rect = CD3DX12_RECT(0, 0, static_cast<LONG>(targetSize.x), static_cast<LONG>(targetSize.y));
	CmdList->RSSetScissorRects(1, &rect);

	//デプスステンシルがある場合
	if (auto ptr = depthStencil.lock())
	{
		CmdList->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), &rtvs[0], FALSE, ptr->AsDSV(CmdList));
	}
	//ない場合
	else
	{
		CmdList->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), &rtvs[0], FALSE, nullptr);
	}
}

void GraphicsManager::ClearRTVCommand::Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	renderTarget.lock()->Clear(CmdList);
}

void GraphicsManager::ClearDSVCommand::Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	depthStencil.lock()->Clear(CmdList);
}

void GraphicsManager::RenderCommand::Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	//ディスクリプタセット
	for (int i = 0; i < descDatas.size(); ++i)
	{
		if (!descDatas[i].lock())continue;
		descDatas[i].lock()->SetGraphicsDescriptorBuffer(CmdList, types[i], i);
	}

	vertexBuff.lock()->ChangeBarrierForVertexBuffer(CmdList);

	//頂点ビューセット
	CmdList->IASetVertexBuffers(0, 1, &vertexBuff.lock()->GetVBView());

	//インデックスなし
	if (idxBuff.expired())
	{
		CmdList->DrawInstanced(vertexBuff.lock()->sendVertexNum, instanceNum, 0, 0);
	}
	//インデックスあり
	else
	{
		CmdList->IASetIndexBuffer(&idxBuff.lock()->GetIBView());
		CmdList->DrawIndexedInstanced(idxBuff.lock()->indexNum, instanceNum, 0, 0, 0);
	}
}

void GraphicsManager::DispatchCommand::Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	//ディスクリプタセット
	for (int i = 0; i < descDatas.size(); ++i)
	{
		if (!descDatas[i].lock())continue;
		descDatas[i].lock()->SetComputeDescriptorBuffer(CmdList, types[i], i);
	}
	CmdList->Dispatch(threadNum.x, threadNum.y, threadNum.z);
}


void GraphicsManager::SetRenderTargets(const std::vector<std::shared_ptr<RenderTarget>>& RTs, const std::shared_ptr<DepthStencil>& DS)
{
	if (!renderCommands.empty())StackRenderCommands();	//Zバッファ＆透過するかどうかでソートしてグラフィックスコマンドリストに一括スタック
	gCommands.emplace_back(std::make_shared<SetRenderTargetsCommand>(ConvertToWeakPtrArray(RTs), DS));
}

void GraphicsManager::SetGraphicsPipeline(const std::shared_ptr<GraphicsPipeline>& Pipeline)
{
	//新しいパイプラインのハンドル取得
	const int newPipelineHandle = Pipeline->GetPipelineHandle();

	//既にセットされていたものと同じならスルー
	if (recentPipelineType == GRAPHICS && newPipelineHandle == recentPipelineHandle)return;

	if (!renderCommands.empty())StackRenderCommands();	//Zバッファ＆透過するかどうかでソートしてグラフィックスコマンドリストに一括スタック
	gCommands.emplace_back(std::make_shared<SetGraphicsPipelineCommand>(Pipeline));

	//パイプラインタイプ記録
	recentPipelineType = GRAPHICS;

	//パイプラインハンドル記録
	recentPipelineHandle = newPipelineHandle;
}

void GraphicsManager::SetComputePipeline(const std::shared_ptr<ComputePipeline>& Pipeline)
{
	//新しいパイプラインのハンドル取得
	const int newPipelineHandle = Pipeline->GetPipelineHandle();

	//既にセットされていたものと同じならスルー
	if (recentPipelineType == COMPUTE && newPipelineHandle == recentPipelineHandle)return;

	if (!renderCommands.empty())StackRenderCommands();	//Zバッファ＆透過するかどうかでソートしてグラフィックスコマンドリストに一括スタック
	gCommands.emplace_back(std::make_shared<SetComputePipelineCommand>(Pipeline));

	//パイプラインタイプ記録
	recentPipelineType = COMPUTE;

	//パイプラインハンドル記録
	recentPipelineHandle = newPipelineHandle;
}

void GraphicsManager::ClearRenderTarget(const std::shared_ptr<RenderTarget>& RenderTarget)
{
	if (!renderCommands.empty())StackRenderCommands();	//Zバッファ＆透過するかどうかでソートしてグラフィックスコマンドリストに一括スタック
	gCommands.emplace_back(std::make_shared<ClearRTVCommand>(RenderTarget));
}

void GraphicsManager::ClearDepthStencil(const std::shared_ptr<DepthStencil>& DepthStencil)
{
	if (!renderCommands.empty())StackRenderCommands();	//Zバッファ＆透過するかどうかでソートしてグラフィックスコマンドリストに一括スタック
	gCommands.emplace_back(std::make_shared<ClearDSVCommand>(DepthStencil));
}

void GraphicsManager::CopyTexture(const std::shared_ptr<TextureBuffer>& DestTex, const std::shared_ptr<TextureBuffer>& SrcTex)
{
	gCommands.emplace_back(std::make_shared<CopyTex>(DestTex, SrcTex));
}

void GraphicsManager::ObjectRender(const std::shared_ptr<VertexBuffer>& VertexBuff, const std::vector<std::shared_ptr<DescriptorData>>& DescDatas,
	const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes,
	const float& Depth, const bool& TransFlg, const int& InstanceNum)
{
	//ソートするので gCommands ではなく一時的にrenderCommandsに積む
	renderCommands.emplace_back(std::make_shared<RenderCommand>(VertexBuff, ConvertToWeakPtrArray(DescDatas), DescHandleTypes, Depth, TransFlg, InstanceNum));
}

void GraphicsManager::ObjectRender(const std::shared_ptr<VertexBuffer>& VertexBuff,
	const std::shared_ptr<IndexBuffer>& IndexBuff,
	const std::vector<std::shared_ptr<DescriptorData>>& DescDatas,
	const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes,
	const float& Depth, const bool& TransFlg, const int& InstanceNum)
{
	//ソートするので gCommands ではなく一時的にrenderCommandsに積む
	renderCommands.emplace_back(std::make_shared<RenderCommand>(VertexBuff, IndexBuff, ConvertToWeakPtrArray(DescDatas), DescHandleTypes, Depth, TransFlg, InstanceNum));
}

void GraphicsManager::Dispatch(const Vec3<UINT>& ThreadNum, const std::vector<std::shared_ptr<DescriptorData>>& DescDatas, const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes)
{
	gCommands.emplace_back(std::make_shared<DispatchCommand>(ThreadNum, ConvertToWeakPtrArray(DescDatas), DescHandleTypes));
}


void GraphicsManager::StackRenderCommands()
{
	//ソート
	//Z値より透過するかどうかが優先度高い
	renderCommands.sort([](std::shared_ptr<RenderCommand> a, std::shared_ptr<RenderCommand> b)
		{
			if (a->trans == b->trans)
			{
				//Zソート（添字小さい = 奥）
				return b->depth < a->depth;
			}
			else
			{
				return !a->trans && b->trans;
			}
		});

	//一括スタック
	for (auto ptr : renderCommands)
	{
		//キャストしてグラフィックコマンドに積む
		gCommands.emplace_back(std::static_pointer_cast<GraphicsCommandBase>(ptr));
	}

	//レンダリングコマンドクリア
	renderCommands.clear();
}

void GraphicsManager::CommandsExcute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	//最後に積み上げられたのがレンダリングコマンドだった場合
	if (!renderCommands.empty())
	{
		//Zバッファ＆透過するかどうかでソートしてグラフィックスコマンドリストに一括スタック
		StackRenderCommands();
	}

	for (auto itr = gCommands.begin(); itr != gCommands.end(); ++itr)
	{
		(*itr)->Excute(CmdList);
	}

	//コマンドリストクリア
	gCommands.clear();
	recentPipelineType = NONE;
	recentPipelineHandle = -1;
}

void GraphicsManager::CopyTex::Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
{
	destTex.lock()->CopyTexResource(CmdList, srcTex.lock().get());
}