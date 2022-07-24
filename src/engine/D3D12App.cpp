#include "D3D12App.h"
#include"KuroFunc.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include<array>

D3D12App* D3D12App::s_instance = nullptr;
std::map<std::string, D3D12App::LoadLambda_t> D3D12App::s_loadLambdaTable;

void D3D12App::Initialize(const HWND& Hwnd, const Vec2<int>& ScreenSize, const bool& UseHDR, const Color& ClearValue, const bool& IsFullScreen)
{
//デバッグレイヤー
#ifdef _DEBUG
	//デバッグレイヤーをオンに
	ComPtr<ID3D12Debug> spDebugController0;
	ComPtr<ID3D12Debug1> spDebugController1;
	D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0));
	spDebugController0->QueryInterface(IID_PPV_ARGS(&spDebugController1));
	spDebugController1->SetEnableGPUBasedValidation(true);
#endif

	HRESULT result;

	//グラフィックスボードのアダプタを列挙
	//DXGIファクトリーの生成
	result = CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory));
	//アダプターの列挙用
	std::vector<ComPtr<IDXGIAdapter>>adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	ComPtr<IDXGIAdapter> tmpAdapter = nullptr;
	for (int i = 0; m_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		result = m_dxgiFactory->EnumAdapters(i, &tmpAdapter);
		adapters.push_back(tmpAdapter);		//動的配列に追加する
	}
	for (int i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC adesc{};
		result = adapters[i]->GetDesc(&adesc);	//アダプターの情報を取得
		std::wstring strDesc = adesc.Description;	//アダプター名
		//Microsoft Basic Render Driver,Intel UHD Graphics を回避
		if (strDesc.find(L"Microsoft") == std::wstring::npos
			&& strDesc.find(L"Intel") == std::wstring::npos)
		{
			tmpAdapter = adapters[i];	//採用
			break;
		}
	}

	//デバイスの生成（Direct3D12の基本オブジェクト）
	//対応レベルの配列
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;

	for (int i = 0; i < _countof(levels); i++)
	{
		//採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter.Get(), levels[i], IID_PPV_ARGS(&m_device));
		if (result == S_OK)
		{
			//デバイスを生成できた時点でループを抜ける
			featureLevel = levels[i];
			break;
		}
	}

	m_descHeapCBV_SRV_UAV = std::make_unique<DescriptorHeap_CBV_SRV_UAV>(m_device);
	m_descHeapRTV = std::make_unique<DescriptorHeap_RTV>(m_device);
	m_descHeapDSV = std::make_unique<DescriptorHeap_DSV>(m_device);

	//コマンドアロケータを生成
	m_commandAllocators.resize(s_frameBufferCount);
	for (UINT i = 0; i < s_frameBufferCount; ++i)
	{
		result = m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_commandAllocators[i])
		);
		if (FAILED(result))
		{
			printf("コマンドアロケータの生成失敗\n");
			assert(0);
		}
	}
	result = m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&m_oneshotCommandAllocator)
	);
	//バッファの転送を行うためにコマンドリストを使うので準備
	m_commandAllocators[0]->Reset();

	//コマンドリストを生成（GPUに、まとめて命令を送るためのコマンドリストを生成する）
	result = m_device->CreateCommandList(
		0, 
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocators[0].Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList));

	//コマンドキューの生成（コマンドリストをGPUに順に実行させていく為の仕組みを生成する）
	//標準設定でコマンドキューを生成
	D3D12_COMMAND_QUEUE_DESC queueDesc{
	  D3D12_COMMAND_LIST_TYPE_DIRECT,
	  0,
	  D3D12_COMMAND_QUEUE_FLAG_NONE,
	  0
	};
	result = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

	// スワップチェインの生成
	{
		DXGI_SWAP_CHAIN_DESC1 scDesc{};
		scDesc.BufferCount = s_frameBufferCount;
		scDesc.Width = ScreenSize.x;
		scDesc.Height = ScreenSize.y;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		scDesc.SampleDesc.Count = 1;
		//scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;  // ディスプレイの解像度も変更する場合にはコメント解除。

		//HDR対応化
		bool useHDR = UseHDR;
		if (useHDR)
		{
			bool isDisplayHDR10 = false;
			UINT index = 0;
			ComPtr<IDXGIOutput>current;
			while (tmpAdapter->EnumOutputs(index, &current) != DXGI_ERROR_NOT_FOUND)
			{
				ComPtr<IDXGIOutput6>output6;
				current.As(&output6);

				DXGI_OUTPUT_DESC1 desc;
				output6->GetDesc1(&desc);
				isDisplayHDR10 |= desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
				++index;
			}
			if (!isDisplayHDR10)useHDR = false;
		}
		if (useHDR)scDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		else scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		m_backBuffFormat = scDesc.Format;	//バックバッファのフォーマットを保存しておく

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc{};
		fsDesc.Windowed = IsFullScreen ? FALSE : TRUE;
		fsDesc.RefreshRate.Denominator = 1000;
		fsDesc.RefreshRate.Numerator = 60317;
		fsDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		fsDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;

		ComPtr<IDXGISwapChain1> swapchain1;
		result = m_dxgiFactory->CreateSwapChainForHwnd(
			m_commandQueue.Get(),
			Hwnd,
			&scDesc,
			&fsDesc,
			nullptr,
			&swapchain1);

		if (FAILED(result))
		{
			printf("スワップチェイン生成失敗\n");
			assert(0);
		}

		m_swapchain = std::make_unique<Swapchain>(m_device, swapchain1, *m_descHeapCBV_SRV_UAV, *m_descHeapRTV, useHDR, ClearValue);
	}

	//画像ロードのラムダ式生成
	s_loadLambdaTable["sph"]
		= s_loadLambdaTable["spa"]
		= s_loadLambdaTable["bmp"]
		= s_loadLambdaTable["png"]
		= s_loadLambdaTable["jpg"]
		= [](const std::wstring& Path, TexMetadata* Meta, ScratchImage& Img)->HRESULT
	{
		return LoadFromWICFile(Path.c_str(), WIC_FLAGS_NONE, Meta, Img);
	};
	s_loadLambdaTable["tga"]
		= [](const std::wstring& Path, TexMetadata* Meta, ScratchImage& Img)->HRESULT
	{
		return LoadFromTGAFile(Path.c_str(), Meta, Img);
	};
	s_loadLambdaTable["dds"]
		= [](const std::wstring& Path, TexMetadata* Meta, ScratchImage& Img)->HRESULT
	{
		return LoadFromDDSFile(Path.c_str(), DDS_FLAGS_NONE, Meta, Img);
	};

	//画像を分割するパイプライン
	{
		//シェーダ
		auto cs = CompileShader("resource/engine/RectTexture.hlsl", "CSmain", "cs_6_4");
		//ルートパラメータ
		std::vector<RootParam>rootParams =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"分割後の画像"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"分割前の画像"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"分割番号")
		};
		//パイプライン生成
		m_splitImgPipeline = GenerateComputePipeline(cs, rootParams, { WrappedSampler(true, false) });
	}
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> D3D12App::GenerateRootSignature(const std::vector<RootParam>& RootParams, std::vector<D3D12_STATIC_SAMPLER_DESC>& Samplers)
{
	ComPtr<ID3D12RootSignature>rootSignature;
	std::vector<CD3DX12_ROOT_PARAMETER>rootParameters;
	std::vector< CD3DX12_DESCRIPTOR_RANGE>rangeArray;

	//各レンジタイプでレジスターがいくつ登録されたか
	int registerNum[D3D12_DESCRIPTOR_RANGE_TYPE_NUM] = { 0 };
	for (auto& param : RootParams)
	{
		//ディスクリプタとして初期化
		if (param.m_descriptor)
		{
			//タイプの取得
			auto& type = param.m_descriptorRangeType;

			//ディスクリプタレンジ初期化
			CD3DX12_DESCRIPTOR_RANGE range{};
			range.Init(type, 1, registerNum[(int)type]);

			registerNum[(int)type]++;
			rangeArray.emplace_back(range);
		}
		//ビューで初期化
		else
		{
			rootParameters.emplace_back();
			if (param.m_viewType == SRV)
			{
				auto type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				rootParameters.back().InitAsShaderResourceView(registerNum[(int)type]);
				registerNum[(int)type]++;
			}
			else if (param.m_viewType == CBV)
			{
				auto type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
				rootParameters.back().InitAsConstantBufferView(registerNum[(int)type]);
				registerNum[(int)type]++;
			}
			else if (param.m_viewType == UAV)
			{
				auto type = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				rootParameters.back().InitAsUnorderedAccessView(registerNum[(int)type]);
				registerNum[(int)type]++;
			}
			else
			{
				assert(0);	//コンストラクタで避けられてるはずだけど一応
			}
		}
	}

	for (int i = 0; i < rangeArray.size(); ++i)
	{
		rootParameters.emplace_back();
		rootParameters.back().InitAsDescriptorTable(1, &rangeArray[i]);
	}

	for (int i = 0; i < Samplers.size(); ++i)
	{
		Samplers[i].ShaderRegister = i;
	}
	// ルートシグネチャの設定
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_0(static_cast<UINT>(rootParameters.size()), rootParameters.data(),
		static_cast<UINT>(Samplers.size()), Samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob;
	ComPtr<ID3DBlob> errorBlob = nullptr;	//エラーオブジェクト
	// バージョン自動判定のシリアライズ
	auto hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

	if (FAILED(hr))
	{
		//errorBlobからエラー内容string型にコピー
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin());
		errstr += '\n';
		assert(0);
	}

	// ルートシグネチャの生成
	hr = m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	//ルートシグネチャ生成に失敗
	assert(SUCCEEDED(hr));

	return rootSignature;
}

