#pragma once
#include"D3D12Data.h"

//グラフィックスマネージャ
class GraphicsManager
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

#pragma region 専用のグラフィックスコマンド
	//グラフィックスコマンド基底クラス
	class GraphicsCommandBase
	{
	public:
		virtual void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList) = 0;
	};

	//レンダーターゲットセットコマンド
	class SetRenderTargetsCommand : public GraphicsCommandBase
	{
		SetRenderTargetsCommand() = delete;

		const std::vector<std::weak_ptr<RenderTarget>>renderTargets;
		const std::weak_ptr<DepthStencil>depthStencil;
	public:
		SetRenderTargetsCommand(const std::vector<std::weak_ptr<RenderTarget>>& RTs, const std::weak_ptr<DepthStencil>& DS) :renderTargets(RTs), depthStencil(DS) {}
		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override;
	};

	//グラフィックスパイプラインセットコマンド
	class SetGraphicsPipelineCommand : public GraphicsCommandBase
	{
		SetGraphicsPipelineCommand() = delete;

		std::weak_ptr<GraphicsPipeline> gPipeline;
	public:
		SetGraphicsPipelineCommand(std::weak_ptr<GraphicsPipeline> Pipeline) :gPipeline(Pipeline) {}
		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override
		{
			gPipeline.lock()->SetPipeline(CmdList);
		}
		const int& GetPipelineHandle() { return gPipeline.lock()->GetPipelineHandle(); }
	};

	//コンピュートパイプラインセットコマンド
	class SetComputePipelineCommand : public GraphicsCommandBase
	{
		SetComputePipelineCommand() = delete;

		std::weak_ptr<ComputePipeline> cPipeline;
	public:
		SetComputePipelineCommand(std::weak_ptr<ComputePipeline> Pipeline) :cPipeline(Pipeline) {}
		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override
		{
			cPipeline.lock()->SetPipeline(CmdList);
		}
		const int& GetPipelineHandle() { return cPipeline.lock()->GetPipelineHandle(); }
	};

	//レンダーターゲットクリアコマンド
	class ClearRTVCommand : public GraphicsCommandBase
	{
		ClearRTVCommand() = delete;
		std::weak_ptr<RenderTarget>renderTarget;

	public:
		ClearRTVCommand(const std::weak_ptr<RenderTarget>& RenderTarget) :renderTarget(RenderTarget) {}
		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override;
	};

	//デプスステンシルクリアコマンド
	class ClearDSVCommand : public GraphicsCommandBase
	{
		ClearDSVCommand() = delete;
		std::weak_ptr<DepthStencil>depthStencil;
	public:
		ClearDSVCommand(const std::weak_ptr<DepthStencil>& DepthStencil) :depthStencil(DepthStencil) {}
		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override;
	};

	//レンダリング情報（レンダーコマンドに渡す情報）
	//オブジェクトのレンダリングコマンド
	class RenderCommand : public GraphicsCommandBase
	{
		RenderCommand() = delete;

		std::weak_ptr<VertexBuffer>vertexBuff;
		std::weak_ptr<IndexBuffer>idxBuff;
		const std::vector<std::weak_ptr<DescriptorData>> descDatas;	//ディスクリプタ（CBV,SRVなど）
		const std::vector<DESC_HANDLE_TYPE> types;	//セットするディスクリプタタイプ
		const int instanceNum = 1;	//インスタンス数（インスタンシング描画用）

	public:
		const float depth = 0.0f;	//Zソート用
		const bool trans = false;	//透過オブジェクト

		//インデックスなし
		RenderCommand(const std::weak_ptr<VertexBuffer>& VertexBuff,
			const std::vector<std::weak_ptr<DescriptorData>>& DescDatas,
			const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes,
			const float& Depth,
			const bool& TransFlg,
			const int& InstanceNum = 1)
			:vertexBuff(VertexBuff), descDatas(DescDatas), types(DescHandleTypes), depth(Depth), trans(TransFlg), instanceNum(InstanceNum) {}

		//インデックスあり
		RenderCommand(const std::weak_ptr<VertexBuffer>& VertexBuff,
			const std::weak_ptr<IndexBuffer>& IndexBuff,
			const std::vector<std::weak_ptr<DescriptorData>>& DescDatas,
			const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes,
			const float& Depth,
			const bool& TransFlg,
			const int& InstanceNum = 1)
			:vertexBuff(VertexBuff), idxBuff(IndexBuff), descDatas(DescDatas), types(DescHandleTypes), depth(Depth), trans(TransFlg), instanceNum(InstanceNum) {}

		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override;
	};

	//コンピュートシェーダ用Dispatchコマンド
	class DispatchCommand : public GraphicsCommandBase
	{
		DispatchCommand() = delete;

		const Vec3<int>threadNum;
		const std::vector<std::weak_ptr<DescriptorData>> descDatas;	//ディスクリプタ（CBV,SRVなど）
		const std::vector<DESC_HANDLE_TYPE> types;	//セットするディスクリプタタイプ

	public:
		DispatchCommand(const Vec3<int>& ThreadNum,
			const std::vector<std::weak_ptr<DescriptorData>>& DescDatas,
			const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes)
			:threadNum(ThreadNum), descDatas(DescDatas), types(DescHandleTypes) {}

		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override;
	};

	//テクスチャのコピー
	class CopyTex : public GraphicsCommandBase
	{
		std::weak_ptr<TextureBuffer>destTex;
		std::weak_ptr<TextureBuffer>srcTex;
	public:
		CopyTex(const std::weak_ptr<TextureBuffer>& DestTex, const std::weak_ptr<TextureBuffer>& SrcTex)
			:destTex(DestTex), srcTex(SrcTex) {}

		void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList)override;
	};

