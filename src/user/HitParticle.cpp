#include "HitParticle.h"
#include"D3D12App.h"
#include"Camera.h"
#include"KuroEngine.h"

static const float COL_MIN = 0.5f;
static const float COL_MAX = 0.9f;
static const UINT COMPUTE_THREAD_BLOCK_SIZE = 128;

HitParticle::HitParticle()
{
	//ブロックの個体情報生成
	m_particleBuff = D3D12App::Instance()->GenerateRWStructuredBuffer(sizeof(Particle), s_particleNum, nullptr, "IndirectSample - BlockBuffer");

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

	//更新用コンピュートパイプライン
	{
		//シェーダー
		auto cs = D3D12App::Instance()->CompileShader(
			"resource/user/shaders/HitParticle_Compute.hlsl", "CSmain", "cs_6_0");

		//ルートパラメータ
		std::vector<RootParam>cRootParams
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"設定"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"各ブロック個体情報"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"コマンドバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,"カリング後のコマンドバッファ"),
		};

		m_cUpdatePipeline = D3D12App::Instance()->GenerateComputePipeline(
			cs, cRootParams, { WrappedSampler(false,false) });
	}


	m_indirectDev = D3D12App::Instance()->GenerateIndirectDevice(EXCUTE_INDIRECT_TYPE::DRAW, gRootParams, { WrappedSampler(false,false) });

	Vec3<float>initPos = { 0,0,0 };
	m_vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), 1, &initPos, "IndirectSample - VertexBuffer");

	m_configBuffer = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Config), 1, &m_config, "IndirectSample - CallingConfig");
}

void HitParticle::Init(Camera& Cam)
{
	//パーティクルの初期化
	std::array<Particle, s_particleNum>particleInitData;
	m_particleBuff->Mapping(particleInitData.data());

	if (m_invalidCommandBuffer)
	{
		//初期化
		std::array<IndirectDrawCommand<2>, s_particleNum>commands;
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

		//コマンドバッファ作成
		m_commandBuffer = D3D12App::Instance()->GenerateIndirectCommandBuffer(DRAW, s_particleNum, 2, false, commands.data(), "HitParticle - AllParticle");
		m_aliveCommandBuffer = D3D12App::Instance()->GenerateIndirectCommandBuffer(DRAW, s_particleNum, 2, true, nullptr, "HitParticle - AliveParticle");

		m_invalidCommandBuffer = false;
	}

	//生存パーティクルコマンドのカウンターバッファのリセット
	m_aliveCommandBuffer->ResetCounterBuffer();
}

void HitParticle::Update()
{
	//m_config.camAddress = Cam.GetBuff()->GetResource()->GetBuff()->GetGPUVirtualAddress();
	//m_configBuffer->Mapping(&m_config);

	//カウンタバッファリセット
	m_aliveCommandBuffer->ResetCounterBuffer();

	auto threadX = static_cast<int>(ceil(s_particleNum / float(COMPUTE_THREAD_BLOCK_SIZE)));
	std::vector<std::shared_ptr<DescriptorData>>datas = {
			m_configBuffer,
			m_particleBuff,
			m_commandBuffer->GetBuff(),
			m_aliveCommandBuffer->GetBuff()
	};
	std::vector<DESC_HANDLE_TYPE>types = { CBV,UAV,SRV,UAV };
	D3D12App::Instance()->DispathOneShot(m_cUpdatePipeline, Vec3<int>(threadX, 1, 1), datas, types);
}

void HitParticle::Draw(Camera& Cam)
{
	//グラフィックス
	KuroEngine::Instance()->Graphics().SetGraphicsPipeline(m_gPipeline);

	KuroEngine::Instance()->Graphics().ExecuteIndirectDraw(m_vertBuff, m_aliveCommandBuffer, m_indirectDev);
}

void HitParticle::Emit(int Num, Vec3<float>Pos)
{
	//CPU上でのパーティクルバッファのポインタ取得
	Particle* ptArray = m_particleBuff->GetResource()->GetBuffOnCpu<Particle>();

	int lifeSpan = KuroFunc::GetRand(45, 60);
	Color col;
	col.m_r = KuroFunc::GetRand(0.3f, 0.5f);
	col.m_g = KuroFunc::GetRand(0.3f, 0.5f);
	col.m_b = KuroFunc::GetRand(0.8f, 1.0f);

	int generateNum = 0;
	for (int ptIdx = 0; ptIdx < s_particleNum; ++ptIdx)
	{
		//パーティクル取得
		auto& pt = ptArray[ptIdx];

		//稼働中ならスルー
		if (pt.m_life)continue;

		pt.m_pos = Pos;
		pt.m_vel = KuroFunc::GetRand(Vec3<float>(-1, -1, -1), Vec3<float>(1, 1, 1));
		pt.m_color = col;
		pt.m_life = 1.0f;
		pt.m_lifeSpan = lifeSpan;

		//指定数分、生成完了
		if (Num == ++generateNum)return;
	}

	//足りなかった
	assert(0);
}