std::shared_ptr<VertexBuffer> D3D12App::GenerateVertexBuffer(const size_t& VertexSize, const int& VertexNum, void* InitSendData, const char* Name, const bool& RWStructuredBuff)
{
	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_GENERIC_READ;

	//頂点バッファサイズ
	UINT sizeVB = static_cast<UINT>(VertexSize * VertexNum);

	D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(sizeVB);
	if (RWStructuredBuff)
	{
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		//ヒーププロパティ
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		prop.CreationNodeMask = 1;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		prop.Type = D3D12_HEAP_TYPE_CUSTOM;
		prop.VisibleNodeMask = 1;
	}

	//頂点バッファ生成
	ComPtr<ID3D12Resource1>buff;
	auto hr = m_device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));

	assert(SUCCEEDED(hr));

	//名前のセット
	if (Name != nullptr)buff->SetName(KuroFunc::GetWideStrFromStr(Name).c_str());

	//頂点バッファビュー作成
	D3D12_VERTEX_BUFFER_VIEW vbView;
	vbView.BufferLocation = buff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeVB;
	vbView.StrideInBytes = static_cast<UINT>(VertexSize);

	//専用のクラスにまとめる
	std::shared_ptr<VertexBuffer>result;

	//読み取り専用構造化バッファを生成するか
	if (RWStructuredBuff)
	{
		//シェーダリソースビュー作成
		m_descHeapCBV_SRV_UAV->CreateUAV(m_device, buff, VertexSize, VertexNum);

		//ビューを作成した位置のディスクリプタハンドルを取得
		DescHandles handles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

		result = std::make_shared<VertexBuffer>(buff, barrier, vbView, handles);
	}
	else
	{
		result = std::make_shared<VertexBuffer>(buff, barrier, vbView);
	}

	//初期化マッピング
	if (InitSendData != nullptr)result->Mapping(InitSendData);

	return result;
}

std::shared_ptr<IndexBuffer> D3D12App::GenerateIndexBuffer(const int& IndexNum, void* InitSendData, const char* Name, const DXGI_FORMAT& IndexFormat)
{
	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_GENERIC_READ;

	size_t indexSize = 0;
	if (IndexFormat == DXGI_FORMAT_R32_UINT)indexSize = sizeof(unsigned int);
	else if (IndexFormat == DXGI_FORMAT_R16_UINT)indexSize = sizeof(unsigned short);
	else assert(0);

	//インデックスバッファサイズ
	UINT sizeIB = static_cast<UINT>(indexSize * IndexNum);

	//頂点バッファ生成
	ComPtr<ID3D12Resource1>buff;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto idxDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeIB);
	auto hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&idxDesc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));

	assert(SUCCEEDED(hr));

	//名前のセット
	if (Name != nullptr)buff->SetName(KuroFunc::GetWideStrFromStr(Name).c_str());

	//インデックスバッファビュー作成
	D3D12_INDEX_BUFFER_VIEW ibView;
	ibView.BufferLocation = buff->GetGPUVirtualAddress();
	ibView.Format = IndexFormat;
	ibView.SizeInBytes = sizeIB;

	//専用のクラスにまとめる
	auto result = std::make_shared<IndexBuffer>(buff, barrier, ibView, indexSize);

	//初期化マッピング
	if (InitSendData != nullptr)result->Mapping(InitSendData);

	return result;
}

std::shared_ptr<ConstantBuffer> D3D12App::GenerateConstantBuffer(const size_t& DataSize, const int& ElementNum, void* InitSendData, const char* Name)
{
	//アライメントしたサイズ
	size_t alignmentSize = (static_cast<UINT>(DataSize * ElementNum) + 0xff) & ~0xff;

	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_GENERIC_READ;

	//定数バッファ生成
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(alignmentSize);
	ComPtr<ID3D12Resource1>buff;
	auto hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));

	assert(SUCCEEDED(hr));


	//名前のセット
	if (Name != nullptr)buff->SetName(KuroFunc::GetWideStrFromStr(Name).c_str());

	//定数バッファビュー作成
	m_descHeapCBV_SRV_UAV->CreateCBV(m_device, buff->GetGPUVirtualAddress(), alignmentSize);

	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles handles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//専用の定数バッファクラスにまとめる
	std::shared_ptr<ConstantBuffer>result;
	result = std::make_shared<ConstantBuffer>(buff, barrier, handles, DataSize, ElementNum);

	//初期値マッピング
	if (InitSendData != nullptr)result->Mapping(InitSendData);

	return result;
}

std::shared_ptr<StructuredBuffer> D3D12App::GenerateStructuredBuffer(const size_t& DataSize, const int& ElementNum, void* InitSendData, const char* Name)
{
	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_GENERIC_READ;

	//定数バッファ生成
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(DataSize * ElementNum);
	ComPtr<ID3D12Resource1>buff;
	auto hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));

	assert(SUCCEEDED(hr));

	//名前のセット
	if (Name != nullptr)buff->SetName(KuroFunc::GetWideStrFromStr(Name).c_str());

	//シェーダリソースビュー作成
	m_descHeapCBV_SRV_UAV->CreateSRV(m_device, buff, DataSize, ElementNum);

	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles handles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//専用の構造化バッファクラスにまとめる
	std::shared_ptr<StructuredBuffer>result;
	result = std::make_shared<StructuredBuffer>(buff, barrier, handles, DataSize, ElementNum);

	//初期値マッピング
	if (InitSendData != nullptr)result->Mapping(InitSendData);

	return result;
}

