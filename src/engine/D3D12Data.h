#pragma once
#include<d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include<dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")
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
	bool m_invalid = true;
	//CPUハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
	//GPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;

public:
	//コンストラクタ
	DescHandles() {}
	DescHandles(const DescHandles& tmp)
	{
		assert(tmp);
		Initialize(tmp.m_cpuHandle, tmp.m_gpuHandle);
	}
	DescHandles(DescHandles&& tmp)
	{
		assert(tmp);
		Initialize(tmp.m_cpuHandle, tmp.m_gpuHandle);
	}
	DescHandles(const D3D12_CPU_DESCRIPTOR_HANDLE& CPUHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& GPUHandle) { Initialize(CPUHandle, GPUHandle); }

	//初期化
	void Initialize(const D3D12_CPU_DESCRIPTOR_HANDLE& CPUHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& GPUHandle)
	{
		m_cpuHandle = CPUHandle;
		m_gpuHandle = GPUHandle;
		m_invalid = false;
	}

	// if 文にそのままかけて初期済みか判定出来るように
	operator bool()const { return !m_invalid; }
	//代入演算子
	void operator=(const DescHandles& rhs)
	{
		assert(!rhs.m_invalid);
		m_cpuHandle = rhs.m_cpuHandle;
		m_gpuHandle = rhs.m_gpuHandle;
		m_invalid = false;
	}

	operator const D3D12_CPU_DESCRIPTOR_HANDLE& ()const
	{
		assert(!m_invalid);
		return m_cpuHandle;
	}
	operator const D3D12_CPU_DESCRIPTOR_HANDLE* ()const
	{
		assert(!m_invalid);
		return &m_cpuHandle;
	}
	operator const D3D12_GPU_DESCRIPTOR_HANDLE& ()const
	{
		assert(!m_invalid);
		return m_gpuHandle;
	}
	operator const D3D12_GPU_DESCRIPTOR_HANDLE* ()const
	{
		assert(!m_invalid);
		return &m_gpuHandle;
	}
};

class DescHandlesContainer
{
private:
	DescHandles m_handles[DESC_HANDLE_NUM];

public:
	void Initialize(const DESC_HANDLE_TYPE& Type, const DescHandles& Handle)
	{
		m_handles[Type] = Handle;
	}
	const DescHandles& GetHandle(const DESC_HANDLE_TYPE& Type) { return m_handles[Type]; }
};

//GPUリソース（データコンテナ）
class GPUResource
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	GPUResource() = delete;
	GPUResource(const GPUResource& tmp) = delete;
	GPUResource(GPUResource&& tmp) = delete;

	void* m_buffOnCPU = nullptr;//CPU側からアクセスできるするバッファのアドレス
	bool m_mapped = false;
	ComPtr<ID3D12Resource>m_buff = nullptr;	//リソースバッファ
	D3D12_RESOURCE_STATES m_barrier = D3D12_RESOURCE_STATE_COMMON;	//リソースバリアの状態

public:
	GPUResource(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier,const wchar_t* Name = nullptr) 
	{
		m_buff = Buff;
		if (Name != nullptr)m_buff.Get()->SetName(Name);

		m_barrier = Barrier;
	}
	~GPUResource()
	{
		if (m_mapped)m_buff->Unmap(0, nullptr);
	}

	//バッファ名のセッタ
	void SetName(const wchar_t* Name)
	{
		m_buff->SetName(Name);
	}

	//バッファ取得
	const ComPtr<ID3D12Resource>& GetBuff() { return m_buff; }
	//現在のリソースバリア取得
	const D3D12_RESOURCE_STATES& GetResourceBarrier() { return m_barrier; }

	//マッピング
	void Mapping(const size_t& DataSize, const int& ElementNum, const void* SendData);
	//リソースバリアの変更
	void ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& NewBarrier);
	//バッファのコピー（インスタンスは別物、引数にコピー元）
	void CopyGPUResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, GPUResource& CopySource);

	template<typename T>
	T* GetBuffOnCpu() { return (T*) m_buffOnCPU; }
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

	std::shared_ptr<GPUResource> m_resource;	//GPUバッファ
	DescHandlesContainer m_handles;	//ディスクリプタハンドル

	//バッファセットのタイミングで呼ばれる関数、リソースバリアを変えるなど
	virtual void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type) {};

