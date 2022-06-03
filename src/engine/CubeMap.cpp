#include "CubeMap.h"
#include"KuroEngine.h"
#include"Camera.h"
#include"DrawFunc3D.h"
#include"DrawFunc2D.h"

std::shared_ptr<TextureBuffer>CubeMap::DEFAULT_CUBE_MAP_TEX;

const std::array<std::string, CubeMap::SURFACE_NUM>CubeMap::SURFACE_NAME_TAG =
{
	"- Front(+Z)",
	"- Back(-Z)",
	"- Right(+X)",
	"- Left(-X)",
	"- Up(+Y)",
	"- Down(-Y)",
};

CubeMap::CubeMap()
{
	//デフォルトのテクスチャ
	if (!DEFAULT_CUBE_MAP_TEX)
	{
		DEFAULT_CUBE_MAP_TEX = D3D12App::Instance()->GenerateTextureBuffer("resource/engine/whiteCube.dds", true);
	}

	cubeMap = DEFAULT_CUBE_MAP_TEX;
}

void CubeMap::CopyCubeMap(std::shared_ptr<CubeMap> Src)
{
	this->cubeMap->CopyTexResource(D3D12App::Instance()->GetCmdList(), Src->cubeMap.get());
}

std::shared_ptr<StaticallyCubeMap>& StaticallyCubeMap::GetDefaultCubeMap()
{
	static std::shared_ptr<StaticallyCubeMap>DEFAULT_CUBE_MAP;
	if (!DEFAULT_CUBE_MAP)
	{
		DEFAULT_CUBE_MAP = std::make_shared<StaticallyCubeMap>("DEFAULT_STATICALLY_CUBE_MAP");
	}

	return DEFAULT_CUBE_MAP;
}

void StaticallyCubeMap::ResetMeshVertices()
{
	//頂点の順番
	static const enum { LB, LT, RB, RT, IDX_NUM };

	//FRONT面からのオフセット回転行列
	static const std::array<Matrix, SURFACE_NUM>OFFSET_MAT =
	{
		XMMatrixIdentity(),	//基準
		KuroMath::RotateMat({0.0f,Angle(180),0.0f}),
		KuroMath::RotateMat({0,0,1},{1,0,0}),
		KuroMath::RotateMat({0,0,1},{-1,0,0}),
		KuroMath::RotateMat({0,0,1},{0,1,0}),
		KuroMath::RotateMat({0,0,1},{0,-1,0}),
	};

	//辺の長さの半分
	const float sideLengthHalf = sideLength * 0.5f;

	//基準となるFRONT(+Z)
	surfaces[FRONT].mesh.vertices.resize(IDX_NUM);
	surfaces[FRONT].mesh.vertices[LB].pos = { -sideLengthHalf,-sideLengthHalf,sideLengthHalf };
	surfaces[FRONT].mesh.vertices[LB].uv = { 0.0f,1.0f };
	surfaces[FRONT].mesh.vertices[LT].pos = { -sideLengthHalf,sideLengthHalf,sideLengthHalf };
	surfaces[FRONT].mesh.vertices[LT].uv = { 0.0f,0.0f };
	surfaces[FRONT].mesh.vertices[RB].pos = { sideLengthHalf,-sideLengthHalf,sideLengthHalf };
	surfaces[FRONT].mesh.vertices[RB].uv = { 1.0f,1.0f };
	surfaces[FRONT].mesh.vertices[RT].pos = { sideLengthHalf,sideLengthHalf,sideLengthHalf };
	surfaces[FRONT].mesh.vertices[RT].uv = { 1.0f,0.0f };

	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		if (surfaceIdx == FRONT)continue;	//基準面だった場合はスルー

		surfaces[surfaceIdx].mesh.vertices.resize(IDX_NUM);

		for (int vertIdx = 0; vertIdx < IDX_NUM; ++vertIdx)
		{
			//基準面の頂点を回転させて求める
			surfaces[surfaceIdx].mesh.vertices[vertIdx].pos = KuroMath::TransformVec3(surfaces[FRONT].mesh.vertices[vertIdx].pos, OFFSET_MAT[surfaceIdx]);
			//UVは同じ
			surfaces[surfaceIdx].mesh.vertices[vertIdx].uv = surfaces[FRONT].mesh.vertices[vertIdx].uv;
		}
	}
}