std::shared_ptr<RWStructuredBuffer> D3D12App::GenerateRWStructuredBuffer(const size_t& DataSize, const int& ElementNum, void* InitSendData, const char* Name)
{
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(DataSize * ElementNum);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	//ヒーププロパティ
	D3D12_HEAP_PROPERTIES prop{};
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask = 1;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask = 1;

	//定数バッファ生成
	ComPtr<ID3D12Resource1>buff;
	auto hr = m_device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));

	assert(SUCCEEDED(hr));

	//名前のセット
	if (Name != nullptr)buff->SetName(KuroFunc::GetWideStrFromStr(Name).c_str());

	//シェーダリソースビュー作成
	m_descHeapCBV_SRV_UAV->CreateUAV(m_device, buff, DataSize, ElementNum);

	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles handles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//専用の構造化バッファクラスにまとめる
	std::shared_ptr<RWStructuredBuffer>result;
	result = std::make_shared<RWStructuredBuffer>(buff, barrier, handles, DataSize, ElementNum);
	if(InitSendData)result->Mapping(InitSendData);

	return result;
}

std::shared_ptr<TextureBuffer> D3D12App::GenerateTextureBuffer(const Color& Color, const int& Width, const DXGI_FORMAT& Format)
{
	//既にあるか確認
	for (auto itr = m_colorTextures.begin(); itr != m_colorTextures.end(); ++itr)
	{
		if (itr->m_color == Color && itr->m_width == Width)
		{
			return itr->m_tex;
		}
	}

	//なかったので生成する
	const int texDataCount = Width * Width;

	//テクスチャデータ配列
	XMFLOAT4* texturedata = new XMFLOAT4[texDataCount];

	//全ピクセルの色を初期化
	for (int i = 0; i < texDataCount; ++i)
	{
		texturedata[i].x = Color.m_r;	//R
		texturedata[i].y = Color.m_g;	//G
		texturedata[i].z = Color.m_b;	//B
		texturedata[i].w =Color.m_a;	//A
	}

	//テクスチャヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	//テクスチャリソース設定
	D3D12_RESOURCE_DESC texDesc{};	//リソース設定
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	//2Dテクスチャ用
	texDesc.Format = Format;	//RGBAフォーマット
	texDesc.Width = Width;
	texDesc.Height = Width;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;

	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_GENERIC_READ;

	//テクスチャ用リソースバッファの生成
	ComPtr<ID3D12Resource1>buff;
	auto hr = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));

	assert(SUCCEEDED(hr));

	//バッファに名前セット
	std::wstring name = L"ColorTexture - ";
	name += std::to_wstring(Color.m_r) + L" , ";
	name += std::to_wstring(Color.m_g) + L" , ";
	name += std::to_wstring(Color.m_b) + L" , ";
	name += std::to_wstring(Color.m_a);
	buff->SetName(name.c_str());

	//テクスチャバッファにデータ転送
	hr = buff->WriteToSubresource(
		0,
		nullptr,	//全領域へコピー
		texturedata,	//元データアドレス
		sizeof(XMFLOAT4) * Width,	//1ラインサイズ
		sizeof(XMFLOAT4) * texDataCount	//１枚サイズ
	);
	delete[] texturedata;

	assert(SUCCEEDED(hr));

	//シェーダーリソースビュー作成
	m_descHeapCBV_SRV_UAV->CreateSRV(m_device, buff, Format);

	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles handles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//専用のシェーダーリソースクラスにまとめる
	std::shared_ptr<TextureBuffer>result;
	result = std::make_shared<TextureBuffer>(buff, barrier, handles, texDesc);
	
	//テクスチャ用のリソースバリアに変更
	result->ChangeBarrier(m_commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	//作成したカラーテクスチャ情報を記録
	ColorTexture colorTexData;
	colorTexData.m_color = Color;
	colorTexData.m_width = Width;
	colorTexData.m_tex = result;
	m_colorTextures.emplace_back(colorTexData);

	return result;
}

std::shared_ptr<TextureBuffer> D3D12App::GenerateTextureBuffer(const std::string& LoadImgFilePath, const bool& SRVAsCube)
{
	//既にあるか確認
	for (auto itr = m_loadImgTextures.begin(); itr != m_loadImgTextures.end(); ++itr)
	{
		if (itr->m_path == LoadImgFilePath)
		{
			return itr->m_textures[0];
		}
	}

	TexMetadata metadata{};
	ScratchImage image{};

	//ワイド文字変換
	auto wtexpath = KuroFunc::GetWideStrFromStr(LoadImgFilePath);

	//拡張子取得
	auto ext = KuroFunc::GetExtension(LoadImgFilePath);

	//ロード
	auto hr = s_loadLambdaTable[ext](
		wtexpath,
		&metadata,
		image);
	assert(SUCCEEDED(hr));


	ComPtr<ID3D12Resource>buff;
	CreateTexture(m_device.Get(), metadata, &buff);

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	PrepareUpload(m_device.Get(), image.GetImages(), image.GetImageCount(), metadata, subresources);
	const auto totalBytes = GetRequiredIntermediateSize(buff.Get(), 0, UINT(subresources.size()));

	auto texDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

	ComPtr<ID3D12GraphicsCommandList> command;
	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_oneshotCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&command));
	assert(SUCCEEDED(hr));
	command->SetName(L"OneShotCommand");

	ComPtr<ID3D12Resource1>staging;
	CD3DX12_HEAP_PROPERTIES heapPropForTex(D3D12_HEAP_TYPE_UPLOAD);
	hr = m_device->CreateCommittedResource(
		&heapPropForTex,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&staging));
	assert(SUCCEEDED(hr));
	UpdateSubresources(command.Get(), buff.Get(), staging.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	//名前セット
	buff->SetName(wtexpath.c_str());

	//シェーダーリソースビュー作成
	if (SRVAsCube)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = metadata.format;
		srvDesc.TextureCube.MipLevels = UINT(metadata.mipLevels);
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		m_descHeapCBV_SRV_UAV->CreateSRV(m_device, buff, srvDesc);
	}
	else
	{
		m_descHeapCBV_SRV_UAV->CreateSRV(m_device, buff, metadata.format);
		texDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height);
	}

	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles handles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//専用のシェーダーリソースクラスにまとめる
	std::shared_ptr<TextureBuffer>result;
	result = std::make_shared<TextureBuffer>(buff, D3D12_RESOURCE_STATE_COPY_DEST, handles, texDesc);

	//テクスチャ用のリソースバリアに変更
	result->ChangeBarrier(command, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	ID3D12CommandList* commandList[] = {
   command.Get()
	};
	command->Close();
	m_commandQueue->ExecuteCommandLists(1, commandList);
	
	ComPtr<ID3D12Fence1> fence;
	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));
	const UINT64 expectValue = 1;
	m_commandQueue->Signal(fence.Get(), expectValue);
	do
	{
	} while (fence->GetCompletedValue() != expectValue);
	m_oneshotCommandAllocator->Reset();

	return result;
}

