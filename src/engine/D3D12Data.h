#pragma once
#include<d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include<wrl.h>
#include <cassert>
#include"Vec.h"
#include"KuroFunc.h"
#include"Color.h"

enum DESC_HANDLE_TYPE { CBV, SRV, UAV, RTV, DSV, DESC_HANDLE_NUM };

//ディスクリプタハンドル
class DescHandles
{
private:
	//初期化されているか
	bool invalid = true;
	//CPUハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	//GPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

public:
	//コンストラクタ
	DescHandles() {}
	DescHandles(const DescHandles& tmp)
	{
		KuroFunc::ErrorMessage(!tmp, "DescHandles", "コピーコンストラクタ", "コピー元の初期化がされていません\n");
		Initialize(tmp.cpuHandle, tmp.gpuHandle);
	}
	DescHandles(DescHandles&& tmp)
	{
		KuroFunc::ErrorMessage(!tmp, "DescHandles", "ムーブコンストラクタ", "ムーブ元の初期化がされていません\n");
		Initialize(tmp.cpuHandle, tmp.gpuHandle);
	}
	DescHandles(const D3D12_CPU_DESCRIPTOR_HANDLE& CPUHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& GPUHandle) { Initialize(CPUHandle, GPUHandle); }

	//初期化
	void Initialize(const D3D12_CPU_DESCRIPTOR_HANDLE& CPUHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& GPUHandle)
	{
		cpuHandle = CPUHandle;
		gpuHandle = GPUHandle;
		invalid = false;
	}

	// if 文にそのままかけて初期済みか判定出来るように
	operator bool()const { return !invalid; }
	//代入演算子
	void operator=(const DescHandles& rhs)
	{
		KuroFunc::ErrorMessage(rhs.invalid, "DescHandles", "代入演算子", "代入するディスクリプタハンドルが初期化されていません\n");
		cpuHandle = rhs.cpuHandle;
		gpuHandle = rhs.gpuHandle;
		invalid = false;
	}

	operator const D3D12_CPU_DESCRIPTOR_HANDLE& ()const
	{
		KuroFunc::ErrorMessage(invalid, "DescHandles", "operatorゲッタ", "初期化されていません\n");
		return cpuHandle;
	}
	operator const D3D12_CPU_DESCRIPTOR_HANDLE* ()const
	{
		KuroFunc::ErrorMessage(invalid, "DescHandles", "operatorゲッタ", "初期化されていません\n");
		return &cpuHandle;
	}
	operator const D3D12_GPU_DESCRIPTOR_HANDLE& ()const
	{
		KuroFunc::ErrorMessage(invalid, "DescHandles", "operatorゲッタ", "初期化されていません\n");
		return gpuHandle;
	}
	operator const D3D12_GPU_DESCRIPTOR_HANDLE* ()const
	{
		KuroFunc::ErrorMessage(invalid, "DescHandles", "operatorゲッタ", "初期化されていません\n");
		return &gpuHandle;
	}
};

class DescHandlesContainer
{
private:
	DescHandles handles[DESC_HANDLE_NUM];

public:
	void Initialize(const DESC_HANDLE_TYPE& Type, const DescHandles& Handle)
	{
		handles[Type] = Handle;
	}
	const DescHandles& GetHandle(const DESC_HANDLE_TYPE& Type) { return handles[Type]; }
};

//GPUリソース（データコンテナ）
class GPUResource
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	GPUResource() = delete;
	GPUResource(const GPUResource& tmp) = delete;
	GPUResource(GPUResource&& tmp) = delete;

	void* buffOnCPU = nullptr;//CPU側からアクセスできるするバッファのアドレス
	bool mapped = false;
	ComPtr<ID3D12Resource>buff = nullptr;	//リソースバッファ
	D3D12_RESOURCE_STATES barrier = D3D12_RESOURCE_STATE_COMMON;	//リソースバリアの状態

