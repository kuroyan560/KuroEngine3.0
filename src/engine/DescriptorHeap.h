#pragma once
#include<d3d12.h>
#include<wrl.h>
#include"d3dx12.h"
#include <cassert>
//ディスクリプタヒープ基底クラス
class DescriptorHeapBase
{
protected:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
	static const int MAX_SRV_CBV_UAV = 2048;
	static const int MAX_SAMPLER = 100;
	static const int MAX_RTV = 100;
	static const int MAX_DSV = 100;

protected:
	DescriptorHeapBase(ComPtr<ID3D12Device> Device, const D3D12_DESCRIPTOR_HEAP_TYPE& Type, const int& MaxDescriptorNum);

	ComPtr<ID3D12DescriptorHeap>heap;	//ディスクリプタヒープ
	D3D12_CPU_DESCRIPTOR_HANDLE headHandleCpu;	//ヒープ先頭のCPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE headHandleGpu;	//ヒープ先頭のGPUハンドル
	UINT incrementSize;	//１つのディスクリプタのサイズ（ヒープタイプに依存）

	int num = 0;	//生成済ディスクリプタの数
	void OnCreateView() { num++; }

public:
	//ディスクリプタヒープゲッタ
	const ComPtr<ID3D12DescriptorHeap>&GetHeap() { return heap; }
	//GPUハンドルゲッタ
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const int& Index)	//場所指定
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(headHandleGpu, Index, incrementSize);
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuHandleTail()		//最後に生成されたディスクリプタハンドル
	{
		if (num == 0)
		{
			printf("まだディスクリプタハンドルが生成されていません\n");
			assert(0);
		}
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(headHandleGpu, num - 1, incrementSize);
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuHandleEnd()		//最後尾
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(headHandleGpu, num , incrementSize);
	}

	//CPUハンドルゲッタ
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const int& Index)	//場所指定
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(headHandleCpu, Index, incrementSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuHandleTail()		//最後に生成されたディスクリプタハンドル
	{
		if (num == 0)
		{
			printf("まだディスクリプタハンドルが生成されていません\n");
			assert(0);
		}
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(headHandleCpu, num - 1, incrementSize);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuHandleEnd()	//最後尾
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(headHandleCpu, num, incrementSize);
	}
};

//D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
class DescriptorHeap_CBV_SRV_UAV : public DescriptorHeapBase
{
	static const int MAX_DESCRIPTOR_NUM = 2048;
public:
	DescriptorHeap_CBV_SRV_UAV(ComPtr<ID3D12Device> Device)
		:DescriptorHeapBase(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAX_DESCRIPTOR_NUM) {}

	//定数バッファビュー生成
	void CreateCBV(const ComPtr<ID3D12Device>& Device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& Desc);
	void CreateCBV(const ComPtr<ID3D12Device>& Device, const D3D12_GPU_VIRTUAL_ADDRESS& BuffGpuAddress, const size_t& SizeInBytes)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
		desc.BufferLocation = BuffGpuAddress;
		desc.SizeInBytes = static_cast<UINT>(SizeInBytes);
		CreateCBV(Device, desc);
	}

	//シェーダーリソースビュー生成
	void CreateSRV(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12Resource>& Buff, const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc);
	//テクスチャバッファ
	void CreateSRV(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12Resource>& Buff, const DXGI_FORMAT& Format)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};	//設定構造体
		srvDesc.Format = Format;	//RGBA
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	//2Dテクスチャ
		srvDesc.Texture2D.MipLevels = 1;
		CreateSRV(Device, Buff, srvDesc);
	}
	//構造化バッファ
	void CreateSRV(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12Resource>& Buff, const size_t& DataSize, const int& ElementNum)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};	//設定構造体
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(ElementNum);
		srvDesc.Buffer.StructureByteStride = static_cast<UINT>(DataSize);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		CreateSRV(Device, Buff, srvDesc);
	}

	//アンオーダードアクセスビュー生成
	void CreateUAV(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12Resource>& Buff, const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc);
	void CreateUAV(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12Resource>& Buff, const size_t& DataSize, const int& ElementNum,
		const D3D12_UAV_DIMENSION& Dimension = D3D12_UAV_DIMENSION_BUFFER, const DXGI_FORMAT& Format = DXGI_FORMAT_UNKNOWN)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
		desc.ViewDimension = Dimension;
		desc.Format = Format;
		desc.Buffer.NumElements = static_cast<UINT>(ElementNum);
		desc.Buffer.StructureByteStride = static_cast<UINT>(DataSize);
		CreateUAV(Device, Buff, desc);
	}
};

//D3D12_DESCRIPTOR_HEAP_TYPE_RTV
class DescriptorHeap_RTV : public DescriptorHeapBase
{
	static const int MAX_DESCRIPTOR_NUM = 2048;
public:
	DescriptorHeap_RTV(ComPtr<ID3D12Device> Device)
		:DescriptorHeapBase(Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, MAX_DESCRIPTOR_NUM) {}

	void CreateRTV(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12Resource>& Buff, const D3D12_RENDER_TARGET_VIEW_DESC* ViewDesc = nullptr);
};

//D3D12_DESCRIPTOR_HEAP_TYPE_DSV
class DescriptorHeap_DSV : public DescriptorHeapBase
{
	static const int MAX_DESCRIPTOR_NUM = 100;
public:
	DescriptorHeap_DSV(ComPtr<ID3D12Device> Device)
		:DescriptorHeapBase(Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, MAX_DESCRIPTOR_NUM) {}

	void CreateDSV(const ComPtr<ID3D12Device>& Device, const ComPtr<ID3D12Resource>& Buff, const D3D12_DEPTH_STENCIL_VIEW_DESC* ViewDesc = nullptr);
};