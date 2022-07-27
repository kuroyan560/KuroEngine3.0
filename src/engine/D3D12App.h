#pragma once
//DirectX12関連の機能を円滑に利用するためのクラス================================================
#include"D3D12Data.h"
#include"DescriptorHeap.h"
#include"Color.h"

#include<map>
#include<functional>
#include<DirectXTex.h>
using namespace DirectX;

#include"Swapchain.h"

//D3D12Appを利用側のクラス
class D3D12AppUser
{
protected:
	friend class D3D12App;
	D3D12AppUser() {}
	virtual ~D3D12AppUser(){}
	virtual void Render() = 0;	//レンダリング関連の処理
};

class D3D12App
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	static D3D12App* s_instance;
	//ダブルバッファ
	static const UINT s_frameBufferCount = 2;

	//ロード用のラムダ式のための型
	using LoadLambda_t = std::function<HRESULT(const std::wstring& Path, TexMetadata* Meta, ScratchImage& Img)>;
	static std::map<std::string, LoadLambda_t>s_loadLambdaTable;

public:
	static D3D12App* Instance()
	{
		assert(s_instance != nullptr);
		return s_instance;
	}

private:
	//DirectX12デバイス
	ComPtr<ID3D12Device> m_device;
	//DXGIファクトリ
	ComPtr<IDXGIFactory6>m_dxgiFactory = nullptr;
	//コマンドアロケータ
	std::vector<ComPtr<ID3D12CommandAllocator>> m_commandAllocators;
	ComPtr<ID3D12CommandAllocator>m_oneshotCommandAllocator;
	//コマンドリスト
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	//コマンドキュー
	ComPtr<ID3D12CommandQueue> m_commandQueue;

	//ディスクリプタヒープ
	std::unique_ptr<DescriptorHeap_CBV_SRV_UAV> m_descHeapCBV_SRV_UAV;
	std::unique_ptr<DescriptorHeap_RTV> m_descHeapRTV;
	std::unique_ptr<DescriptorHeap_DSV> m_descHeapDSV;

	//スワップチェイン
	std::unique_ptr<Swapchain>m_swapchain;
	DXGI_FORMAT m_backBuffFormat = DXGI_FORMAT_UNKNOWN;	//バックバッファのフォーマット

	//生成したカラーテクスチャ記録用
	struct ColorTexture
	{
		Color m_color;
		int m_width;
		std::shared_ptr<TextureBuffer>m_tex;
	};
	std::list<ColorTexture>m_colorTextures;

	//生成した画像テクスチャ記録用
	struct LoadImgTexture
	{
		std::string m_path;
		std::vector<std::shared_ptr<TextureBuffer>>m_textures;
	};
	std::list<LoadImgTexture>m_loadImgTextures;

	//画像を分割するパイプライン
	struct SplitImgConstData
	{
		Vec2<int> m_imgNum = { 0,0 };
		Vec2<int> m_splitSize;
	};
	int m_splitTexBuffCount = 0;	//SpliteTexBufferが呼ばれた回数
	std::vector<std::shared_ptr<ConstantBuffer>>m_splitImgConstBuff;
	std::shared_ptr<ComputePipeline>m_splitImgPipeline;

	void Initialize(const HWND& Hwnd, const Vec2<int>& ScreenSize, const bool& UseHDR, const Color& ClearValue, const bool& IsFullScreen);
	ComPtr<ID3D12RootSignature>GenerateRootSignature(const std::vector<RootParam>& RootParams, std::vector<D3D12_STATIC_SAMPLER_DESC>& Samplers);