public:
	GPUResource(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier,const wchar_t* Name = nullptr) 
	{
		buff = Buff;
		if (Name != nullptr)buff.Get()->SetName(Name);

		barrier = Barrier;
	}
	~GPUResource()
	{
		if (mapped)buff->Unmap(0, nullptr);
	}

	//バッファ名のセッタ
	void SetName(const wchar_t* Name)
	{
		buff->SetName(Name);
	}

	//バッファ取得
	const ComPtr<ID3D12Resource>& GetBuff() { return buff; }
	//マッピング
	void Mapping(const size_t& DataSize, const int& ElementNum, const void* SendData);
	//リソースバリアの変更
	void ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& NewBarrier);
	//バッファのコピー（インスタンスは別物、引数にコピー元）
	void CopyGPUResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, GPUResource& CopySource);

	template<typename T>
	const T* GetBuffOnCpu() { return (T*) buffOnCPU; }
};

//レンダリングの際にセットするバッファ（ディスクリプタ登録の必要があるタイプのデータ）
class DescriptorData
{
	DescriptorData() = delete;
	DescriptorData(const DescriptorData& tmp) = delete;
	DescriptorData(DescriptorData&& tmp) = delete;
protected:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::shared_ptr<GPUResource> resource;	//GPUバッファ
	DescHandlesContainer handles;	//ディスクリプタハンドル

	DescriptorData(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier) :resource(std::make_shared<GPUResource>(Buff, Barrier)) {}
	DescriptorData(const std::shared_ptr<GPUResource>& GPUResource) :resource(GPUResource) {}	//同じものを差す

	//バッファセットのタイミングで呼ばれる関数、リソースバリアを変えるなど
	virtual void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type) = 0;
public:
	//バッファセット
	void SetGraphicsDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam);
	void SetComputeDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam);
	const std::shared_ptr<GPUResource>& GetResource() { return resource; }
};

//定数バッファ
class ConstantBuffer : public DescriptorData
{
	ConstantBuffer() = delete;
	ConstantBuffer(const ConstantBuffer& tmp) = delete;
	ConstantBuffer(ConstantBuffer&& tmp) = delete;
private:
	const size_t dataSize;	//ユーザ定義データの１要素サイズ
	const int elementNum = 0;	//要素数

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

public:
	ConstantBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& CBVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(Buff, Barrier), dataSize(DataSize), elementNum(ElementNum) 
	{
		handles.Initialize(CBV, CBVHandles);
	}
	void Mapping(const void* SendData)
	{
		resource->Mapping(dataSize, elementNum, SendData);
	}
};

//構造化バッファ（見かけ上は定数バッファと同じ）
class StructuredBuffer : public DescriptorData
{
	StructuredBuffer() = delete;
	StructuredBuffer(const StructuredBuffer& tmp) = delete;
	StructuredBuffer(StructuredBuffer&& tmp) = delete;

private:
	const size_t dataSize;	//ユーザ定義データの１要素サイズ
	const int elementNum = 0;	//要素数

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
public:
	StructuredBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& SRVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(Buff, Barrier), dataSize(DataSize), elementNum(ElementNum) 
	{
		handles.Initialize(SRV, SRVHandles);
	}
	void Mapping(void* SendData)
	{
		resource->Mapping(dataSize, elementNum, SendData);
	}
};

