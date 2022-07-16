#include "IndirectSample.h"
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

	//ルートパラメータ
	std::vector<RootParam>rootParams
	{
		RootParam(CBV,"カメラ定数バッファ"),
		RootParam(CBV,"各ブロック定数バッファ"),
		//RootParam(SRV,"テクスチャ")
	};

	//グラフィックスパイプライン生成
	{
		//パイプライン設定
		PipelineInitializeOption gPipelineOption(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		//シェーダー情報
		Shaders gPipelineShaders;
		gPipelineShaders.m_vs = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "VSmain", "vs_5_0");
		gPipelineShaders.m_gs = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "GSmain", "gs_5_0");
		gPipelineShaders.m_ps = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "PSmain", "ps_5_0");

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>renderTargetInfo = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_None) };

		//パイプライン生成
		m_gPipeline = D3D12App::Instance()->GenerateGraphicsPipeline(
			gPipelineOption, 
			gPipelineShaders,
			{ InputLayoutParam("POSITION",DXGI_FORMAT_R32G32B32_FLOAT) },
			rootParams, 
			renderTargetInfo, 
			{ WrappedSampler(false, false) });
	}

	m_indirectDev = D3D12App::Instance()->GenerateIndirectDevice(EXCUTE_INDIRECT_TYPE::DRAW, rootParams, { WrappedSampler(false,false) });

	Vec3<float>initPos = { 0,0,0 };
	m_vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), 1, &initPos, "IndirectSample - VertexBuffer");
}

void IndirectSample::Init(Camera& Cam)
{
	std::array<IndirectCommand<2>, s_blockNum>commands;
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

	//コマンドバッファ生成
	m_commandBuffer = D3D12App::Instance()->GenerateRWStructuredBuffer(commands.front().GetSize(), s_blockNum, commands.data());
}

void IndirectSample::Update()
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
}

void IndirectSample::Draw()
{
	auto cmdList = D3D12App::Instance()->GetCmdList();
	m_gPipeline->SetPipeline(cmdList);

	cmdList->IASetVertexBuffers(0, 0, &m_vertBuff->GetVBView());

	m_commandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	m_indirectDev->Excute(
		cmdList,
		s_blockNum,
		m_commandBuffer->GetResource()->GetBuff().Get(),
		0,
		false);

	m_commandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}