std::shared_ptr<TextureBuffer> D3D12App::GenerateTextureBuffer(const Vec2<int>& Size, const DXGI_FORMAT& Format, const char* Name)
{
	//テクスチャリソース設定
	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		Format,	//RGBAフォーマット
		(UINT)Size.x,
		(UINT)Size.y,
		(UINT16)1,
		(UINT16)1);
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	//テクスチャ用リソースバッファの生成
	ComPtr<ID3D12Resource1>buff;
	CD3DX12_HEAP_PROPERTIES heapPropForTex(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto hr = m_device->CreateCommittedResource(
		&heapPropForTex,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));
	assert(SUCCEEDED(hr));

	//名前セット
	if (Name != nullptr)
	{
		buff->SetName(KuroFunc::GetWideStrFromStr(Name).c_str());
	}

	//シェーダーリソースビュー作成
	m_descHeapCBV_SRV_UAV->CreateSRV(m_device, buff, texDesc.Format);
	DescHandles srvHandles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//アンオーダードアクセスビュー作成
	m_descHeapCBV_SRV_UAV->CreateUAV(m_device, buff, 4, static_cast<int>(texDesc.Width * texDesc.Height), D3D12_UAV_DIMENSION_TEXTURE2D, texDesc.Format);
	DescHandles uavHandles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//専用のシェーダーリソースクラスにまとめる
	std::shared_ptr<TextureBuffer>result;
	result = std::make_shared<TextureBuffer>(buff, barrier, srvHandles, texDesc);
	result->SetUAVHandles(uavHandles);

	return result;
}

std::shared_ptr<TextureBuffer> D3D12App::GenerateTextureBuffer(const std::vector<char>& ImgData, const int& Channel)
{
	assert(-1 <= Channel && Channel < 4);

	// VRM なので png/jpeg などのファイルを想定し、WIC で読み込む.
	ComPtr<ID3D12Resource1> staging;
	HRESULT hr;
	ScratchImage image;
	hr = LoadFromWICMemory(ImgData.data(), ImgData.size(), WIC_FLAGS_NONE, nullptr, image);
	assert(SUCCEEDED(hr));

	auto metadata = image.GetMetadata();
	const Image* img = image.GetImage(0, 0, 0);	//生データ抽出

	if (Channel != -1)
	{
		std::vector<uint8_t>pixelBuff;	//特定のチャンネルのみのピクセルデータ
		for (size_t h = 0; h < img->height; ++h)
		{
			for (size_t w = 0; w < img->width; w++)
			{
				int idx = h * static_cast<int>(img->width) * 4 + static_cast<int>(w) * 4 + Channel;
				pixelBuff.emplace_back(img->pixels[idx]);
			}
		}

		int idx = 0;
		for (size_t h = 0; h < img->height; ++h)
		{
			for (size_t w = 0; w < img->width; w++)
			{
				int startIdx = h * static_cast<int>(img->width) * 4 + (static_cast<int>(w) * 4);
				img->pixels[startIdx] = pixelBuff[idx++];	//R
				img->pixels[startIdx + 1] = 0;	//G
				img->pixels[startIdx + 2] = 0;	//B
				img->pixels[startIdx + 3] = 0;	//A
			}
		}
	}

	//テクスチャリソース設定
	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,	//RGBAフォーマット
		metadata.width,
		(UINT)metadata.height,
		(UINT16)metadata.arraySize,
		(UINT16)metadata.mipLevels);

	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_GENERIC_READ;

	//テクスチャ用リソースバッファの生成
	ComPtr<ID3D12Resource1>buff;
	CD3DX12_HEAP_PROPERTIES heapPropForTex(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	hr = m_device->CreateCommittedResource(
		&heapPropForTex,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));
	assert(SUCCEEDED(hr));

	//名前セット
	//buff->SetName(wtexpath.c_str());

	//テクスチャバッファにデータ転送
	hr = buff->WriteToSubresource(
		0,
		nullptr,	//全領域へコピー
		img->pixels,	//元データアドレス
		(UINT)img->rowPitch,	//1ラインサイズ
		(UINT)img->slicePitch	//１枚サイズ
	);
	assert(SUCCEEDED(hr));

	//シェーダーリソースビュー作成
	m_descHeapCBV_SRV_UAV->CreateSRV(m_device, buff, metadata.format);

	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles handles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//専用のシェーダーリソースクラスにまとめる
	std::shared_ptr<TextureBuffer>result;
	result = std::make_shared<TextureBuffer>(buff, barrier, handles, texDesc);

	//テクスチャ用のリソースバリアに変更
	result->ChangeBarrier(m_commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	//作成したカラーテクスチャ情報を記録
	LoadImgTexture loadImgTexData;
	//loadImgTexData.path = LoadImgFilePath;
	loadImgTexData.m_textures = { result };
	m_loadImgTextures.emplace_back(loadImgTexData);

	return result;
}

void D3D12App::GenerateTextureBuffer(std::shared_ptr<TextureBuffer>* Array, const std::shared_ptr<TextureBuffer>& SorceTexBuffer, const int& AllNum, const Vec2<int>& SplitNum, const std::string& Name)
{
	//ディスクリプタヒープをセット
	ID3D12DescriptorHeap* heaps[] = { m_descHeapCBV_SRV_UAV->GetHeap().Get() };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	m_splitImgPipeline->SetPipeline(m_commandList);
	SorceTexBuffer->SetComputeDescriptorBuffer(m_commandList, SRV, 1);

	SplitImgConstData constData;

	//分割前のサイズを記録
	constData.m_splitSize = { static_cast<int>(SorceTexBuffer->GetDesc().Width) / SplitNum.x,static_cast<int>(SorceTexBuffer->GetDesc().Height) / SplitNum.y };

	for (int i = 0; i < AllNum; ++i)
	{
		if (m_splitImgConstBuff.size() < m_splitTexBuffCount + 1)
		{
			std::string name = "SplitImgConstBuff - " + std::to_string(i);
			m_splitImgConstBuff.emplace_back(GenerateConstantBuffer(sizeof(SplitImgConstData), 1, nullptr, name.c_str()));
		}

		//描き込み先用のテクスチャバッファ
		auto splitResult = GenerateTextureBuffer(constData.m_splitSize, SorceTexBuffer->GetDesc().Format, (Name + " - " + std::to_string(i)).c_str());

		splitResult->SetComputeDescriptorBuffer(m_commandList, UAV, 0);

		//splitImgConstBuff[i]->Mapping(&constData);
		//splitImgConstBuff[i]->SetComputeDescriptorBuffer(commandList, CBV, 2);
		m_splitImgConstBuff[m_splitTexBuffCount]->Mapping(&constData);
		m_splitImgConstBuff[m_splitTexBuffCount]->SetComputeDescriptorBuffer(m_commandList, CBV, 2);

		static const int THREAD_NUM = 8;
		const Vec2<UINT>thread =
		{
			static_cast<UINT>(constData.m_splitSize.x / THREAD_NUM),
			static_cast<UINT>(constData.m_splitSize.y / THREAD_NUM)
		};
		m_commandList->Dispatch(thread.x, thread.y, 1);

		//テクスチャ用のリソースバリア変更
		splitResult->ChangeBarrier(m_commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		Array[i] = splitResult;

		constData.m_imgNum.x++;
		if (SplitNum.x <= constData.m_imgNum.x)
		{
			constData.m_imgNum.x = 0;
			constData.m_imgNum.y++;
		}

		m_splitTexBuffCount++;
	}
}

void D3D12App::GenerateTextureBuffer(std::shared_ptr<TextureBuffer>* Array, const std::string& LoadImgFilePath, const int& AllNum, const Vec2<int>& SplitNum)
{
	auto sourceTexture = GenerateTextureBuffer(LoadImgFilePath);
	return GenerateTextureBuffer(Array, sourceTexture, AllNum, SplitNum, LoadImgFilePath);
}

std::shared_ptr<IndirectCommandBuffer> D3D12App::GenerateIndirectCommandBuffer(const EXCUTE_INDIRECT_TYPE& IndirectType, const int& MaxCommandCount, const int& GpuAddressNum, const bool& CounterBuffer, const void* InitCommandData, const char* Name)
{
	std::shared_ptr<GPUResource>counterBuffer;
	//カウンターバッファ
	if(CounterBuffer)
	{
		ComPtr<ID3D12Resource>buff;

		D3D12_HEAP_PROPERTIES heapProp = {};
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapProp.CreationNodeMask = 1;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
		heapProp.VisibleNodeMask = 1;

		auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto barrier = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		auto hr = m_device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			barrier,
			nullptr,
			IID_PPV_ARGS(&buff));
		assert(SUCCEEDED(hr));

		std::string name;
		if (Name)name = std::string(Name);
		name += "- CounterBuffer";
		buff->SetName(KuroFunc::GetWideStrFromStr(name).c_str());

		counterBuffer = std::make_shared<GPUResource>(buff, barrier);
	}

	//コマンド１つあたりのサイズ
	size_t commandSize = sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * GpuAddressNum;
	if (IndirectType == DRAW)commandSize += sizeof(D3D12_DRAW_ARGUMENTS);
	else if (IndirectType == DRAW_INDEXED)commandSize += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
	else if (IndirectType == DISPATCH)commandSize += sizeof(D3D12_DISPATCH_ARGUMENTS);
	else assert(0);

	//ヒーププロパティ
	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	//リソース設定
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(commandSize * MaxCommandCount, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_COPY_DEST;

	//バッファ生成
	ComPtr<ID3D12Resource1>buff;
	auto hr = m_device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		barrier,
		nullptr,
		IID_PPV_ARGS(&buff));
	assert(SUCCEEDED(hr));
	if(Name)buff->SetName(KuroFunc::GetWideStrFromStr(Name).c_str());

	//UAV設定
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = MaxCommandCount;
	uavDesc.Buffer.StructureByteStride = commandSize;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	//SRV設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = MaxCommandCount;
	srvDesc.Buffer.StructureByteStride = commandSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	//ビュー生成
	auto uavDescHandles = D3D12App::Instance()->CreateUAV(buff, uavDesc, CounterBuffer ? counterBuffer->GetBuff() : nullptr);
	auto srvDescHandles = D3D12App::Instance()->CreateSRV(buff, srvDesc);

	std::shared_ptr<DescriptorData>data = std::make_shared<DescriptorData>(buff, barrier);
	data->InitDescHandle(UAV, uavDescHandles);
	data->InitDescHandle(SRV, srvDescHandles);

	return std::make_shared<IndirectCommandBuffer>(IndirectType, MaxCommandCount, commandSize, data, counterBuffer);
}