//出力先バッファ
class RWStructuredBuffer : public DescriptorData
{
	RWStructuredBuffer() = delete;
	RWStructuredBuffer(const RWStructuredBuffer& tmp) = delete;
	RWStructuredBuffer(RWStructuredBuffer&& tmp) = delete;

private:
	const size_t dataSize;	//ユーザ定義データの１要素サイズ
	const int elementNum = 0;	//要素数

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
public:
	RWStructuredBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& UAVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(Buff, Barrier), dataSize(DataSize), elementNum(ElementNum) 
	{
		handles.Initialize(UAV, UAVHandles);
	}
	RWStructuredBuffer(const std::shared_ptr<GPUResource>& GPUResource, const DescHandles& UAVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(GPUResource), dataSize(DataSize), elementNum(ElementNum)
	{
		handles.Initialize(UAV, UAVHandles);
	}

	void CopyBuffOnGPU(const ComPtr<ID3D12GraphicsCommandList>& CmdList, GPUResource& Dest) { Dest.CopyGPUResource(CmdList, *this->resource); }

	std::weak_ptr<GPUResource>GetResource() { return resource; }
};

//テクスチャリソース基底クラス
class TextureBuffer : public DescriptorData
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	TextureBuffer() = delete;
	TextureBuffer(const TextureBuffer& tmp) = delete;
	TextureBuffer(TextureBuffer&& tmp) = delete;
protected:
	CD3DX12_RESOURCE_DESC texDesc;	//テクスチャ情報（幅、高さなど）

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

public:
	TextureBuffer(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& SRVHandles, const CD3DX12_RESOURCE_DESC& Desc)
		:DescriptorData(Buff, Barrier), texDesc(Desc) 
	{
		handles.Initialize(SRV, SRVHandles);
	}
	TextureBuffer(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& SRVHandles, const D3D12_RESOURCE_DESC& Desc)
		:DescriptorData(Buff, Barrier), texDesc(CD3DX12_RESOURCE_DESC(Desc)) 
	{
		handles.Initialize(SRV, SRVHandles);
	}
	TextureBuffer(const std::shared_ptr<GPUResource>& GPUResource, const DescHandles& SRVHandles, const D3D12_RESOURCE_DESC& Desc)
	: DescriptorData(GPUResource), texDesc(CD3DX12_RESOURCE_DESC(Desc))
	{
		handles.Initialize(SRV, SRVHandles);
	}

	void ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& Barrier) { resource->ChangeBarrier(CmdList, Barrier); }
	void CopyTexResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, TextureBuffer* CopySource);
	const CD3DX12_RESOURCE_DESC& GetDesc() { return texDesc; }
	void SetUAVHandles(const DescHandles& UAVHandles) { handles.Initialize(UAV, UAVHandles); }
	Vec2<int>GetGraphSize()
	{
		return Vec2<int>(static_cast<int>(texDesc.Width), static_cast<int>(texDesc.Height));
	}
};

//レンダーターゲット
class RenderTarget : public TextureBuffer
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	RenderTarget() = delete;
	RenderTarget(const RenderTarget& tmp) = delete;
	RenderTarget(RenderTarget&& tmp) = delete;
private:
	float clearValue[4] = { 0.0f };	//クリア値
	//ピクセルシェーダーリソースとして使われる
	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
public:
	RenderTarget(const ComPtr<ID3D12Resource1>& Buff,
		const D3D12_RESOURCE_STATES& Barrier,
		const DescHandles& SRVHandles,
		const DescHandles& RTVHandles,
		const CD3DX12_RESOURCE_DESC& Desc,
		const Color& ClearValue = Color(0.0f, 0.0f, 0.0f, 0.0f))
		:TextureBuffer(Buff, Barrier, SRVHandles, Desc)
	{
		handles.Initialize(RTV, RTVHandles);
		clearValue[0] = ClearValue.r;
		clearValue[1] = ClearValue.g;
		clearValue[2] = ClearValue.b;
		clearValue[3] = ClearValue.a;
	}

	RenderTarget(const std::shared_ptr<GPUResource>& GPUResource,
		const DescHandles& SRVHandles,
		const DescHandles& RTVHandles,
		const CD3DX12_RESOURCE_DESC& Desc,
		const Color& ClearValue = Color(0.0f, 0.0f, 0.0f, 0.0f))
		:TextureBuffer(GPUResource, SRVHandles, Desc)
	{
		handles.Initialize(RTV, RTVHandles);
		clearValue[0] = ClearValue.r;
		clearValue[1] = ClearValue.g;
		clearValue[2] = ClearValue.b;
		clearValue[3] = ClearValue.a;
	}


	//レンダーターゲットをクリア
	void Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	//リソースバリア変更
	void ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& Barrier) { resource->ChangeBarrier(CmdList, Barrier); }

	//レンダーターゲットとしてセットする準備
	const DescHandles& AsRTV(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		return handles.GetHandle(RTV);
	}
};