public:
	DescriptorData(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier) :m_resource(std::make_shared<GPUResource>(Buff, Barrier)) {}
	DescriptorData(const std::shared_ptr<GPUResource>& GPUResource) :m_resource(GPUResource) {}	//同じものを差す
	//バッファセット
	void SetGraphicsDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam);
	void SetComputeDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type, const int& RootParam);
	std::shared_ptr<GPUResource>& GetResource() { return m_resource; }

	void InitDescHandle(const DESC_HANDLE_TYPE& Type, const DescHandles& Handle)
	{
		m_handles.Initialize(Type, Handle);
	}
};

//定数バッファ
class ConstantBuffer : public DescriptorData
{
	ConstantBuffer() = delete;
	ConstantBuffer(const ConstantBuffer& tmp) = delete;
	ConstantBuffer(ConstantBuffer&& tmp) = delete;
private:
	const size_t m_dataSize;	//ユーザ定義データの１要素サイズ
	const int m_elementNum = 0;	//要素数

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

public:
	ConstantBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& CBVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(Buff, Barrier), m_dataSize(DataSize), m_elementNum(ElementNum) 
	{
		m_handles.Initialize(CBV, CBVHandles);
	}
	void Mapping(const void* SendData)
	{
		m_resource->Mapping(m_dataSize, m_elementNum, SendData);
	}
};

//構造化バッファ（見かけ上は定数バッファと同じ）
class StructuredBuffer : public DescriptorData
{
	StructuredBuffer() = delete;
	StructuredBuffer(const StructuredBuffer& tmp) = delete;
	StructuredBuffer(StructuredBuffer&& tmp) = delete;

private:
	const size_t m_dataSize;	//ユーザ定義データの１要素サイズ
	const int m_elementNum = 0;	//要素数

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
public:
	StructuredBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& SRVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(Buff, Barrier), m_dataSize(DataSize), m_elementNum(ElementNum) 
	{
		m_handles.Initialize(SRV, SRVHandles);
	}
	void Mapping(void* SendData)
	{
		m_resource->Mapping(m_dataSize, m_elementNum, SendData);
	}
};

//出力先バッファ
class RWStructuredBuffer : public DescriptorData
{
	RWStructuredBuffer() = delete;
	RWStructuredBuffer(const RWStructuredBuffer& tmp) = delete;
	RWStructuredBuffer(RWStructuredBuffer&& tmp) = delete;

private:
	const size_t m_dataSize;	//ユーザ定義データの１要素サイズ
	const int m_elementNum = 0;	//要素数

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
public:
	RWStructuredBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& UAVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(Buff, Barrier), m_dataSize(DataSize), m_elementNum(ElementNum) 
	{
		m_handles.Initialize(UAV, UAVHandles);
	}
	RWStructuredBuffer(const std::shared_ptr<GPUResource>& GPUResource, const DescHandles& UAVHandles, const size_t& DataSize, const int& ElementNum)
		:DescriptorData(GPUResource), m_dataSize(DataSize), m_elementNum(ElementNum)
	{
		m_handles.Initialize(UAV, UAVHandles);
	}

	void CopyBuffOnGPU(const ComPtr<ID3D12GraphicsCommandList>& CmdList, GPUResource& Dest) { Dest.CopyGPUResource(CmdList, *this->m_resource); }

	void Mapping(void* SendData)
	{
		m_resource->Mapping(m_dataSize, m_elementNum, SendData);
	}
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
	CD3DX12_RESOURCE_DESC m_texDesc;	//テクスチャ情報（幅、高さなど）

	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

public:
	TextureBuffer(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& SRVHandles, const CD3DX12_RESOURCE_DESC& Desc)
		:DescriptorData(Buff, Barrier), m_texDesc(Desc) 
	{
		m_handles.Initialize(SRV, SRVHandles);
	}
	TextureBuffer(const ComPtr<ID3D12Resource>& Buff, const D3D12_RESOURCE_STATES& Barrier, const DescHandles& SRVHandles, const D3D12_RESOURCE_DESC& Desc)
		:DescriptorData(Buff, Barrier), m_texDesc(CD3DX12_RESOURCE_DESC(Desc)) 
	{
		m_handles.Initialize(SRV, SRVHandles);
	}
	TextureBuffer(const std::shared_ptr<GPUResource>& GPUResource, const DescHandles& SRVHandles, const D3D12_RESOURCE_DESC& Desc)
	: DescriptorData(GPUResource), m_texDesc(CD3DX12_RESOURCE_DESC(Desc))
	{
		m_handles.Initialize(SRV, SRVHandles);
	}