DescHandles D3D12App::CreateSRV(const ComPtr<ID3D12Resource>& Buff, const D3D12_SHADER_RESOURCE_VIEW_DESC& ViewDesc)
{
	m_descHeapCBV_SRV_UAV->CreateSRV(m_device, Buff, ViewDesc);
	return DescHandles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());
}

DescHandles D3D12App::CreateRTV(const ComPtr<ID3D12Resource>& Buff, const D3D12_RENDER_TARGET_VIEW_DESC* ViewDesc)
{
	m_descHeapRTV->CreateRTV(m_device, Buff, ViewDesc);
	return DescHandles(m_descHeapRTV->GetCpuHandleTail(), m_descHeapRTV->GetGpuHandleTail());
}

DescHandles D3D12App::CreateDSV(const ComPtr<ID3D12Resource>& Buff, const D3D12_DEPTH_STENCIL_VIEW_DESC* ViewDesc)
{
	m_descHeapDSV->CreateDSV(m_device, Buff, ViewDesc);
	return DescHandles(m_descHeapDSV->GetCpuHandleTail(), m_descHeapDSV->GetGpuHandleTail());
}

DescHandles D3D12App::CreateUAV(const ComPtr<ID3D12Resource>& Buff, const D3D12_UNORDERED_ACCESS_VIEW_DESC& ViewDesc, const ComPtr<ID3D12Resource>& CounterBuff)
{
	m_descHeapCBV_SRV_UAV->CreateUAV(m_device, Buff, ViewDesc, CounterBuff);
	return DescHandles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());
}

std::shared_ptr<RenderTarget> D3D12App::GenerateRenderTarget(const DXGI_FORMAT& Format, const Color& ClearValue, const Vec2<int> Size, const wchar_t* TargetName, D3D12_RESOURCE_STATES InitState, int MipLevel, int ArraySize)
{
	//レンダーターゲット設定
	CD3DX12_RESOURCE_DESC desc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		static_cast<UINT>(Size.x),
		static_cast<UINT>(Size.y),
		ArraySize,
		MipLevel,
		Format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	);

	//レンダーターゲットのクリア値

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = Format;
	clearValue.Color[0] = ClearValue.m_r;
	clearValue.Color[1] = ClearValue.m_g;
	clearValue.Color[2] = ClearValue.m_b;
	clearValue.Color[3] = ClearValue.m_a;

	//リソース生成
	ComPtr<ID3D12Resource1>buff;
	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto hr = m_device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		InitState,
		&clearValue,
		IID_PPV_ARGS(&buff));
	assert(SUCCEEDED(hr));

	//名前セット
	if (TargetName != nullptr)buff->SetName(TargetName);

	//SRV作成
	m_descHeapCBV_SRV_UAV->CreateSRV(m_device, buff, Format);
	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles srvHandles(m_descHeapCBV_SRV_UAV->GetCpuHandleTail(), m_descHeapCBV_SRV_UAV->GetGpuHandleTail());

	//RTV作成
	m_descHeapRTV->CreateRTV(m_device, buff);
	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles rtvHandles(m_descHeapRTV->GetCpuHandleTail(), m_descHeapRTV->GetGpuHandleTail());

	//専用のレンダーターゲットクラスにまとめる
	std::shared_ptr<RenderTarget>result;
	result = std::make_shared<RenderTarget>(buff, InitState, srvHandles, rtvHandles, desc, ClearValue);

	return result;
}

std::shared_ptr<DepthStencil> D3D12App::GenerateDepthStencil(const Vec2<int>& Size, const DXGI_FORMAT& Format, const float& ClearValue)
{
	//リソース設定
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
		Format,	//深度値フォーマット
		static_cast<UINT>(Size.x),
		static_cast<UINT>(Size.y),
		1, 0,
		1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);	//デプスステンシル

	//リソースバリア
	auto barrier = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	//デプスステンシルバッファ生成
	ComPtr<ID3D12Resource1>buff;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto clearVal = CD3DX12_CLEAR_VALUE(Format, ClearValue, 0);
	auto hr = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearVal,
		IID_PPV_ARGS(&buff));
	assert(SUCCEEDED(hr));

	//DSV作成
	m_descHeapDSV->CreateDSV(m_device, buff);
	//ビューを作成した位置のディスクリプタハンドルを取得
	DescHandles handles(m_descHeapDSV->GetCpuHandleTail(), m_descHeapDSV->GetGpuHandleTail());

	//専用のレンダーターゲットクラスにまとめる
	std::shared_ptr<DepthStencil>result;
	result = std::make_shared<DepthStencil>(buff, barrier, handles, desc, ClearValue);

	return result;
}

void D3D12App::SetDescHeaps()
{
	//ディスクリプタヒープをセット
	ID3D12DescriptorHeap* heaps[] = { m_descHeapCBV_SRV_UAV->GetHeap().Get() };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
}