StaticallyCubeMap::StaticallyCubeMap(const std::string& Name, const float& SideLength) : name(Name), sideLength(SideLength)
{
	//デフォルトのテクスチャ
	static std::shared_ptr<TextureBuffer>DEFAULT_TEX = D3D12App::Instance()->GenerateTextureBuffer(Color(1.0f, 1.0f, 1.0f, 1.0f));

	ResetMeshVertices();

	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		//面ごとのメッシュの名前設定
		surfaces[surfaceIdx].mesh.name = name + SURFACE_NAME_TAG[surfaceIdx];

		//メッシュのバッファ生成
		surfaces[surfaceIdx].mesh.CreateBuff();

		//デフォルトのテクスチャアタッチ
		surfaces[surfaceIdx].tex = DEFAULT_TEX;
	}
}

void StaticallyCubeMap::SetSideLength(const float& Length)
{
	sideLength = Length;
	ResetMeshVertices();
	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		surfaces[surfaceIdx].mesh.Mapping();
	}
}

void StaticallyCubeMap::Draw(Camera& Cam)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE;

	//パイプライン未生成
	if (!PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/CubeMap.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/CubeMap.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"カラーテクスチャ"),
		};

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION",DXGI_FORMAT_R32G32B32_FLOAT),
			InputLayoutParam("TEXCOORD",DXGI_FORMAT_R32G32_FLOAT),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_None) };
		//パイプライン生成
		PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false) });
	}

	KuroEngine::Instance().Graphics().SetPipeline(PIPELINE);

	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		auto& s = surfaces[surfaceIdx];
		KuroEngine::Instance().Graphics().ObjectRender(
			s.mesh.vertBuff,
			{ Cam.GetBuff(), s.tex },
			{ CBV,SRV },
			sideLength * 0.5f,
			false);
	}
}

int DynamicCubeMap::ID = 0;
std::array<std::unique_ptr<Camera>, DynamicCubeMap::SURFACE_NUM>DynamicCubeMap::CAMERA;

DynamicCubeMap::DynamicCubeMap(const int& CubeMapEdge)
{
	if (ID == 0)
	{
		static std::array<Vec3<float>, SURFACE_NUM>TARGET =
		{
			Vec3<float>(1,0,0),
			Vec3<float>(-1,0,0),
			Vec3<float>(0,1,0),
			Vec3<float>(0,-1,0),
			Vec3<float>(0,0,1),
			Vec3<float>(0,0,-1)
		};

		static std::array<Vec3<float>, SURFACE_NUM>UP =
		{
			Vec3<float>(0,1,0),
			Vec3<float>(0,1,0),
			Vec3<float>(0,0,-1),
			Vec3<float>(0,0,1),
			Vec3<float>(0,1,0),
			Vec3<float>(0,1,0)
		};

		for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
		{
			CAMERA[surfaceIdx] = std::make_unique<Camera>("DynamicCubeMap" + SURFACE_NAME_TAG[surfaceIdx]);
			CAMERA[surfaceIdx]->SetPos({ 0,0,0 });
			CAMERA[surfaceIdx]->SetAngleOfView(Angle(90));
			CAMERA[surfaceIdx]->SetTarget(TARGET[surfaceIdx]);
			CAMERA[surfaceIdx]->SetAspect(1.0f);
			CAMERA[surfaceIdx]->SetUp(UP[surfaceIdx]);
		}
	}

#pragma region キューブマップテクスチャバッファ生成
	auto texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		texFormat,
		CubeMapEdge, CubeMapEdge, 6, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	D3D12_CLEAR_VALUE clearTexValue;
	clearTexValue.Format = texFormat;
	clearTexValue.Color[0] = 0.0f;
	clearTexValue.Color[1] = 0.0f;
	clearTexValue.Color[2] = 0.0f;
	clearTexValue.Color[3] = 0.0f;

	const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto texBarrier = D3D12_RESOURCE_STATE_RENDER_TARGET;

	Microsoft::WRL::ComPtr<ID3D12Resource1>buff;
	HRESULT hr = D3D12App::Instance()->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		texBarrier,
		&clearTexValue,
		IID_PPV_ARGS(&buff));
	
	std::wstring name = L"DynamicCubeMap" + std::to_wstring(ID++);
	buff->SetName(name.c_str());

	D3D12_SHADER_RESOURCE_VIEW_DESC cubeMapSrvDesc{};
	cubeMapSrvDesc.Format = texFormat;
	cubeMapSrvDesc.TextureCube.MipLevels = 1;
	cubeMapSrvDesc.TextureCube.MostDetailedMip = 0;
	cubeMapSrvDesc.TextureCube.ResourceMinLODClamp = 0;
	cubeMapSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	cubeMapSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	auto srvDescHandles = D3D12App::Instance()->CreateSRV(buff, cubeMapSrvDesc);

	cubeMap = std::make_shared<TextureBuffer>(buff, texBarrier, srvDescHandles, texDesc);
