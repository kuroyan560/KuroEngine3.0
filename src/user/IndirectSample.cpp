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

void IndirectSample::GenerateCommandBuffer(std::array<IndirectCommand, BLOCK_NUM>& UploadCommands)
{
	auto device = D3D12App::Instance()->GetDevice();
	auto cmdList = D3D12App::Instance()->GetCmdList();

	const auto commandSize = IndirectCommand::GetSize(2);
	const auto commandArraySize = commandSize * BLOCK_NUM;

	D3D12_RESOURCE_DESC commandBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(commandArraySize);
	auto heapDescDef = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	device->CreateCommittedResource(
		&heapDescDef,
		D3D12_HEAP_FLAG_NONE,
		&commandBufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_commandBuffer));
	m_commandBuffer->SetName(L"IndirectSample - CommandBuffer");

	auto heapDescUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	device->CreateCommittedResource(
		&heapDescUpload,
		D3D12_HEAP_FLAG_NONE,
		&commandBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_uploadCommandBuffer));
	m_uploadCommandBuffer->SetName(L"IndirectSample - UploadCommandBuffer");

	D3D12_SUBRESOURCE_DATA commandData = {};
	commandData.pData = reinterpret_cast<UINT8*>(&UploadCommands[0]);
	commandData.RowPitch = commandArraySize;
	commandData.SlicePitch = commandData.RowPitch;

	UpdateSubresources<1>(cmdList.Get(), m_commandBuffer.Get(), m_uploadCommandBuffer.Get(), 0, 0, 1, &commandData);
	auto barrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(m_commandBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cmdList->ResourceBarrier(1, &barrierTransition);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = BLOCK_NUM;
	srvDesc.Buffer.StructureByteStride = commandSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	auto srvDescHandle = D3D12App::Instance()->CreateSRV(m_commandBuffer, srvDesc);
	//m_commandBuffer = std::make_shared<StructuredBuffer>(m_commandBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srvDescHandle, commandSize, BLOCK_NUM);
}

IndirectSample::IndirectSample()
{
	//ブロックの個体情報生成
	for (auto& b : m_blockDataArray)
	{
		b.scale = KuroFunc::GetRand(MIN_SCALE, MAX_SCALE);
		b.vel = { 0,KuroFunc::GetRand(MIN_VEL,MAX_VEL),0 };
		b.offset = { 0,KuroFunc::GetRand(MIN_OFFSET, MAX_OFFSET),0 };
		b.color.r = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.color.g = KuroFunc::GetRand(COL_MIN, COL_MAX);
		b.color.b = KuroFunc::GetRand(COL_MIN, COL_MAX);
	}
	m_blockBuff = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Block), BLOCK_NUM, m_blockDataArray.data(), "IndirectSample - BlockBuffer");

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
	std::array<IndirectCommand, BLOCK_NUM>commands;
	D3D12_GPU_VIRTUAL_ADDRESS camBuffAddress = Cam.GetBuff()->GetResource()->GetBuff()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS blockBuffAddress = m_blockBuff->GetResource()->GetBuff()->GetGPUVirtualAddress();
	auto incrementSize = sizeof(Block);
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
		blockBuffAddress += incrementSize;

		//SRV0（テクスチャ情報）
	}
	GenerateCommandBuffer(commands);
}

void IndirectSample::Update()
{
	for (auto& b : m_blockDataArray)
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
	m_blockBuff->Mapping(m_blockDataArray.data());
}

void IndirectSample::Draw()
{
	auto cmdList = D3D12App::Instance()->GetCmdList();
	m_gPipeline->SetPipeline(cmdList);

	cmdList->IASetVertexBuffers(0, 0, &m_vertBuff->GetVBView());

	//m_commandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	D3D12_RESOURCE_BARRIER commandBuffBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_commandBuffer.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	cmdList->ResourceBarrier(1, &commandBuffBarrier);

	m_indirectDev->Excute(
		cmdList,
		BLOCK_NUM,
		//m_commandBuffer->GetResource()->GetBuff().Get(),
		m_commandBuffer.Get(),
		0,
		nullptr,
		0);

	//m_commandBuffer->GetResource()->ChangeBarrier(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	commandBuffBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	commandBuffBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	cmdList->ResourceBarrier(1, &commandBuffBarrier);
}