void D3D12App::Render(D3D12AppUser* User)
{
	SetDescHeaps();

	//スワップチェイン表示可能からレンダーターゲット描画可能へ
	m_swapchain->GetBackBufferRenderTarget()->ChangeBarrier(m_commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	//レンダーターゲットをクリア
	m_swapchain->GetBackBufferRenderTarget()->Clear(m_commandList);

	//レンダリング処理
	User->Render();

	//レンダーターゲットからスワップチェイン表示可能へ
	m_swapchain->GetBackBufferRenderTarget()->ChangeBarrier(m_commandList, D3D12_RESOURCE_STATE_PRESENT);

	//命令のクローズ
	auto hr = m_commandList->Close();
	assert(SUCCEEDED(hr));

	//コマンドリストの実行
	ID3D12CommandList* cmdLists[] = { m_commandList.Get() };	//コマンドリストの配列
	m_commandQueue->ExecuteCommandLists(1, cmdLists);

	//バッファをフリップ（裏表の入れ替え）
	hr = m_swapchain->GetSwapchain()->Present(1, 0);
	assert(SUCCEEDED(hr));

	//バックバッファ番号取得
	auto frameIdx = m_swapchain->GetSwapchain()->GetCurrentBackBufferIndex();

	//コマンドアロケータリセット
	hr = m_commandAllocators[frameIdx]->Reset();	//キューをクリア
	assert(SUCCEEDED(hr));

	//コマンドリスト
	hr = m_commandList->Reset(m_commandAllocators[frameIdx].Get(), nullptr);		//コマンドリストを貯める準備
	assert(SUCCEEDED(hr));

	//コマンドリストの実行完了を待つ
	m_swapchain->WaitPreviousFrame(m_commandQueue, frameIdx);

	//SplitTexBuff呼ばれた回数リセット
	m_splitTexBuffCount = 0;
}

void D3D12App::UploadCPUResource(std::shared_ptr<GPUResource>& Dest, const size_t& DataSize, const int& ElementNum, const void* SendData)
{
	ComPtr<ID3D12GraphicsCommandList> command;
	auto hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_oneshotCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&command));
	assert(SUCCEEDED(hr));
	command->SetName(L"OneShotCommand");

	size_t buffSize = DataSize * ElementNum;

	ComPtr<ID3D12Resource> uploadBuffer;
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);
	m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer));

	D3D12_SUBRESOURCE_DATA data = {};
	data.pData = SendData;
	data.RowPitch = buffSize;
	data.SlicePitch = data.RowPitch;

	auto beforeBarrier = Dest->GetResourceBarrier();
	Dest->ChangeBarrier(command, D3D12_RESOURCE_STATE_COPY_DEST);
	UpdateSubresources<1>(command.Get(), Dest->GetBuff().Get(), uploadBuffer.Get(), 0, 0, 1, &data);
	Dest->ChangeBarrier(command, beforeBarrier);

	ID3D12CommandList* commandList[] = {
   command.Get()
	};
	command->Close();
	m_commandQueue->ExecuteCommandLists(1, commandList);

	ComPtr<ID3D12Fence1> fence;
	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));
	const UINT64 expectValue = 1;
	m_commandQueue->Signal(fence.Get(), expectValue);
	do
	{
	} while (fence->GetCompletedValue() != expectValue);
	m_oneshotCommandAllocator->Reset();
}

#include <filesystem>
#include<fstream>
Microsoft::WRL::ComPtr<IDxcBlob>D3D12App::CompileShader(const std::string& FilePath, const std::string& EntryPoint, const std::string& ShaderModel)
{
	HRESULT hr;
	std::ifstream infile(FilePath, std::ifstream::binary);
	if (!infile) {
		throw std::runtime_error("failed shader compile.");
	}

	auto w_filePath = KuroFunc::GetWideStrFromStr(FilePath);
	auto w_entryPoint = KuroFunc::GetWideStrFromStr(EntryPoint);
	auto w_shaderModel = KuroFunc::GetWideStrFromStr(ShaderModel);

	std::stringstream strstream;

	strstream << infile.rdbuf();

	std::string shaderCode = strstream.str();
	Microsoft::WRL::ComPtr<IDxcLibrary> library;
	hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	Microsoft::WRL::ComPtr<IDxcCompiler> compiler;
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> source;


	hr = library->CreateBlobWithEncodingFromPinned(
		(LPBYTE)shaderCode.c_str(), (UINT32)shaderCode.size(), CP_UTF8, &source);
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	// インクルードを使う場合には適切に設定すること.
	hr = library->CreateIncludeHandler(&includeHandler);
	// コンパイルオプションの指定.
	std::vector<LPCWSTR> arguments;
	arguments.emplace_back(L"/Od");
	arguments.emplace_back(L"/Zi");
	arguments.emplace_back(L"-Qembed_debug");

	Microsoft::WRL::ComPtr<IDxcOperationResult> dxcResult;
	hr = compiler->Compile(
		source.Get(),
		w_filePath.c_str(),
		w_entryPoint.data(),
		w_shaderModel.data(),
		arguments.data(),
		static_cast<UINT32>(arguments.size()),
		nullptr,
		0,
		includeHandler.Get(),
		&dxcResult
	);

	if (FAILED(hr)) {
		throw std::runtime_error("failed shader compile.");
	}

	dxcResult->GetStatus(&hr);
	if (FAILED(hr)) {
		ComPtr<IDxcBlobEncoding> errBlob;
		dxcResult->GetErrorBuffer(&errBlob);
		//errorBlobからエラー内容string型にコピー
		std::string errstr;
		errstr.resize(errBlob->GetBufferSize());

		std::copy_n((char*)errBlob->GetBufferPointer(),
			errBlob->GetBufferSize(),
			errstr.begin());
		errstr += '\n';
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
		assert(0);
	}
	Microsoft::WRL::ComPtr<IDxcBlob> blob;
	dxcResult->GetResult(&blob);
	return blob;

	/*
	//コンパイルの準備-------------------------
	Microsoft::WRL::ComPtr<IDxcLibrary> library;
	HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));

	Microsoft::WRL::ComPtr<IDxcCompiler> compiler;
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

	auto w_filePath = KuroFunc::GetWideStrFromStr(FilePath);
	auto w_entryPoint = KuroFunc::GetWideStrFromStr(EntryPoint);
	auto w_shaderModel = KuroFunc::GetWideStrFromStr(ShaderModel);

	uint32_t codePage = CP_UTF8;
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
	hr = library->CreateBlobFromFile(w_filePath.c_str(), &codePage, &sourceBlob);
	//コンパイルの準備-------------------------

	Microsoft::WRL::ComPtr<IDxcOperationResult> result;
	hr = compiler->Compile(
		sourceBlob.Get(),		// pSource
		w_filePath.c_str(),		// pSourceName
		w_entryPoint.c_str(),		// pEntryPoint
		w_shaderModel.c_str(),	// pTargetProfile
		nullptr, 0,				// pArguments, argCount
		nullptr, 0,				// pDefines, defineCount
		nullptr,				// pIncludeHandler
		&result);				// ppResult

	if (SUCCEEDED(hr))
	{
		result->GetStatus(&hr);
	}

	if (FAILED(hr))
	{
		ComPtr<IDxcBlobEncoding> errBlob;
		result->GetErrorBuffer(&errBlob);
		//errorBlobからエラー内容string型にコピー
		std::string errstr;
		errstr.resize(errBlob->GetBufferSize());

		std::copy_n((char*)errBlob->GetBufferPointer(),
			errBlob->GetBufferSize(),
			errstr.begin());
		errstr += '\n';
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
		assert(0);
	}

	ComPtr<IDxcBlob> code;
	result->GetResult(&code);
	return code;
	*/


	//D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	//psoDesc.PS = CD3DX12_SHADER_BYTECODE(code->GetBufferPointer(), code->GetBufferSize());
	//CComPtr<ID3D12PipelineState> pso;
	//hr = DirectX12Device::Instance()->dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

	/*
	HRESULT hr;

	std::filesystem::path hlslFilename(FilePath);

	std::ifstream infile(hlslFilename, std::ifstream::binary);
	if (!infile) {
		throw std::runtime_error("failed shader compile.");
	}
	std::wstring fileName = hlslFilename.filename().wstring();
	std::stringstream strstream;
	strstream << infile.rdbuf();
	std::string shaderCode = strstream.str();
	ComPtr<IDxcLibrary> library;
	DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	ComPtr<IDxcCompiler> compiler;
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	ComPtr<IDxcBlobEncoding> source;
	library->CreateBlobWithEncodingFromPinned(
		(LPBYTE)shaderCode.c_str(), (UINT32)shaderCode.size(), CP_UTF8, &source);
	ComPtr<IDxcIncludeHandler> includeHandler;
	// インクルードを使う場合には適切に設定すること.
	library->CreateIncludeHandler(&includeHandler);
	// コンパイルオプションの指定.
	std::vector<LPCWSTR> arguments;
	arguments.emplace_back(L"/Od");
	arguments.emplace_back(L"/Zi");
	arguments.emplace_back(L"-Qembed_debug");
	const auto shaderModel = KuroFunc::GetWideStrFromStr(ShaderModel.c_str());
	const auto entryPpoint = KuroFunc::GetWideStrFromStr(EntryPoint.c_str());
	ComPtr<IDxcOperationResult> dxcResult;
	hr = compiler->Compile(source.Get(), fileName.c_str(),
		entryPpoint.c_str(), shaderModel.c_str(), arguments.data(), UINT(arguments.size()),
		nullptr, 0, nullptr, &dxcResult);
	if (FAILED(hr)) {
		throw std::runtime_error("failed shader compile.");
	}
	dxcResult->GetStatus(&hr);
	if (FAILED(hr)) {
		ComPtr<IDxcBlobEncoding> errBlob;
		dxcResult->GetErrorBuffer(&errBlob);
		//errorBlobからエラー内容string型にコピー
		std::string errstr;
		errstr.resize(errBlob->GetBufferSize());

		std::copy_n((char*)errBlob->GetBufferPointer(),
			errBlob->GetBufferSize(),
			errstr.begin());
		errstr += '\n';
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
	}
	ComPtr<IDxcBlob>result;
	dxcResult->GetResult(&result);
	return result;
	*/

	/*
		 ... errBlob の内容をエラーメッ
	ComPtr<ID3DBlob>result;
	ComPtr<ID3DBlob> errorBlob = nullptr;	//エラーオブジェクト

	//ワイド文字に変換
	const std::wstring wideFilePath = KuroFunc::GetWideStrFromStr(FilePath);

	//頂点シェーダの読み込みとコンパイル
	auto hr = D3DCompileFromFile(
		wideFilePath.c_str(),		//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルード可能にする
		EntryPoint.c_str(), ShaderModel.c_str(),	//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//デバッグ用設定
		0,
		result.GetAddressOf(), &errorBlob);


	//シェーダのエラー内容を表示
	if (FAILED(hr))
	{
		//errorBlobからエラー内容string型にコピー
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			errstr.begin());
		errstr += '\n';
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(errstr.c_str());
		exit(1);
	}
	return result;
	*/
}

