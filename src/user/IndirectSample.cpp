#include "IndirectSample.h"
#include"D3D12App.h"
#include"Camera.h"

static const float MIN_SCALE = 0.01f;
static const float MAX_SCALE = 0.1f;
static const float MIN_VEL = 0.003f;
static const float MAX_VEL = 0.05f;
static const float RANGE = 15.0f;
//static const Vec3<float> MIN_OFFSET = { -RANGE,-RANGE,-RANGE };
static const Vec3<float> MIN_OFFSET = { -RANGE,-RANGE,-RANGE };
//static const Vec3<float> MAX_OFFSET = { RANGE,RANGE,RANGE };
static const Vec3<float> MAX_OFFSET = { RANGE,-RANGE,RANGE };
static const float COL_MIN = 0.5f;
static const float COL_MAX = 0.9f;
static const UINT COMPUTE_THREAD_BLOCK_SIZE = 128;

void IndirectSample::GenerateCommandBuffers(const size_t& CommandSize)
{
	using namespace Microsoft::WRL;

	if (!m_invalidCommandBuffer)return;

	auto device = D3D12App::Instance()->GetDevice();

	//コマンドバッファ生成
	{
		//ヒーププロパティ
		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		//リソース設定
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(CommandSize * s_blockNum);
		//リソースバリア
		auto barrier = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		ComPtr<ID3D12Resource1>buff;
		auto hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			barrier,
			nullptr,
			IID_PPV_ARGS(&buff));
		buff->SetName(L"IndirectSample - CommandBuffer");

		//SRV設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = s_blockNum;
		srvDesc.Buffer.StructureByteStride = sizeof(CommandSize);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		//SRV作成
		auto srvDescHandles = D3D12App::Instance()->CreateSRV(buff, srvDesc);

		//まとめる
		m_commandBuffer = std::make_shared<DescriptorData>(buff, barrier);
		//SRVディスクリプタ登録
		m_commandBuffer->InitDescHandle(SRV, srvDescHandles);
	}
	//カウンターバッファ生成
	{
		m_counterBuffer = IndirectDevice::GenerateCounterBuffer(device);
	}

	//カリング処理後のコマンドバッファ作成
	{
		//ヒーププロパティ
		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		//リソース設定
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(CommandSize * s_blockNum,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		//リソースバリア
		auto barrier = D3D12_RESOURCE_STATE_COPY_DEST;

		ComPtr<ID3D12Resource1>buff;
		auto hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			barrier,
			nullptr,
			IID_PPV_ARGS(&buff));
		buff->SetName(L"IndirectSample - ProcessedCommandBuffer");

		//UAV設定
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = s_blockNum;
		uavDesc.Buffer.StructureByteStride = CommandSize;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		//UAV作成
		auto uavDescHandles = D3D12App::Instance()->CreateUAV(buff, uavDesc, m_counterBuffer->GetBuff());

		//まとめる
		m_processedCommandBuffer = std::make_shared<DescriptorData>(buff, barrier);
		//SRVディスクリプタ登録
		m_processedCommandBuffer->InitDescHandle(UAV, uavDescHandles);
	}

	m_invalidCommandBuffer = false;
}

IndirectSample::IndirectSample()
{
	//ブロックの個体情報生成
	for (auto& b : m_blockDataArray)
	{
		b.m_scale = KuroFunc::GetRand(MIN_SCALE, MAX_SCALE);
		b.m_vel = { 0,KuroFunc::GetRand(MIN_VEL,MAX_VEL),0 };
		b.m_offset = KuroFunc::GetRand(MIN_OFFSET, MAX_OFFSET);
		b.m_color.m_r = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.m_color.m_g = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.m_color.m_b = KuroFunc::GetRand(COL_MIN, COL_MAX);
	}
	m_blockBuff = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Block), s_blockNum, m_blockDataArray.data(), "IndirectSample - BlockBuffer");

	//グラフィックス用ルートパラメータ
	std::vector<RootParam>gRootParams
	{
		RootParam(CBV,"カメラ定数バッファ"),
		RootParam(CBV,"各ブロック定数バッファ"),
	};

	//グラフィックスパイプライン生成
	{
		//パイプライン設定
		PipelineInitializeOption gPipelineOption(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		//シェーダー情報
		Shaders gPipelineShaders;
		gPipelineShaders.m_vs = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "VSmain", "vs_6_4");
		gPipelineShaders.m_gs = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "GSmain", "gs_6_4");
		gPipelineShaders.m_ps = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "PSmain", "ps_6_4");

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>renderTargetInfo = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_None) };

		//パイプライン生成
		m_gPipeline = D3D12App::Instance()->GenerateGraphicsPipeline(
			gPipelineOption, 
			gPipelineShaders,
			{ InputLayoutParam("POSITION",DXGI_FORMAT_R32G32B32_FLOAT) },
			gRootParams,
			renderTargetInfo, 
			{ WrappedSampler(false, false) });
	}

	//コンピュートパイプライン生成
	{
		//シェーダー
		auto cs = D3D12App::Instance()->CompileShader(
			"resource/user/IndirectSample_Calling.hlsl", "CSmain", "cs_6_0");

		//ルートパラメータ
		std::vector<RootParam>cRootParams
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ定数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カリング情報"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"各ブロック個体情報"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"コマンドバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"カリング後のコマンドバッファ"),
		};

		m_cPipeline = D3D12App::Instance()->GenerateComputePipeline(
			cs, cRootParams, { WrappedSampler(false,false) }
		);
	}

	m_indirectDev = D3D12App::Instance()->GenerateIndirectDevice(EXCUTE_INDIRECT_TYPE::DRAW, gRootParams, { WrappedSampler(false,false) });

	Vec3<float>initPos = { 0,0,0 };
	m_vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), 1, &initPos, "IndirectSample - VertexBuffer");

	m_callingConfigBuffer = D3D12App::Instance()->GenerateConstantBuffer(sizeof(CallingConfig), 1, &m_callingConfig, "IndirectSample - CallingConfig");
}