#pragma endregion

#pragma region キューブマップデプスステンシル生成

	CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		CubeMapEdge, CubeMapEdge, 6, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	CD3DX12_CLEAR_VALUE clearDepthValue{};
	clearDepthValue.Format = depthDesc.Format;
	clearDepthValue.DepthStencil.Depth = 1.0f;

	auto depthBarrier = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	Microsoft::WRL::ComPtr<ID3D12Resource1>depthBuff;
	hr = D3D12App::Instance()->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		depthBarrier,
		&clearDepthValue,
		IID_PPV_ARGS(&depthBuff));

	D3D12_DEPTH_STENCIL_VIEW_DESC cubeMapDsvDesc{};
	cubeMapDsvDesc.Format = depthDesc.Format;
	cubeMapDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	cubeMapDsvDesc.Texture2DArray.ArraySize = 6;
	cubeMapDsvDesc.Texture2DArray.FirstArraySlice = 0;
	auto dsvDescHandles = D3D12App::Instance()->CreateDSV(depthBuff, &cubeMapDsvDesc);

	cubeDepth = std::make_shared<DepthStencil>(depthBuff, depthBarrier, dsvDescHandles, depthDesc);

#pragma endregion

	//各面ごとのレンダーターゲットとしての設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.ArraySize = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Texture2DArray.ArraySize = 1;

	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		//レンダーターゲット
		rtvDesc.Texture2DArray.FirstArraySlice = surfaceIdx;
		srvDesc.Texture2DArray.FirstArraySlice = surfaceIdx;
		auto rtvDescHandles = D3D12App::Instance()->CreateRTV(buff, &rtvDesc);
		auto srvDescHandles = D3D12App::Instance()->CreateSRV(buff, srvDesc);
		surfaceTargets[surfaceIdx].renderTargets = std::make_shared<RenderTarget>(cubeMap->GetResource(), srvDescHandles, rtvDescHandles, texDesc, Color(0.0f, 0.0f, 0.0f, 0.0f));

		//デプスステシル
		dsvDesc.Texture2DArray.FirstArraySlice = surfaceIdx;
		auto dsvDescHandles = D3D12App::Instance()->CreateDSV(depthBuff, &dsvDesc);
		surfaceTargets[surfaceIdx].depthStencil = std::make_shared<DepthStencil>(depthBuff, depthBarrier, dsvDescHandles, depthDesc, 1.0f);
	}
}

void DynamicCubeMap::Clear()
{
	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		auto& rt = surfaceTargets[surfaceIdx].renderTargets;
		rt->Clear(D3D12App::Instance()->GetCmdList());
		auto& ds = surfaceTargets[surfaceIdx].depthStencil;
		ds->Clear(D3D12App::Instance()->GetCmdList());
	}
}

void DynamicCubeMap::DrawToCubeMap(LightManager& LigManager, const std::vector<std::weak_ptr<ModelObject>>& ModelObject)
{
	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		auto& rt = surfaceTargets[surfaceIdx].renderTargets;
		auto& ds = surfaceTargets[surfaceIdx].depthStencil;

		KuroEngine::Instance().Graphics().SetRenderTargets({ rt }, ds);

		for (auto& modelPtr : ModelObject)
		{
			DrawFunc3D::DrawPBRShadingModel(LigManager, modelPtr, *CAMERA[surfaceIdx], StaticallyCubeMap::GetDefaultCubeMap());
		}
	}
}