std::shared_ptr<GraphicsPipeline>D3D12App::GenerateGraphicsPipeline(
	const PipelineInitializeOption& Option,
	const Shaders& ShaderInfo,
	const std::vector<InputLayoutParam>& InputLayout,
	const std::vector<RootParam>& RootParams,
	const std::vector<RenderTargetInfo>& RenderTargetFormat,
	std::vector<D3D12_STATIC_SAMPLER_DESC> Samplers)
{
	HRESULT hr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

	std::vector<D3D12_INPUT_ELEMENT_DESC>inputLayout;
	//インプットレイアウト
	{
		for (auto& param : InputLayout)
		{
			D3D12_INPUT_ELEMENT_DESC input =
			{
				param.m_semantics.c_str(),	//セマンティック名
				0,				//同じセマンティック名が複数あるときに使うインデックス（０でよい）
				param.m_format,	//要素数とビット数を表す（XYZの３つでfloat型なので R32G32B32_FLOAT)
				0,	//入力スロットインデックス（０でよい）
				D3D12_APPEND_ALIGNED_ELEMENT,	//データのオフセット値（D3D12_APPEND_ALIGNED_ELEMENTだと自動設定）
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,		//入力データ種別（標準はD3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA）
				0		//一度に描画するインスタンス数（０でよい）
			};
			inputLayout.emplace_back(input);
		}
		desc.InputLayout.pInputElementDescs = &inputLayout[0];
		desc.InputLayout.NumElements = static_cast<UINT>(inputLayout.size());
	}

	//ルートパラメータ
	ComPtr<ID3D12RootSignature>rootSignature = GenerateRootSignature(RootParams, Samplers);

	//グラフィックスパイプライン設定にルートシグネチャをセット
	desc.pRootSignature = rootSignature.Get();

	//グラフィックスパイプライン設定
	{
		//サンプルマスク
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定

		// ラスタライザステート
		desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		//カリングモード
		if (!Option.m_calling)
		{
			desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		}
		//ワイヤーフレーム
		if (Option.m_wireFrame)
		{
			desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		}
		//深度テスト
		if (Option.m_depthTest)
		{
			desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			//デプスの書き込みを禁止（深度テストは行う）
			if (!Option.m_depthWriteMask)
			{
				desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			}
		}
		//三角形の表を判断する際の向き
		desc.RasterizerState.FrontCounterClockwise = Option.m_frontCounterClockWise;
		//デプスステンシルバッファフォーマット
		desc.DSVFormat = Option.m_dsvFormat;

		desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		//同時レンダーターゲットで独立したブレンディングを有効にするか
		desc.BlendState.IndependentBlendEnable = Option.m_independetBlendEnable;

		// 1ピクセルにつき1回サンプリング
		desc.SampleDesc.Count = 1;

		// 図形の形状設定
		desc.PrimitiveTopologyType = Option.m_primitiveTopologyType;

		//書き込み先レンダーターゲット
		desc.NumRenderTargets = 0;
		for (auto& info : RenderTargetFormat)
		{
			int idx = ++desc.NumRenderTargets - 1;

			//描画先レンダーターゲットのフォーマット
			desc.RTVFormats[idx] = info.m_format;

			//アルファブレンディング設定
			if (info.m_blendMode == AlphaBlendMode_Trans)	//半透明合成
			{
				desc.BlendState.RenderTarget[idx].BlendEnable = true;
				desc.BlendState.RenderTarget[idx].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				desc.BlendState.RenderTarget[idx].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
				desc.BlendState.RenderTarget[idx].BlendOp = D3D12_BLEND_OP_ADD;
			}
			else if (info.m_blendMode == AlphaBlendMode_Add)	//加算合成
			{
				//加算合成のブレンドステート作成
				desc.BlendState.RenderTarget[idx].BlendEnable = true;
				desc.BlendState.RenderTarget[idx].SrcBlend = D3D12_BLEND_ONE;
				desc.BlendState.RenderTarget[idx].DestBlend = D3D12_BLEND_ONE;
				desc.BlendState.RenderTarget[idx].BlendOp = D3D12_BLEND_OP_ADD;
			}
			else desc.BlendState.RenderTarget[idx].BlendEnable = false;	//完全上書き
		}
	}

	//シェーダーオブジェクトセット
	if (ShaderInfo.m_vs.Get())desc.VS = CD3DX12_SHADER_BYTECODE(ShaderInfo.m_vs->GetBufferPointer(), ShaderInfo.m_vs->GetBufferSize());
	if (ShaderInfo.m_ps.Get())desc.PS = CD3DX12_SHADER_BYTECODE(ShaderInfo.m_ps->GetBufferPointer(), ShaderInfo.m_ps->GetBufferSize());
	if(ShaderInfo.m_ds.Get())desc.DS = CD3DX12_SHADER_BYTECODE(ShaderInfo.m_ds->GetBufferPointer(), ShaderInfo.m_ds->GetBufferSize());
	if(ShaderInfo.m_hs.Get())desc.HS = CD3DX12_SHADER_BYTECODE(ShaderInfo.m_hs->GetBufferPointer(), ShaderInfo.m_hs->GetBufferSize());
	if(ShaderInfo.m_gs.Get())desc.GS = CD3DX12_SHADER_BYTECODE(ShaderInfo.m_gs->GetBufferPointer(), ShaderInfo.m_gs->GetBufferSize());

	//グラフィックスパイプラインの生成
	ComPtr<ID3D12PipelineState>pipeline;
	hr = m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline));
	assert(SUCCEEDED(hr));

	return std::make_shared<GraphicsPipeline>(pipeline, rootSignature, Option.m_primitiveTopology);
}

