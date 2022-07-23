#include "IndirectSample.h"
#include"D3D12App.h"
#include"Camera.h"
#include"KuroEngine.h"

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

void IndirectSample::GenerateCommandBuffers(const int& GpuAddressNum)
{
	using namespace Microsoft::WRL;

	if (!m_invalidCommandBuffer)return;

	auto device = D3D12App::Instance()->GetDevice();

	//コマンドバッファ生成
	{
		m_commandBuffer = D3D12App::Instance()->GenerateIndirectCommandBuffer(DRAW, s_blockNum, GpuAddressNum);
	}

	//カリング処理後のコマンドバッファ作成
	{
		m_processedCommandBuffer = D3D12App::Instance()->GenerateIndirectCommandBuffer(DRAW, s_blockNum, GpuAddressNum, true);
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
		gPipelineShaders.m_vs = D3D12App::Instance()->CompileShader("resource/user/shaders/IndirectSample_Block.hlsl", "VSmain", "vs_6_4");
		gPipelineShaders.m_gs = D3D12App::Instance()->CompileShader("resource/user/shaders/IndirectSample_Block.hlsl", "GSmain", "gs_6_4");
		gPipelineShaders.m_ps = D3D12App::Instance()->CompileShader("resource/user/shaders/IndirectSample_Block.hlsl", "PSmain", "ps_6_4");

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
		auto cs = D3D12App::Instance()->CompileShader(	"resource/user/shaders/IndirectSample_Calling.hlsl", "CSmain", "cs_6_0");

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
	std::array<IndirectDrawCommand<2>, s_blockNum>commands;
	//std::array<IndirectDrawCommand<1>, s_blockNum>commands;
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
		blockBuffAddress += incrementSize;

		//SRV0（テクスチャ情報）
	}

	GenerateCommandBuffers(2);

	//コマンドバッファにデータ転送
	D3D12App::Instance()->UploadCPUResource(m_commandBuffer->GetBuff()->GetResource(), m_commandBuffer->GetCommandSize(), s_blockNum, commands.data());
}

void IndirectSample::Update(float CullingOffset)
{
	for (auto& b : m_blockDataArray)
	{
		b.m_offset += b.m_vel;
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

	m_callingConfig.cullOffset = CullingOffset;

	m_callingConfigBuffer->Mapping(&m_callingConfig);
	//m_enableCulling = EnableCalling;
}

void IndirectSample::Draw(Camera& Cam)
{
	auto cmdList = D3D12App::Instance()->GetCmdList();

	if (m_enableCulling)
	{
		//カウンターバッファリセット
		m_processedCommandBuffer->ResetCounterBuffer();

		KuroEngine::Instance().Graphics().SetComputePipeline(m_cPipeline);

		auto threadX = static_cast<int>(ceil(s_blockNum / float(COMPUTE_THREAD_BLOCK_SIZE)));

		KuroEngine::Instance().Graphics().Dispatch(Vec3<int>(threadX, 1, 1),
			{
				Cam.GetBuff(),
				m_callingConfigBuffer,
				m_blockBuff,
				m_commandBuffer->GetBuff(),
				m_processedCommandBuffer->GetBuff()
			},
			{
				CBV,CBV,SRV,SRV,UAV
			}
			);
	}

	//グラフィックス
	KuroEngine::Instance().Graphics().SetGraphicsPipeline(m_gPipeline);

	if (m_enableCulling)
	{
		KuroEngine::Instance().Graphics().ExecuteIndirectDraw(m_vertBuff, m_processedCommandBuffer, m_indirectDev);
	}
	else
	{
		KuroEngine::Instance().Graphics().ExecuteIndirectDraw(m_vertBuff, m_commandBuffer, m_indirectDev);
	}
}
