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

void HitParticle::GenerateCommandBuffers(const int& GpuAddressNum)
{
	using namespace Microsoft::WRL;

	//コマンドバッファ作成
	{
		m_deadComBuffer = D3D12App::Instance()->GenerateIndirectCommandBuffer(DRAW, 2, GpuAddressNum, false, nullptr, "HitParticle - DeadParticle");
		m_aliveComBuffer = D3D12App::Instance()->GenerateIndirectCommandBuffer(DRAW, 2, GpuAddressNum, true, nullptr, "HitParticle - AliveParticle");
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

	std::array<IndirectDrawCommand<2>, s_particleNum>commands;
	//std::array<IndirectDrawCommand<1>, s_blockNum>commands;
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

	//コマンドバッファ生成していなかったら生成
	if (m_invalidCommandBuffer)GenerateCommandBuffers(2);

	//生存パーティクルコマンドのカウンターバッファのリセット
	m_aliveComBuffer->ResetCounterBuffer();

	//初期化用コンピュートパイプラインを走らせる
	D3D12App::Instance()->DispathOneShot(
		m_cInitPipeline,
		Vec3<int>(GetThreadNumX(), 1, 1),
		{
			m_deadComBuffer->GetBuff(),
			m_aliveComBuffer->GetBuff()
		},
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
