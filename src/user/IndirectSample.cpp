#include "IndirectSample.h"
#include"D3D12App.h"
#include"Camera.h"

static const float MIN_SCALE = 0.1f;
static const float MAX_SCALE = 1.0f;
static const float MIN_VEL = 0.02f;
static const float MAX_VEL = 0.5f;
static const float MIN_OFFSET = 0.0f;
static const float MAX_OFFSET = 6.0f;
static const float COL_MIN = 0.5f;
static const float COL_MAX = 1.0f;

IndirectSample::IndirectSample()
{
	//ブロックの個体情報生成
	for (auto& b : blockDataArray)
	{
		b.scale = KuroFunc::GetRand(MIN_SCALE, MAX_SCALE);
		b.vel = { 0,KuroFunc::GetRand(MIN_VEL,MAX_VEL),0 };
		b.offset = { 0,KuroFunc::GetRand(MIN_OFFSET, MAX_OFFSET),0 };
		b.color.r = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.color.g = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.color.b = KuroFunc::GetRand(COL_MIN, COL_MAX);
	}
	blockBuff = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Block), BLOCK_NUM, blockDataArray.data(), "IndirectSample - BlockBuffer");

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
		gPipelineShaders.vs = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "VSmain", "vs_5_0");
		gPipelineShaders.gs = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "GSmain", "gs_5_0");
		gPipelineShaders.ps = D3D12App::Instance()->CompileShader("resource/user/IndirectSample_Block.hlsl", "PSmain", "ps_5_0");

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>renderTargetInfo = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_None) };

		//パイプライン生成
		gPipeline = D3D12App::Instance()->GenerateGraphicsPipeline(
			gPipelineOption, 
			gPipelineShaders,
			{ InputLayoutParam("POSITION",DXGI_FORMAT_R32G32B32_FLOAT) },
			rootParams, 
			renderTargetInfo, 
			{ WrappedSampler(false, false) });
	}

	indirectDev = D3D12App::Instance()->GenerateIndirectDevice(EXCUTE_INDIRECT_TYPE::DRAW, rootParams, { WrappedSampler(false,false) });

	Vec3<float>initPos = { 0,0,0 };
	vertBuff = D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vec3<float>), 1, &initPos, "IndirectSample - VertexBuffer");
}

void IndirectSample::Init(Camera& Cam)
{
	std::array<IndirectCommand, BLOCK_NUM>commands;
	D3D12_GPU_VIRTUAL_ADDRESS camBuffAddress = Cam.GetBuff()->GetResource()->GetBuff()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS blockBuffAddress = blockBuff->GetResource()->GetBuff()->GetGPUVirtualAddress();
	for (auto& com : commands)
	{
		com.drawArgs.VertexCountPerInstance = 1;
		com.drawArgs.InstanceCount = 1;
		com.drawArgs.StartInstanceLocation = 0;
		com.drawArgs.StartVertexLocation = 0;

		//CBV0（カメラ情報）
		com.gpuAddressArray.emplace_back(camBuffAddress);

		//CBV1（ブロック情報）
		com.gpuAddressArray.emplace_back(blockBuffAddress);
		blockBuffAddress += sizeof(Block);

		//SRV0（テクスチャ情報）
	}
	commandBuffer = D3D12App::Instance()->GenerateStructuredBuffer(commands.front().GetSize(), BLOCK_NUM, commands.data(), "IndirectSample - CommandBuffer");
}

void IndirectSample::Update()
{
	for (auto& b : blockDataArray)
	{
		b.offset += b.vel;
		if (MAX_OFFSET < b.offset.y)
		{
			b.offset.y = MIN_OFFSET;

			b.scale = KuroFunc::GetRand(MIN_SCALE, MAX_SCALE);
			b.vel = { 0,KuroFunc::GetRand(MIN_VEL,MAX_VEL),0 };
			b.color.r = KuroFunc::GetRand(COL_MIN, COL_MAX);
			b.color.g = KuroFunc::GetRand(COL_MIN, COL_MAX);
			b.color.b = KuroFunc::GetRand(COL_MIN, COL_MAX);
		}
	}
	blockBuff->Mapping(blockDataArray.data());
}

void IndirectSample::Draw()
{
	auto cmdList = D3D12App::Instance()->GetCmdList();
	gPipeline->SetPipeline(cmdList);

	cmdList->IASetVertexBuffers(0, 0, &vertBuff->GetVBView());

	indirectDev->Excute(
		cmdList,
		BLOCK_NUM,
		commandBuffer->GetResource()->GetBuff().Get(),
		0,
		nullptr,
		0);
}