//デプスステンシル
class DepthStencil
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	DepthStencil() = delete;
	DepthStencil(const DepthStencil& tmp) = delete;
	DepthStencil(DepthStencil&& tmp) = delete;
private:
	const CD3DX12_RESOURCE_DESC desc;
	float clearValue = 1.0f;

	GPUResource resource;
	DescHandlesContainer handles;	//ディスクリプタハンドル

public:
	DepthStencil(const ComPtr<ID3D12Resource1>& Buff,
		const D3D12_RESOURCE_STATES& Barrier,
		const DescHandles& DSVHandles,
		const CD3DX12_RESOURCE_DESC& Desc,
		const float& ClearValue = 1.0f)
		:resource(Buff, Barrier), desc(Desc), clearValue(ClearValue)
	{
		handles.Initialize(DSV, DSVHandles);
	}

	//デプスステンシルをクリア
	void Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	//DSV取得
	const DescHandles& AsDSV(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		resource.ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		return handles.GetHandle(DSV);
	}
};

//頂点バッファ
class VertexBuffer
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	VertexBuffer() = delete;
	VertexBuffer(const VertexBuffer& tmp) = delete;
	VertexBuffer(VertexBuffer&& tmp) = delete;

	//頂点バッファ
	std::shared_ptr<GPUResource> resource;
	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	//読み取り専用
	std::shared_ptr<RWStructuredBuffer>rwBuff;
public:
	//頂点サイズ
	const size_t vertexSize;
	//生成した頂点数
	const unsigned int vertexNum;
	//送信する頂点数
	unsigned int sendVertexNum;

	VertexBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const D3D12_VERTEX_BUFFER_VIEW& VBView)
		:vbView(VBView), vertexSize(VBView.StrideInBytes), vertexNum(VBView.SizeInBytes / VBView.StrideInBytes), sendVertexNum(vertexNum) 
	{
		resource = std::make_shared<GPUResource>(Buff, Barrier);
	}
	VertexBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const D3D12_VERTEX_BUFFER_VIEW& VBView, const DescHandles& UAVHandle)
		:vbView(VBView), vertexSize(VBView.StrideInBytes), vertexNum(VBView.SizeInBytes / VBView.StrideInBytes), sendVertexNum(vertexNum)
	{
		resource = std::make_shared<GPUResource>(Buff, Barrier);
		rwBuff = std::make_shared<RWStructuredBuffer>(resource, UAVHandle, vertexSize, sendVertexNum);
	}
	void Mapping(void* SendData)
	{
		resource->Mapping(vertexSize, sendVertexNum, SendData);
	}
	void SetName(const wchar_t* Name)
	{
		resource->SetName(Name);
	}
	const D3D12_VERTEX_BUFFER_VIEW& GetVBView() { return vbView; }

	//読み取り専用構造化バッファ取得
	std::weak_ptr<RWStructuredBuffer>GetRWStructuredBuff()
	{
		KuroFunc::ErrorMessage(!rwBuff, "VertexBuffer", "GetRWStructuredBuff", "頂点バッファの描き込み用構造化バッファは未生成です\n");
		return rwBuff;
	}
	//頂点バッファとして使うためにリソースバリア変更
	void ChangeBarrierForVertexBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
};

//インデックスバッファ
class IndexBuffer
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	IndexBuffer() = delete;
	IndexBuffer(const IndexBuffer& tmp) = delete;
	IndexBuffer(IndexBuffer&& tmp) = delete;

	//インデックスバッファ
	GPUResource resource;
	//インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};