public:
	D3D12App(const HWND& Hwnd, const Vec2<int>& ScreenSize, const bool& UseHDR, const Color& ClearValue, const bool& IsFullScreen = false)
	{
		assert(s_instance == nullptr);
		s_instance = this;
		Initialize(Hwnd, ScreenSize, UseHDR, ClearValue, IsFullScreen);
	}

	//ゲッタ
	const ComPtr<ID3D12Device>& GetDevice() { return m_device; }
	const ComPtr<ID3D12GraphicsCommandList>& GetCmdList() { return m_commandList; }
	const DXGI_FORMAT& GetBackBuffFormat() { return m_backBuffFormat; }
	const std::shared_ptr<RenderTarget>& GetBackBuffRenderTarget() { return m_swapchain->GetBackBufferRenderTarget(); }
	ComPtr<ID3D12CommandAllocator>& GetOneShotCommandAllocator() { return m_oneshotCommandAllocator; }

	//ディスクリプタデータヒープのセット(コンピュートシェーダーを利用するときはユーザーからも呼び出せるように)
	void SetDescHeaps();
	//レンダリング（D3D12AppUserを継承した KuroEngine 以外では呼び出せない）
	void Render(D3D12AppUser* User);

	//CPUリソースをGPUに昇華してコピー
	void UploadCPUResource(std::shared_ptr<GPUResource>& Dest, const size_t& DataSize, const int& ElementNum, const void* SendData);

	//頂点バッファ生成
	std::shared_ptr<VertexBuffer>GenerateVertexBuffer(const size_t& VertexSize, const int& VertexNum, void* InitSendData = nullptr, const char* Name = nullptr, const bool& RWStructuredBuff = false);
	//インデックスバッファ生成
	std::shared_ptr<IndexBuffer>GenerateIndexBuffer(const int& IndexNum, void* InitSendData = nullptr, const char* Name = nullptr, const DXGI_FORMAT& IndexFormat = DXGI_FORMAT_R32_UINT);

	//定数バッファ生成
	std::shared_ptr<ConstantBuffer>GenerateConstantBuffer(const size_t& DataSize, const int& ElementNum, void* InitSendData = nullptr, const char* Name = nullptr);
	//構造化バッファ生成
	std::shared_ptr<StructuredBuffer>GenerateStructuredBuffer(const size_t& DataSize, const int& ElementNum, void* InitSendData = nullptr, const char* Name = nullptr);
	//出力用バッファ作成
	std::shared_ptr<RWStructuredBuffer>GenerateRWStructuredBuffer(const size_t& DataSize, const int& ElementNum, void* InitSendData = nullptr, const char* Name = nullptr);
	//テクスチャバッファ生成
	std::shared_ptr<TextureBuffer>GenerateTextureBuffer(const Color& Color, const int& Width = 1, const DXGI_FORMAT& Format = DXGI_FORMAT_R32G32B32A32_FLOAT);	//単色塗りつぶしテクスチャ生成
	std::shared_ptr<TextureBuffer>GenerateTextureBuffer(const std::string& LoadImgFilePath, const bool& SRVAsCube = false);	//画像ファイル
	std::shared_ptr<TextureBuffer>GenerateTextureBuffer(const Vec2<int>&Size,const DXGI_FORMAT& Format,const char* Name = nullptr);	//コンピュートシェーダーでの描き込み先用（UAVも作成する）
	std::shared_ptr<TextureBuffer>GenerateTextureBuffer(const std::vector<char>& ImgData, const int& Channel = -1);	//Channelで特定のチャンネルのみRチャンネルに出力
	//テクスチャを分割してテクスチャ生成
	void GenerateTextureBuffer(std::shared_ptr<TextureBuffer>* Array, const std::shared_ptr<TextureBuffer>& SorceTexBuffer, const int& AllNum, const Vec2<int>& SplitNum, const std::string& Name);
	/// <summary>
	/// 画像を読み込んで分割 (LoadDivGraph)
	/// </summary>
	/// <param name="Array">分割した画像の書き込み先画像配列</param>
	/// <param name="LoadImgFilePath">画像ファイルパス</param>
	/// <param name="AllNum">総分割数</param>
	/// <param name="SplitNum">縦横の分割数</param>
	/// <param name="SplitSize">分割後のサイズ</param>
	/// <returns></returns>
	void GenerateTextureBuffer(std::shared_ptr<TextureBuffer>* Array, const std::string& LoadImgFilePath, const int& AllNum, const Vec2<int>& SplitNum);
	//Indirect用コマンドバッファ生成
	std::shared_ptr<IndirectCommandBuffer>GenerateIndirectCommandBuffer(const EXCUTE_INDIRECT_TYPE& IndirectType, const int& MaxCommandCount,
		const int& GpuAddressNum, const bool& CounterBuffer = false, const void* InitCommandData = nullptr, const char* Name = nullptr);

	//SRV作成（D3D12App経由）
	DescHandles CreateSRV(const ComPtr<ID3D12Resource>& Buff, const D3D12_SHADER_RESOURCE_VIEW_DESC& ViewDesc);
	//RTV作成（D3D12App経由）
	DescHandles CreateRTV(const ComPtr<ID3D12Resource>& Buff, const D3D12_RENDER_TARGET_VIEW_DESC* ViewDesc = nullptr);
	//DSV作成（D3D12App経由）
	DescHandles CreateDSV(const ComPtr<ID3D12Resource>& Buff, const D3D12_DEPTH_STENCIL_VIEW_DESC* ViewDesc = nullptr);
	//UAV作成（D3D12App経由）
	DescHandles CreateUAV(const ComPtr<ID3D12Resource>& Buff, const D3D12_UNORDERED_ACCESS_VIEW_DESC& ViewDesc, const ComPtr<ID3D12Resource>& CounterBuff = nullptr);

	//レンダーターゲット生成
	std::shared_ptr<RenderTarget>GenerateRenderTarget(const DXGI_FORMAT& Format, const Color& ClearValue, const Vec2<int>Size,
	const wchar_t* TargetName = nullptr, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_RENDER_TARGET, int MipLevel = 1, int ArraySize = 1);
	//デプスステンシル作成
	std::shared_ptr<DepthStencil>GenerateDepthStencil(const Vec2<int>& Size, const DXGI_FORMAT& Format = DXGI_FORMAT_D32_FLOAT, const float& ClearValue = 1.0f);

	//シェーダーのコンパイル
	ComPtr<IDxcBlob>CompileShader(const std::string& FilePath, const std::string& EntryPoint, const std::string& ShaderModel);

	//パイプライン生成
	/// <summary>
	/// グラフィックスパイプラインの生成
	/// </summary>
	/// <param name="Option">パイプライン各種設定</param>
	/// <param name="ShaderInfo">シェーダー情報</param>
	/// <param name="InputLayout">頂天レイアウト</param>
	/// <param name="RootParams">ルートパラメータ（ディスクリプタよりビューが先になるので注意）</param>
	/// <param name="RenderTargetFormat">描画先レンダーターゲット設定</param>
	/// <param name="SamplerDesc">サンプラー</param>
	std::shared_ptr<GraphicsPipeline>GenerateGraphicsPipeline(
		const PipelineInitializeOption& Option,
		const Shaders& ShaderInfo,
		const std::vector<InputLayoutParam>& InputLayout,
		const std::vector<RootParam>& RootParams,
		const std::vector<RenderTargetInfo>& RenderTargetFormat,
		std::vector<D3D12_STATIC_SAMPLER_DESC> Samplers);

	//コンピュートパイプライン生成
	std::shared_ptr<ComputePipeline>GenerateComputePipeline(
		const ComPtr<IDxcBlob>& ComputeShader,
		const std::vector<RootParam>& RootParams,
		std::vector<D3D12_STATIC_SAMPLER_DESC> Samplers);

	//インダイレクト機構
	std::shared_ptr<IndirectDevice>GenerateIndirectDevice(const EXCUTE_INDIRECT_TYPE& ExcuteIndirectType, const std::vector<RootParam>& RootParams, std::vector<D3D12_STATIC_SAMPLER_DESC> Samplers);

	//バックバッファレンダーターゲットをセット
	void SetBackBufferRenderTarget();

	//ワンショットコマンドアロケーターを利用してコンピュートパイプラインを直ちに実行
	void DispathOneShot(std::weak_ptr<ComputePipeline>Pipeline, Vec3<int>ThreadNum,
		std::vector<std::shared_ptr<DescriptorData>>& Datas, std::vector<DESC_HANDLE_TYPE>& DescTypes);
};