#pragma endregion


private:
	//グラフィックスコマンドリスト
	std::list<std::shared_ptr<GraphicsCommandBase>>gCommands;

	//レンダリングコマンドリスト（ソートのため一時グラフィックスコマンドリストとは別で積み上げる）
	std::list<std::shared_ptr<RenderCommand>>renderCommands;

	//最後にセットされたいパイプラインがグラフィックスかコンピュートか
	enum PIPELINE_TYPE { GRAPHICS, COMPUTE, NONE }recentPipelineType = NONE;

	//最後にセットされたパイプラインハンドル
	int recentPipelineHandle = -1;

	//Zバッファ、透過するかどうかでソート
	void StackRenderCommands();

	//shader_ptr配列をweak_ptr配列に変換
	template<typename T>
	std::vector<std::weak_ptr<T>>ConvertToWeakPtrArray(const std::vector<std::shared_ptr<T>>& Array)
	{
		std::vector<std::weak_ptr<T>>result;
		for (int i = 0; i < Array.size(); ++i)
		{
			result.emplace_back(std::weak_ptr<T>(Array[i]));
		}
		return result;
	}

public:
	//レンダーターゲットのセットコマンド積み上げ
	void SetRenderTargets(const std::vector<std::shared_ptr<RenderTarget>>& RTs, const std::shared_ptr<DepthStencil>& DS = std::shared_ptr<DepthStencil>());

	//グラフィックスパイプラインのセットコマンド積み上げ
	void SetGraphicsPipeline(const std::shared_ptr<GraphicsPipeline>& Pipeline);

	//コンピュートパイプラインのセットコマンド積み上げ
	void SetComputePipeline(const std::shared_ptr<ComputePipeline>& Pipeline);

	//レンダーターゲットのクリアコマンド積み上げ
	void ClearRenderTarget(const std::shared_ptr<RenderTarget>& RenderTarget);

	//デプスステンシルのクリアコマンド積み上げ
	void ClearDepthStencil(const std::shared_ptr<DepthStencil>& DepthStencil);

	//テクスチャコピーコマンド積み上げ
	void CopyTexture(const std::shared_ptr<TextureBuffer>& DestTex, const std::shared_ptr<TextureBuffer>& SrcTex);

	//オブジェクトのレンダリングコマンド積み上げ（インデックスなし）
	void ObjectRender(const std::shared_ptr<VertexBuffer>& VertexBuff,
		const std::vector<std::shared_ptr<DescriptorData>>& DescDatas,
		const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes,
		const float& Depth, const bool& TransFlg, const int& InstanceNum = 1);

	//オブジェクトのレンダリングコマンド積み上げ（インデックスなし）
	void ObjectRender(const std::shared_ptr<VertexBuffer>& VertexBuff, const std::shared_ptr<IndexBuffer>& IndexBuff,
		const std::vector<std::shared_ptr<DescriptorData>>& DescDatas,
		const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes,
		const float& Depth, const bool& TransFlg, const int& InstanceNum = 1);

	//ディスパッチコマンド積み上げ
	void Dispatch(const Vec3<int>& ThreadNum,
		const std::vector<std::shared_ptr<DescriptorData>>& DescDatas,
		const std::vector<DESC_HANDLE_TYPE>& DescHandleTypes);

	//コマンドリスト全実行
	void CommandsExcute(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& CmdList);
};