public:
	//インデックスサイズ
	const size_t indexSize;
	//インデックス数
	const unsigned int indexNum;

	IndexBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const D3D12_INDEX_BUFFER_VIEW& IBView, const size_t& IndexSize)
		:resource(Buff, Barrier), ibView(IBView), indexSize(IndexSize), indexNum(static_cast<unsigned int>(IBView.SizeInBytes / IndexSize)) {}
	void Mapping(void* SendData)
	{
		resource.Mapping(indexSize, indexNum, SendData);
	}
	void SetName(const wchar_t* Name)
	{
		resource.SetName(Name);
	}
	const D3D12_INDEX_BUFFER_VIEW& GetIBView() { return ibView; }
};

//シェーダー情報
class Shaders
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	ComPtr<ID3DBlob>vs;	//頂天シェーダー
	ComPtr<ID3DBlob>ps;	//ピクセルシェーダー
	ComPtr<ID3DBlob>ds;	//ドメインシェーダー
	ComPtr<ID3DBlob>hs;	//ハルシェーダー
	ComPtr<ID3DBlob>gs;	//ジオメトリシェーダー
};

//頂点レイアウトパラメータ
class InputLayoutParam
{
	InputLayoutParam() = delete;
public:
	const std::string semantics;
	const DXGI_FORMAT format;
	InputLayoutParam(const std::string& Semantics, const DXGI_FORMAT& Format) :semantics(Semantics), format(Format) {}
};


#define D3D12_DESCRIPTOR_RANGE_TYPE_NUM D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER+1
//ルートパラメータ(SRV,UAV,CBV,SAMPLER のGPUデータ指定）
class RootParam
{
private:
	RootParam() = delete;

public:
	std::string comment;		//注釈　あってもなくても良い
	D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType;
	DESC_HANDLE_TYPE viewType;
	bool descriptor = false;	//ディスクリプタとして登録されているか
	int descNum = 1;

	RootParam(const D3D12_DESCRIPTOR_RANGE_TYPE& Range, const char* Comment = nullptr, const int& DescNum = 1)
		:descriptorRangeType(Range), descriptor(true), descNum(DescNum) {
		if (Comment != nullptr)comment = Comment;
	}
	RootParam(const DESC_HANDLE_TYPE& ViewType, const char* Comment = nullptr)
		:viewType(ViewType) {
		KuroFunc::ErrorMessage(viewType == RTV || viewType == DSV, "RootParam", "コンストラクタ", "ルートパラメータで RTV / DSV は設定できません\n");
		if (Comment != nullptr)comment = Comment;
	}
};

//サンプラー
class WrappedSampler
{
	void Generate(const D3D12_TEXTURE_ADDRESS_MODE& TexAddressMode, const D3D12_FILTER& Filter)
	{
		sampler.AddressU = TexAddressMode;
		sampler.AddressV = TexAddressMode;
		sampler.AddressW = TexAddressMode;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		sampler.Filter = Filter;	//補間
		sampler.MaxLOD = D3D12_FLOAT32_MAX;	//ミップマップ最大値
		sampler.MinLOD = 0.0f;	//ミップマップ最小値
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		sampler.ShaderRegister = 0;
	}
public:
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	WrappedSampler(const D3D12_TEXTURE_ADDRESS_MODE& TexAddressMode, const D3D12_FILTER& Filter)
	{
		Generate(TexAddressMode, Filter);
	}
	//繰り返すか、補間をかけるか
	WrappedSampler(const bool& Wrap,const bool& Interpolation)
	{
		auto addressMode = !Wrap ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		auto interpolation = Interpolation ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_POINT;
		Generate(addressMode, interpolation);
	}
	operator D3D12_STATIC_SAMPLER_DESC() { return sampler; }
};

//パイプライン各種設定
class PipelineInitializeOption
{
	PipelineInitializeOption() = delete;
public:
	bool calling = true;	//カリング
	bool wireFrame = false;	//ワイヤーフレーム
	bool depthTest = true;	//深度テスト
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;	//デプスステンシルのフォーマット
	bool depthWriteMask = true;	//デプスの書き込み（深度テストを行う場合）
	bool independetBlendEnable = true;		//同時レンダーターゲットで独立したブレンディングを有効にするか
	bool frontCounterClockWise = false;	//三角形の表がどちらか決める際の向き