std::shared_ptr<ComputePipeline> D3D12App::GenerateComputePipeline(const ComPtr<IDxcBlob>& ComputeShader, const std::vector<RootParam>& RootParams, std::vector<D3D12_STATIC_SAMPLER_DESC> Samplers)
{
	HRESULT hr;

	// パイプラインステートを作成
	D3D12_COMPUTE_PIPELINE_STATE_DESC  desc = { 0 };

	//ルートパラメータ
	ComPtr<ID3D12RootSignature>rootSignature = GenerateRootSignature(RootParams, Samplers);

	//グラフィックスパイプライン設定にルートシグネチャをセット
	desc.pRootSignature = rootSignature.Get();

	desc.CS = CD3DX12_SHADER_BYTECODE(ComputeShader->GetBufferPointer(), ComputeShader->GetBufferSize());
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	desc.NodeMask = 0;

	ComPtr<ID3D12PipelineState>pipeline;
	hr = m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline));
	assert(SUCCEEDED(hr));

	return std::make_shared<ComputePipeline>(pipeline, rootSignature);
}

std::shared_ptr<IndirectDevice> D3D12App::GenerateIndirectDevice(const EXCUTE_INDIRECT_TYPE& ExcuteIndirectType, const std::vector<RootParam>& RootParams, std::vector<D3D12_STATIC_SAMPLER_DESC> Samplers)
{
	//Indirectの形式？
	static std::array<D3D12_INDIRECT_ARGUMENT_TYPE, EXCUTE_INDIRECT_TYPE_NUM>EXCUTE_ARG_TYPE =
	{
		D3D12_INDIRECT_ARGUMENT_TYPE_DRAW,
		D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED,
		D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH
	};
	//不適切な値
	assert(0 <= ExcuteIndirectType && ExcuteIndirectType < EXCUTE_INDIRECT_TYPE_NUM);

	//セットするGPUバッファの数
	int gpuBuffNum = RootParams.size();

	//引数バッファをGPUにどう解釈させるか
	std::vector<D3D12_INDIRECT_ARGUMENT_DESC>argDescArray;
	for (int paramIdx = 0; paramIdx < gpuBuffNum; ++paramIdx)
	{
		//設定を末尾に追加、参照を取得
		D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
		//RootParamから情報読み取り
		auto& param = RootParams[paramIdx];

		//各バッファービュー
		if (param.m_viewType == CBV)
		{
			argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			argDesc.ConstantBufferView.RootParameterIndex = paramIdx;
		}
		else if (param.m_viewType == SRV)
		{
			argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
			argDesc.ShaderResourceView.RootParameterIndex = paramIdx;
		}
		else if (param.m_viewType == UAV)
		{
			argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
			argDesc.UnorderedAccessView.RootParameterIndex = paramIdx;
		}
		else assert(0);//用意していない

		argDescArray.emplace_back(argDesc);
	}

	//末尾にIndirect形式の要素追加
	auto& excuteArgDesc = argDescArray.emplace_back();
	excuteArgDesc.Type = EXCUTE_ARG_TYPE[ExcuteIndirectType];

	//ルートパラメータ生成
	auto rootSignature = GenerateRootSignature(RootParams, Samplers);

	//コマンドシグネチャ情報
	D3D12_COMMAND_SIGNATURE_DESC cmdSignatureDesc = {};
	cmdSignatureDesc.pArgumentDescs = argDescArray.data();
	cmdSignatureDesc.NumArgumentDescs = argDescArray.size();
	cmdSignatureDesc.ByteStride = sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * gpuBuffNum + sizeof(D3D12_DRAW_ARGUMENTS);

	//コマンドシグネチャ生成
	ComPtr<ID3D12CommandSignature>cmdSignature;
	auto hr = m_device->CreateCommandSignature(&cmdSignatureDesc, rootSignature.Get(), IID_PPV_ARGS(&cmdSignature));
	assert(SUCCEEDED(hr));

	return std::make_shared<IndirectDevice>(ExcuteIndirectType, cmdSignature);
}

void D3D12App::SetBackBufferRenderTarget()
{
	m_swapchain->GetBackBufferRenderTarget()->ChangeBarrier(m_commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> BACK_BUFF_HANDLE = { m_swapchain->GetBackBufferRenderTarget()->AsRTV(m_commandList) };
	m_commandList->OMSetRenderTargets(static_cast<UINT>(BACK_BUFF_HANDLE.size()), &BACK_BUFF_HANDLE[0], FALSE, nullptr);
}

void D3D12App::DispathOneShot(std::weak_ptr<ComputePipeline>Pipeline, Vec3<int> ThreadNum, 
	std::vector<std::shared_ptr<DescriptorData>>& Datas, std::vector<DESC_HANDLE_TYPE>& DescTypes)
{
	assert(Datas.size() == DescTypes.size());

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>cmdList;
	auto hr = m_device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_oneshotCommandAllocator.Get(),
		nullptr, IID_PPV_ARGS(&cmdList));
	assert(SUCCEEDED(hr));

	//ディスクリプタヒープをセット
	ID3D12DescriptorHeap* heaps[] = { m_descHeapCBV_SRV_UAV->GetHeap().Get() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	Pipeline.lock()->SetPipeline(cmdList);
	for (int descIdx = 0; descIdx < Datas.size(); ++descIdx)
	{
		Datas[descIdx]->SetComputeDescriptorBuffer(cmdList, DescTypes[descIdx], descIdx);
	}
	cmdList->Dispatch(static_cast<UINT>(ThreadNum.x), static_cast<UINT>(ThreadNum.y), static_cast<UINT>(ThreadNum.z));

	ID3D12CommandList* commandList[] = { cmdList.Get() };
	cmdList->Close();
	m_commandQueue->ExecuteCommandLists(1, commandList);

	ComPtr<ID3D12Fence1> fence;
	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));
	const UINT64 expectValue = 1;
	m_commandQueue->Signal(fence.Get(), expectValue);
	do
	{
	} while (fence->GetCompletedValue() != expectValue);
	m_oneshotCommandAllocator->Reset();
}
