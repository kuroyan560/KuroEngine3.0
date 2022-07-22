#include "HitParticle.h"
#include"D3D12App.h"
#include"Camera.h"

static const float MIN_SCALE = 0.01f;
static const float MAX_SCALE = 0.1f;
static const float MIN_VEL = 0.003f;
static const float MAX_VEL = 0.05f;
static const float RANGE = 15.0f;
static const Vec3<float> MIN_OFFSET = { -RANGE,-RANGE,-RANGE };
static const Vec3<float> MAX_OFFSET = { RANGE,RANGE,RANGE };
static const float COL_MIN = 0.5f;
static const float COL_MAX = 0.9f;
static const UINT COMPUTE_THREAD_BLOCK_SIZE = 128;

const int HitParticle::GetThreadNumX()
{
	int threadNumX = static_cast<int>(ceil(s_particleNum / s_threadDiv));
	return threadNumX;
}

void HitParticle::GenerateCommandBuffers(const size_t& CommandSize)
{
	using namespace Microsoft::WRL;

	auto device = D3D12App::Instance()->GetDevice();

	//カウンターバッファ生成
	{
		m_aliveComCounterBuffer = IndirectDevice::GenerateCounterBuffer(device);
		//m_deadComCounterBuffer = IndirectDevice::GenerateCounterBuffer(device);
	}
	//コマンドバッファ作成
	{
		//ヒーププロパティ
		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		//リソース設定
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(CommandSize * s_particleNum,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		//リソースバリア
		auto barrier = D3D12_RESOURCE_STATE_COPY_DEST;

		//死亡パーティクルコマンドバッファ
		ComPtr<ID3D12Resource1>deadPtBuff;
		auto hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			barrier,
			nullptr,
			IID_PPV_ARGS(&deadPtBuff));
		deadPtBuff->SetName(L"HitParticle - DeadParticleCommandBuffer");

		//生存パーティクルコマンドバッファ
		ComPtr<ID3D12Resource1>alivePtBuff;
		auto hr = device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			barrier,
			nullptr,
			IID_PPV_ARGS(&alivePtBuff));
		alivePtBuff->SetName(L"HitParticle - AliveParticleCommandBuffer");

		//UAV設定
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = s_particleNum;
		uavDesc.Buffer.StructureByteStride = CommandSize;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		//SRV設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = s_particleNum;
		srvDesc.Buffer.StructureByteStride = CommandSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		//死亡パーティクルコマンド
		auto uavDescHandles = D3D12App::Instance()->CreateUAV(deadPtBuff, uavDesc, m_aliveComCounterBuffer->GetBuff());
		auto srvDescHandles = D3D12App::Instance()->CreateSRV(deadPtBuff, srvDesc);
		m_deadComBuffer = std::make_shared<DescriptorData>(deadPtBuff, barrier);
		m_deadComBuffer->InitDescHandle(UAV, uavDescHandles);
		m_deadComBuffer->InitDescHandle(SRV, srvDescHandles);

		//稼働中パーティクルコマンド
		//uavDescHandles = D3D12App::Instance()->CreateUAV(alivePtBuff, uavDesc, m_deadComCounterBuffer->GetBuff());
		uavDescHandles = D3D12App::Instance()->CreateUAV(alivePtBuff, uavDesc);
		srvDescHandles = D3D12App::Instance()->CreateSRV(alivePtBuff, srvDesc);
		m_aliveComBuffer = std::make_shared<DescriptorData>(alivePtBuff, barrier);
		m_aliveComBuffer->InitDescHandle(UAV, uavDescHandles);
		m_aliveComBuffer->InitDescHandle(SRV, srvDescHandles);
	}

	m_invalidCommandBuffer = false;
}

HitParticle::HitParticle()
{
	//ブロックの個体情報生成
	for (auto& b : m_particleDataArray)
	{
		b.m_scale = KuroFunc::GetRand(MIN_SCALE, MAX_SCALE);
		b.m_vel = { 0,KuroFunc::GetRand(MIN_VEL,MAX_VEL),0 };
		b.m_pos = KuroFunc::GetRand(MIN_OFFSET, MAX_OFFSET);
		b.m_color.m_r = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.m_color.m_g = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.m_color.m_b = KuroFunc::GetRand(COL_MIN, COL_MAX);
	}
	m_particleBuff = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Particle), s_particleNum, m_particleDataArray.data(), "IndirectSample - BlockBuffer");

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
		gPipelineShaders.m_vs = D3D12App::Instance()->CompileShader("resource/user/shaders/HitParticle_Graphics.hlsl", "VSmain", "vs_6_4");
		gPipelineShaders.m_gs = D3D12App::Instance()->CompileShader("resource/user/shaders/HitParticle_Graphics.hlsl", "GSmain", "gs_6_4");
		gPipelineShaders.m_ps = D3D12App::Instance()->CompileShader("resource/user/shaders/HitParticle_Graphics.hlsl", "PSmain", "ps_6_4");

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

	{
		//ルートパラメータ
		std::vector<RootParam>cRootParams
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"死亡パーティクルコマンドバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"稼働中パーティクルコマンドバッファ"),
		};

		//初期化用コンピュートパイプライン
		auto cs = D3D12App::Instance()->CompileShader(
			"resource/user/shaders/HitParticle_Emitter.hlsl", "CSmain_Init", "cs_6_0");
		m_cInitPipeline = D3D12App::Instance()->GenerateComputePipeline(
			cs, cRootParams, { WrappedSampler(false,false) });


		//生成用コンピュートパイプライン
		cs = D3D12App::Instance()->CompileShader(
			"resource/user/shaders/HitParticle_Emitter.hlsl", "CSmain_Emit", "cs_6_0");
		m_cGeneratePipeline = D3D12App::Instance()->GenerateComputePipeline(
			cs, cRootParams, { WrappedSampler(false,false) });
	}

	//更新用コンピュートパイプライン
	{
		//シェーダー
		auto cs = D3D12App::Instance()->CompileShader(
			"resource/user/shaders/HitParticle_Update.hlsl", "CSmain", "cs_6_0");

		//ルートパラメータ
		std::vector<RootParam>cRootParams
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ定数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"設定"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"各ブロック個体情報"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"コマンドバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"カリング後のコマンドバッファ"),
		};

		m_cUpdatePipeline = D3D12App::Instance()->GenerateComputePipeline(
			cs, cRootParams, { WrappedSampler(false,false) });
	}


	m_indirectDev = D3D12App::Instance()->GenerateIndirectDevice(EXCUTE_INDIRECT_TYPE::DRAW, gRootParams, { WrappedSampler(false,false) });

	Vec3<float>initPos = { 0,0,0 };
	m_vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), 1, &initPos, "IndirectSample - VertexBuffer");

	m_configBuffer = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Emitter), 1, &m_config, "IndirectSample - CallingConfig");
}