void IndirectSample::Init(Camera& Cam)
{
	std::array<IndirectCommand<2>, s_blockNum>commands;
	//std::array<IndirectCommand<1>, s_blockNum>commands;
	D3D12_GPU_VIRTUAL_ADDRESS camBuffAddress = Cam.GetBuff()->GetResource()->GetBuff()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS blockBuffAddress = m_blockBuff->GetResource()->GetBuff()->GetGPUVirtualAddress();
	auto incrementSize = sizeof(Block);
	for (auto& com : commands)
	{
		com.m_drawArgs.VertexCountPerInstance = 1;
		com.m_drawArgs.InstanceCount = 1;
		com.m_drawArgs.StartInstanceLocation = 0;
		com.m_drawArgs.StartVertexLocation = 0;

		//CBV0（カメラ情報）
		com.m_gpuAddressArray[0] = camBuffAddress;

		//CBV1（ブロック情報）
		com.m_gpuAddressArray[1] = blockBuffAddress;
		//com.m_gpuAddressArray[0] = blockBuffAddress;
		blockBuffAddress += incrementSize;

		//SRV0（テクスチャ情報）
	}

	size_t commandDataSize = commands.front().GetSize();

	GenerateCommandBuffers(commandDataSize);

	//コマンドバッファにデータ転送
	D3D12App::Instance()->UploadCPUResource(m_commandBuffer->GetResource(), commandDataSize, s_blockNum, commands.data());
}

void IndirectSample::Update(bool EnableCalling)
//void IndirectSample::Update(bool EnableCalling, Camera& Cam)
{
	for (auto& b : m_blockDataArray)
	{
		//b.proj = Cam.GetProjectionMat();
		//b.view = Cam.GetViewMat();
		//b.m_offset += b.m_vel;
		if (MAX_OFFSET.y < b.m_offset.y)
		{
			b.m_offset = KuroFunc::GetRand(MIN_OFFSET, MAX_OFFSET);
			b.m_offset.y = MIN_OFFSET.y;
			b.m_scale = KuroFunc::GetRand(MIN_SCALE, MAX_SCALE);
			b.m_vel = { 0,KuroFunc::GetRand(MIN_VEL,MAX_VEL),0 };
			b.m_color.m_r = KuroFunc::GetRand(COL_MIN, COL_MAX);
			b.m_color.m_g = KuroFunc::GetRand(COL_MIN, COL_MAX);
			b.m_color.m_b = KuroFunc::GetRand(COL_MIN, COL_MAX);
		}
	}
	m_blockBuff->Mapping(m_blockDataArray.data());

	m_enableCulling = EnableCalling;
}

void IndirectSample::Draw(Camera& Cam)
{
	auto cmdList = D3D12App::Instance()->GetCmdList();

	if (m_enableCulling)
	{
		//コンピュート
		m_cPipeline->SetPipeline(cmdList);

		//カメラセット
		Cam.GetBuff()->SetComputeDescriptorBuffer(cmdList, CBV, 0);

		//カリング情報
		m_callingConfigBuffer->SetComputeDescriptorBuffer(cmdList, CBV, 1);

		//各ブロックの個体情報
		m_blockBuff->SetComputeDescriptorBuffer(cmdList, SRV, 2);

		//コマンドバッファ
		m_commandBuffer->SetComputeDescriptorBuffer(cmdList, SRV, 3);

		//UAVようにリソースバリア変更
		m_processedCommandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		//カリング処理済バッファ
		m_processedCommandBuffer->SetComputeDescriptorBuffer(cmdList, UAV, 4);

		//カウンターバッファのリセット
		IndirectDevice::ResetCounterBuffer(cmdList, m_counterBuffer);


		//実行
		auto threadX = static_cast<UINT>(ceil(s_blockNum / float(COMPUTE_THREAD_BLOCK_SIZE)));
		cmdList->Dispatch(threadX, 1, 1);
	}

	//グラフィックス
	m_gPipeline->SetPipeline(cmdList);

	cmdList->IASetVertexBuffers(0, 0, &m_vertBuff->GetVBView());

	if (m_enableCulling)
	{
		m_processedCommandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		m_indirectDev->Execute(
			cmdList,
			s_blockNum,
			m_processedCommandBuffer->GetResource()->GetBuff().Get(),
			0,
			m_counterBuffer);
	}
	else
	{
		m_commandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		m_indirectDev->Execute(
			cmdList,
			s_blockNum,
			m_commandBuffer->GetResource()->GetBuff().Get(),
			0);

		m_commandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
}