	void ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& Barrier) { m_resource->ChangeBarrier(CmdList, Barrier); }
	void CopyTexResource(const ComPtr<ID3D12GraphicsCommandList>& CmdList, TextureBuffer* CopySource);
	const CD3DX12_RESOURCE_DESC& GetDesc() { return m_texDesc; }
	void SetUAVHandles(const DescHandles& UAVHandles) { m_handles.Initialize(UAV, UAVHandles); }
	Vec2<int>GetGraphSize()
	{
		return Vec2<int>(static_cast<int>(m_texDesc.Width), static_cast<int>(m_texDesc.Height));
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
	float m_clearValue[4] = { 0.0f };	//クリア値
	//ピクセルシェーダーリソースとして使われる
	void OnSetDescriptorBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const DESC_HANDLE_TYPE& Type)override
	{
		m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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
		m_handles.Initialize(RTV, RTVHandles);
		m_clearValue[0] = ClearValue.m_r;
		m_clearValue[1] = ClearValue.m_g;
		m_clearValue[2] = ClearValue.m_b;
		m_clearValue[3] = ClearValue.m_a;
	}

	RenderTarget(const std::shared_ptr<GPUResource>& GPUResource,
		const DescHandles& SRVHandles,
		const DescHandles& RTVHandles,
		const CD3DX12_RESOURCE_DESC& Desc,
		const Color& ClearValue = Color(0.0f, 0.0f, 0.0f, 0.0f))
		:TextureBuffer(GPUResource, SRVHandles, Desc)
	{
		m_handles.Initialize(RTV, RTVHandles);
		m_clearValue[0] = ClearValue.m_r;
		m_clearValue[1] = ClearValue.m_g;
		m_clearValue[2] = ClearValue.m_b;
		m_clearValue[3] = ClearValue.m_a;
	}


	//レンダーターゲットをクリア
	void Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	//リソースバリア変更
	void ChangeBarrier(const ComPtr<ID3D12GraphicsCommandList>& CmdList, const D3D12_RESOURCE_STATES& Barrier) { m_resource->ChangeBarrier(CmdList, Barrier); }

	//レンダーターゲットとしてセットする準備
	const DescHandles& AsRTV(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		return m_handles.GetHandle(RTV);
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
	const CD3DX12_RESOURCE_DESC m_desc;
	float m_clearValue = 1.0f;

	GPUResource m_resource;
	DescHandlesContainer m_handles;	//ディスクリプタハンドル

public:
	DepthStencil(const ComPtr<ID3D12Resource1>& Buff,
		const D3D12_RESOURCE_STATES& Barrier,
		const DescHandles& DSVHandles,
		const CD3DX12_RESOURCE_DESC& Desc,
		const float& ClearValue = 1.0f)
		:m_resource(Buff, Barrier), m_desc(Desc), m_clearValue(ClearValue)
	{
		m_handles.Initialize(DSV, DSVHandles);
	}

	//デプスステンシルをクリア
	void Clear(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	//DSV取得
	const DescHandles& AsDSV(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		m_resource.ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		return m_handles.GetHandle(DSV);
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
	std::shared_ptr<GPUResource> m_resource;
	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW m_vbView = {};
	//読み取り専用
	std::shared_ptr<RWStructuredBuffer>m_rwBuff;
public:
	//頂点サイズ
	const size_t m_vertexSize;
	//生成した頂点数
	const unsigned int m_vertexNum;
	//送信する頂点数
	unsigned int m_sendVertexNum;

	VertexBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const D3D12_VERTEX_BUFFER_VIEW& VBView)
		:m_vbView(VBView), m_vertexSize(VBView.StrideInBytes), m_vertexNum(VBView.SizeInBytes / VBView.StrideInBytes), m_sendVertexNum(m_vertexNum) 
	{
		m_resource = std::make_shared<GPUResource>(Buff, Barrier);
	}
	VertexBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const D3D12_VERTEX_BUFFER_VIEW& VBView, const DescHandles& UAVHandle)
		:m_vbView(VBView), m_vertexSize(VBView.StrideInBytes), m_vertexNum(VBView.SizeInBytes / VBView.StrideInBytes), m_sendVertexNum(m_vertexNum)
	{
		m_resource = std::make_shared<GPUResource>(Buff, Barrier);
		m_rwBuff = std::make_shared<RWStructuredBuffer>(m_resource, UAVHandle, m_vertexSize, m_sendVertexNum);
	}
	void Mapping(void* SendData)
	{
		m_resource->Mapping(m_vertexSize, m_sendVertexNum, SendData);
	}
	void SetName(const wchar_t* Name)
	{
		m_resource->SetName(Name);
	}

	//読み取り専用構造化バッファ取得
	std::weak_ptr<RWStructuredBuffer>GetRWStructuredBuff()
	{
		assert(m_rwBuff);
		return m_rwBuff;
	}
	//頂点バッファとして使うためにリソースバリア変更
	void ChangeBarrierForVertexBuffer(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		m_resource->ChangeBarrier(CmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	void SetView(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		CmdList->IASetVertexBuffers(0, 1, &m_vbView);
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
	GPUResource m_resource;
	//インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW m_ibView = {};

public:
	//インデックスサイズ
	const size_t m_indexSize;
	//インデックス数
	const unsigned int m_indexNum;

	IndexBuffer(const ComPtr<ID3D12Resource1>& Buff, const D3D12_RESOURCE_STATES& Barrier, const D3D12_INDEX_BUFFER_VIEW& IBView, const size_t& IndexSize)
		:m_resource(Buff, Barrier), m_ibView(IBView), m_indexSize(IndexSize), m_indexNum(static_cast<unsigned int>(IBView.SizeInBytes / IndexSize)) {}
	void Mapping(void* SendData)
	{
		m_resource.Mapping(m_indexSize, m_indexNum, SendData);
	}
	void SetName(const wchar_t* Name)
	{
		m_resource.SetName(Name);
	}
	void SetView(const ComPtr<ID3D12GraphicsCommandList>& CmdList)
	{
		CmdList->IASetIndexBuffer(&m_ibView);
	}
};

//シェーダー情報
class Shaders
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	ComPtr<IDxcBlob>m_vs;	//頂点シェーダー
	ComPtr<IDxcBlob>m_ps;	//ピクセルシェーダー
	ComPtr<IDxcBlob>m_ds;	//ドメインシェーダー
	ComPtr<IDxcBlob>m_hs;	//ハルシェーダー
	ComPtr<IDxcBlob>m_gs;	//ジオメトリシェーダー
};

//頂点レイアウトパラメータ
class InputLayoutParam
{
	InputLayoutParam() = delete;
public:
	const std::string m_semantics;
	const DXGI_FORMAT m_format;
	InputLayoutParam(const std::string& Semantics, const DXGI_FORMAT& Format) :m_semantics(Semantics), m_format(Format) {}
};


#define D3D12_DESCRIPTOR_RANGE_TYPE_NUM D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER+1
//ルートパラメータ(SRV,UAV,CBV,SAMPLER のGPUデータ指定）
class RootParam
{
private:
	RootParam() = delete;

public:
	std::string m_comment;		//注釈　あってもなくても良い
	D3D12_DESCRIPTOR_RANGE_TYPE m_descriptorRangeType;
	DESC_HANDLE_TYPE m_viewType;
	bool m_descriptor = false;	//ディスクリプタとして登録されているか
	int m_descNum = 1;

	RootParam(const D3D12_DESCRIPTOR_RANGE_TYPE& Range, const char* Comment = nullptr, const int& DescNum = 1)
		:m_descriptorRangeType(Range), m_descriptor(true), m_descNum(DescNum) {
		if (Comment != nullptr)m_comment = Comment;
		switch (Range)
		{
		case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
			m_viewType = CBV;
			break;
		case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
			m_viewType = SRV;
			break;
		case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
			m_viewType = UAV;
			break;
		default:
			break;
		}
	}
	RootParam(const DESC_HANDLE_TYPE& ViewType, const char* Comment = nullptr)
		:m_viewType(ViewType) {
		if (Comment != nullptr)m_comment = Comment;
		switch (ViewType)
		{
		case CBV:
			m_descriptorRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			break;
		case SRV:
			m_descriptorRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			break;
		case UAV:
			m_descriptorRangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			break;
		default:
			assert(0);
			break;
		}
	}
};

//サンプラー
class WrappedSampler
{
	void Generate(const D3D12_TEXTURE_ADDRESS_MODE& TexAddressMode, const D3D12_FILTER& Filter)
	{
		m_sampler.AddressU = TexAddressMode;
		m_sampler.AddressV = TexAddressMode;
		m_sampler.AddressW = TexAddressMode;
		m_sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		m_sampler.Filter = Filter;	//補間
		m_sampler.MaxLOD = D3D12_FLOAT32_MAX;	//ミップマップ最大値
		m_sampler.MinLOD = 0.0f;	//ミップマップ最小値
		m_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		m_sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		m_sampler.ShaderRegister = 0;
	}
public:
	D3D12_STATIC_SAMPLER_DESC m_sampler = {};
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
	operator D3D12_STATIC_SAMPLER_DESC() { return m_sampler; }
};

//パイプライン各種設定
class PipelineInitializeOption
{
	PipelineInitializeOption() = delete;
public:
	bool m_calling = true;	//カリング
	bool m_wireFrame = false;	//ワイヤーフレーム
	bool m_depthTest = true;	//深度テスト
	DXGI_FORMAT m_dsvFormat = DXGI_FORMAT_D32_FLOAT;	//デプスステンシルのフォーマット
	bool m_depthWriteMask = true;	//デプスの書き込み（深度テストを行う場合）
	bool m_independetBlendEnable = true;		//同時レンダーターゲットで独立したブレンディングを有効にするか
	bool m_frontCounterClockWise = false;	//三角形の表がどちらか決める際の向き


	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	D3D_PRIMITIVE_TOPOLOGY m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;	//図形の形状設定

	//パイプラインの名前で区別するので、必ず命名する
	PipelineInitializeOption(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& TopologyType, const D3D_PRIMITIVE_TOPOLOGY& Topology)
		:m_primitiveTopologyType(TopologyType), m_primitiveTopology(Topology) {}
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
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
	AlphaBlendMode m_blendMode = AlphaBlendMode_None;
	RenderTargetInfo(const DXGI_FORMAT& Format, const AlphaBlendMode& BlendMode) :m_format(Format), m_blendMode(BlendMode) {}
};

//グラフィックスパイプライン情報
class GraphicsPipeline
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	static int s_pipelineNum;	//パイプラインが生成された数（＝識別番号）

private:
	int	m_handle = -1;	//パイプライン識別番号

	ComPtr<ID3D12PipelineState>m_pipeline;			//パイプライン
	ComPtr<ID3D12RootSignature>m_rootSignature;	//ルートシグネチャ
	D3D_PRIMITIVE_TOPOLOGY m_topology;
public:
	GraphicsPipeline(const ComPtr<ID3D12PipelineState>& Pipeline, const ComPtr<ID3D12RootSignature>& RootSignature, const D3D_PRIMITIVE_TOPOLOGY& Topology)
		:m_pipeline(Pipeline), m_rootSignature(RootSignature), m_topology(Topology), m_handle(s_pipelineNum++) {}

	void SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	const int& GetPipelineHandle() { return m_handle; }
};

//コンピュートパイプライン
class ComputePipeline
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	static int s_pipelineNum;	//パイプラインが生成された数（＝識別番号）

private:
	int	m_handle = -1;	//パイプライン識別番号

	ComPtr<ID3D12PipelineState>m_pipeline;			//パイプライン
	ComPtr<ID3D12RootSignature>m_rootSignature;	//ルートシグネチャ
public:
	ComputePipeline(const ComPtr<ID3D12PipelineState>& Pipeline, const ComPtr<ID3D12RootSignature>& RootSignature)
		:m_pipeline(Pipeline), m_rootSignature(RootSignature), m_handle(s_pipelineNum++) {}

	void SetPipeline(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	const int& GetPipelineHandle() { return m_handle; }
};

template<int GpuAddressNum>
class IndirectDrawCommand
{
public:
	//各コマンドでの描画で使用するバッファ
	std::array<D3D12_GPU_VIRTUAL_ADDRESS, GpuAddressNum>m_gpuAddressArray;
	//DrawInstanced の引数にあたるパラメータ
	D3D12_DRAW_ARGUMENTS m_drawArgs;
};

template<int GpuAddressNum>
class IndirectDrawIndexedCommand
{
public:
	//各コマンドでの描画で使用するバッファ
	std::array<D3D12_GPU_VIRTUAL_ADDRESS, GpuAddressNum>m_gpuAddressArray;
	//DrawIndexedInstanced の引数にあたるパラメータ
	D3D12_DRAW_INDEXED_ARGUMENTS m_drawArgs;
};

template<int GpuAddressNum>
class IndirectDispatchCommand
{
public:
	//各コマンドでの描画で使用するバッファ
	std::array<D3D12_GPU_VIRTUAL_ADDRESS, GpuAddressNum>m_gpuAddressArray;
	//Dispatch の引数にあたるパラメータ
	D3D12_DISPATCH_ARGUMENTS m_dispatchArgs;
};

enum EXCUTE_INDIRECT_TYPE { DRAW, DRAW_INDEXED, DISPATCH, EXCUTE_INDIRECT_TYPE_NUM, NONE };
class IndirectCommandBuffer
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	friend class IndirectDevice;

private:
	//Indirectの形式
	EXCUTE_INDIRECT_TYPE m_indirectType = EXCUTE_INDIRECT_TYPE::NONE;
	//コマンドの最大数
	int m_maxCommandCount = 0;
	//コマンドバッファ
	std::shared_ptr<DescriptorData>m_buff;
	//カウンターバッファ
	std::shared_ptr<GPUResource>m_counterBuffer;
	//コマンド１つあたりにサイズ
	size_t m_commandSize;
public:
	IndirectCommandBuffer(EXCUTE_INDIRECT_TYPE IndirectType, int MaxCommandCount, size_t CommandSize,
		std::shared_ptr<DescriptorData>Buff, std::shared_ptr<GPUResource>CounterBuffer = nullptr)
		:m_indirectType(IndirectType), m_maxCommandCount(MaxCommandCount), m_commandSize(CommandSize), m_buff(Buff), m_counterBuffer(CounterBuffer)
	{
		assert(DRAW <= IndirectType && IndirectType < EXCUTE_INDIRECT_TYPE_NUM);
	}
	void ResetCounterBuffer();

	//ゲッタ
	const EXCUTE_INDIRECT_TYPE& IndirectType() { return m_indirectType; }
	const std::shared_ptr<DescriptorData>& GetBuff()
	{
		return m_buff;
	}
	const size_t& GetCommandSize() { return m_commandSize; }
};


class IndirectDevice
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	//Indirectの形式
	EXCUTE_INDIRECT_TYPE m_indirectType = NONE;
	//コマンドシグネチャ
	ComPtr<ID3D12CommandSignature>m_cmdSignature;

public:
	IndirectDevice(EXCUTE_INDIRECT_TYPE IndirectType,const ComPtr<ID3D12CommandSignature>& CmdSignature) 
		:m_indirectType(IndirectType), m_cmdSignature(CmdSignature) 
	{
		assert(DRAW <= IndirectType && IndirectType < EXCUTE_INDIRECT_TYPE_NUM);
	}

	void Execute(const ComPtr<ID3D12GraphicsCommandList>& CmdList, std::shared_ptr<IndirectCommandBuffer>CmdBuff, UINT ArgBufferOffset = 0);

	const EXCUTE_INDIRECT_TYPE& IndirectType() { return m_indirectType; }
};