	D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;	//図形の形状設定

	//パイプラインの名前で区別するので、必ず命名する
	PipelineInitializeOption(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& TopologyType, const D3D_PRIMITIVE_TOPOLOGY& Topology)
		:primitiveTopologyType(TopologyType), primitiveTopology(Topology) {}
};

//アルファブレンディングモード
enum AlphaBlendMode
{
	AlphaBlendMode_None,	//アルファブレンディングなし(上書き)。
	AlphaBlendMode_Trans,	//半透明合成
	AlphaBlendMode_Add,		//加算合成
	AlphaBlendModeNum
};

//書き込み先レンダーターゲット情報
class RenderTargetInfo
{
	RenderTargetInfo() = delete;
public:
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	AlphaBlendMode blendMode = AlphaBlendMode_None;
	RenderTargetInfo(const DXGI_FORMAT& Format, const AlphaBlendMode& BlendMode) :format(Format), blendMode(BlendMode) {}
};

//グラフィックスパイプライン情報
class GraphicsPipeline
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	static int PIPELINE_NUM;	//パイプラインが生成された数（＝識別番号）

private:
	int	handle = -1;	//パイプライン識別番号

	ComPtr<ID3D12PipelineState>pipeline;			//パイプライン
	ComPtr<ID3D12RootSignature>rootSignature;	//ルートシグネチャ
	D3D_PRIMITIVE_TOPOLOGY topology;
public:
	GraphicsPipeline(const ComPtr<ID3D12PipelineState>& Pipeline, const ComPtr<ID3D12RootSignature>& RootSignature, const D3D_PRIMITIVE_TOPOLOGY& Topology)
		:pipeline(Pipeline), rootSignature(RootSignature), topology(Topology), handle(PIPELINE_NUM++) {}

	void SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	const int& GetPipelineHandle() { return handle; }
};

//コンピュートパイプライン
class ComputePipeline
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	static int PIPELINE_NUM;	//パイプラインが生成された数（＝識別番号）

private:
	int	handle = -1;	//パイプライン識別番号

	ComPtr<ID3D12PipelineState>pipeline;			//パイプライン
	ComPtr<ID3D12RootSignature>rootSignature;	//ルートシグネチャ
public:
	ComputePipeline(const ComPtr<ID3D12PipelineState>& Pipeline, const ComPtr<ID3D12RootSignature>& RootSignature)
		:pipeline(Pipeline), rootSignature(RootSignature), handle(PIPELINE_NUM++) {}

	void SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	const int& GetPipelineHandle() { return handle; }
};

template<int GpuAddressNum>
class IndirectCommand
{
public:
	static size_t GetSize()
	{
		return sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * GpuAddressNum + sizeof(D3D12_DRAW_ARGUMENTS);
	}

public:
	//各コマンドでの描画で使用するバッファ
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddressArray[GpuAddressNum];
	//通常描画の引数に使われるパラメータ
	D3D12_DRAW_ARGUMENTS drawArgs;
};

enum EXCUTE_INDIRECT_TYPE { DRAW, DRAW_INDEXED, DISPATCH, EXCUTE_INDIRECT_TYPE_NUM };
class IndirectDevice
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
private:
	//コマンドシグネチャ
	ComPtr<ID3D12CommandSignature>cmdSignature;
	//１つの描画コマンドにつき使用するGPUバッファの数
	int gpuBuffNum;
	//カウントバッファー
	ComPtr<ID3D12Resource>countBuffer;
	//カウントリセット用コピー元バッファ
	ComPtr<ID3D12Resource>countResetBuffer;

public:
	IndirectDevice(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12CommandSignature>& CmdSignature, const int& GPUBufferNum);

	void Excute(const ComPtr<ID3D12GraphicsCommandList>& CmdList,
		int MaxCommandCount,
		ID3D12Resource* ArgBuffer, UINT ArgBufferOffset,
		ID3D12Resource* CountBuffer, UINT CountBufferOffset);
};