void HitParticle::Init(Camera& Cam)
{
	auto cmdList = D3D12App::Instance()->GetCmdList();

	std::array<IndirectCommand<2>, s_particleNum>commands;
	//std::array<IndirectCommand<1>, s_blockNum>commands;
	D3D12_GPU_VIRTUAL_ADDRESS camBuffAddress = Cam.GetBuff()->GetResource()->GetBuff()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS blockBuffAddress = m_particleBuff->GetResource()->GetBuff()->GetGPUVirtualAddress();
	auto incrementSize = sizeof(Particle);
	for (auto& com : commands)
	{
		com.m_drawArgs.VertexCountPerInstance = 1;
		com.m_drawArgs.InstanceCount = 1;
		com.m_drawArgs.StartInstanceLocation = 0;
		com.m_drawArgs.StartVertexLocation = 0;

		//CBV0（カメラ情報）
		com.m_gpuAddressArray[0] = camBuffAddress;

		//CBV1（パーティクル情報）
		com.m_gpuAddressArray[1] = blockBuffAddress;
		blockBuffAddress += incrementSize;
	}

	size_t commandDataSize = commands.front().GetSize();

	//コマンドバッファ生成していなかったら生成
	if (m_invalidCommandBuffer)GenerateCommandBuffers(commandDataSize);

	//生存パーティクルコマンドのカウンターバッファのリセット
	IndirectDevice::ResetCounterBuffer(cmdList, m_aliveComCounterBuffer);

	//初期化用コンピュートパイプラインを走らせる
	D3D12App::Instance()->DispathOneShot(
		m_cInitPipeline,
		Vec3<int>(GetThreadNumX(), 1, 1),
		{ m_deadComBuffer,m_aliveComBuffer },
		{ UAV,UAV }
	);

	//コマンドバッファにデータ転送
	D3D12App::Instance()->UploadCPUResource(m_deadComBuffer->GetResource(), commandDataSize, s_particleNum, commands.data());
}

void HitParticle::Update()
{
	for (auto& b : m_particleDataArray)
	{
		b.m_pos += b.m_vel;
		if (MAX_OFFSET.y < b.m_pos.y)
		{
			b.m_pos = KuroFunc::GetRand(MIN_OFFSET, MAX_OFFSET);
			b.m_pos.y = MIN_OFFSET.y;
			b.m_scale = KuroFunc::GetRand(MIN_SCALE, MAX_SCALE);
			b.m_vel = { 0,KuroFunc::GetRand(MIN_VEL,MAX_VEL),0 };
			b.m_color.m_r = KuroFunc::GetRand(COL_MIN, COL_MAX);
			b.m_color.m_g = KuroFunc::GetRand(COL_MIN, COL_MAX);
			b.m_color.m_b = KuroFunc::GetRand(COL_MIN, COL_MAX);
		}
	}
	m_particleBuff->Mapping(m_particleDataArray.data());

	m_configBuffer->Mapping(&m_config);
	//m_enableCulling = EnableCalling;
}

void HitParticle::Draw(Camera& Cam)
{
	auto cmdList = D3D12App::Instance()->GetCmdList();

	//コンピュート
	{
		m_cPipeline->SetPipeline(cmdList);

		//カメラセット
		Cam.GetBuff()->SetComputeDescriptorBuffer(cmdList, CBV, 0);

		//設定情報
		m_configBuffer->SetComputeDescriptorBuffer(cmdList, CBV, 1);

		//各ブロックの個体情報
		m_particleBuff->SetComputeDescriptorBuffer(cmdList, SRV, 2);

		//コマンドバッファ
		m_commandBuffer->SetComputeDescriptorBuffer(cmdList, SRV, 3);

		//UAVようにリソースバリア変更
		m_processedCommandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		//カリング処理済バッファ
		m_processedCommandBuffer->SetComputeDescriptorBuffer(cmdList, UAV, 4);




		//実行
		auto threadX = static_cast<UINT>(ceil(s_particleNum / float(COMPUTE_THREAD_BLOCK_SIZE)));
		cmdList->Dispatch(threadX, 1, 1);
	}

	//グラフィックス
	{
		m_gPipeline->SetPipeline(cmdList);

		cmdList->IASetVertexBuffers(0, 0, &m_vertBuff->GetVBView());

		m_processedCommandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		m_indirectDev->Execute(
			cmdList,
			s_particleNum,
			m_processedCommandBuffer->GetResource()->GetBuff().Get(),
			0,
			m_counterBuffer);
	}
}
