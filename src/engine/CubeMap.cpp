#include "CubeMap.h"
#include"KuroEngine.h"
#include"Camera.h"
#include"DrawFunc3D.h"
#include"DrawFunc2D.h"
#include"Model.h"
#include"LightManager.h"

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
	//デフォルトのカラー
	static Color DEFAULT_COLOR(0.4f, 0.4f, 0.4f, 1.0f);

	//デフォルトのテクスチャ
	static std::shared_ptr<TextureBuffer>DEFAULT_TEX = D3D12App::Instance()->GenerateTextureBuffer(DEFAULT_COLOR);

	//デフォルトのキューブマップのテクスチャ
	static std::shared_ptr<TextureBuffer>DEFAULT_CUBE_MAP_TEX;
	if (!DEFAULT_CUBE_MAP_TEX)
	{
		static const int EDGE = 512;
		auto texFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			texFormat,
			EDGE, EDGE, 6, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		const auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		auto texBarrier = D3D12_RESOURCE_STATE_RENDER_TARGET;

		Microsoft::WRL::ComPtr<ID3D12Resource1>buff;
		HRESULT hr = D3D12App::Instance()->GetDevice()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			texBarrier,
			nullptr,
			IID_PPV_ARGS(&buff));

		std::wstring name = L"StaticallyCubeMapTex - Default";
		buff->SetName(name.c_str());

		D3D12_SHADER_RESOURCE_VIEW_DESC cubeMapSrvDesc{};
		cubeMapSrvDesc.Format = texFormat;
		cubeMapSrvDesc.TextureCube.MipLevels = 1;
		cubeMapSrvDesc.TextureCube.MostDetailedMip = 0;
		cubeMapSrvDesc.TextureCube.ResourceMinLODClamp = 0;
		cubeMapSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		cubeMapSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		auto srvDescHandles = D3D12App::Instance()->CreateSRV(buff, cubeMapSrvDesc);

		//キューブマップ用のレンダーターゲットビュー
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = texDesc.Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = 6;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		auto rtvDescHandles = D3D12App::Instance()->CreateRTV(buff, &rtvDesc);

		//単色塗りつぶし
		float clearVal[4] = { DEFAULT_COLOR.r,DEFAULT_COLOR.g,DEFAULT_COLOR.b,DEFAULT_COLOR.a };
		D3D12App::Instance()->GetCmdList()->ClearRenderTargetView(rtvDescHandles, clearVal, 0, nullptr);

		DEFAULT_CUBE_MAP_TEX = std::make_shared<TextureBuffer>(buff, texBarrier, srvDescHandles, texDesc);
	}

	//デフォルトキューブマップ割当
	cubeMap = DEFAULT_CUBE_MAP_TEX;

	//メッシュ情報の構築
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
std::shared_ptr<ConstantBuffer>DynamicCubeMap::VIEW_PROJ_MATRICIES;

DynamicCubeMap::DynamicCubeMap(const int& CubeMapEdge)
{
	if (ID == 0)
	{
		std::array<Vec3<float>, SURFACE_NUM>target =
		{
			Vec3<float>(1,0,0),
			Vec3<float>(-1,0,0),
			Vec3<float>(0,1,0),
			Vec3<float>(0,-1,0),
			Vec3<float>(0,0,1),
			Vec3<float>(0,0,-1)
		};

		std::array<Vec3<float>, SURFACE_NUM>up =
		{
			Vec3<float>(0,1,0),
			Vec3<float>(0,1,0),
			Vec3<float>(0,0,-1),
			Vec3<float>(0,0,1),
			Vec3<float>(0,1,0),
			Vec3<float>(0,1,0)
		};

		std::array<Matrix, SURFACE_NUM>viewProj;
		std::array<std::unique_ptr<Camera>, SURFACE_NUM>camera;	//各面に描画する際に用いるカメラ
		for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
		{
			camera[surfaceIdx] = std::make_unique<Camera>("DynamicCubeMap" + SURFACE_NAME_TAG[surfaceIdx]);
			camera[surfaceIdx]->SetPos({ 0,0,0 });
			camera[surfaceIdx]->SetAngleOfView(Angle(90));
			camera[surfaceIdx]->SetTarget(target[surfaceIdx]);
			camera[surfaceIdx]->SetAspect(1.0f);
			camera[surfaceIdx]->SetUp(up[surfaceIdx]);
			viewProj[surfaceIdx] = camera[surfaceIdx]->GetViewMat() * camera[surfaceIdx]->GetProjectionMat();
		}

		VIEW_PROJ_MATRICIES = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), SURFACE_NUM, viewProj.data(), "DynamicCubeMap - ViewProjMatricies");
	}

#pragma region キューブマップテクスチャバッファ生成
	//auto texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	auto texFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
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

	//キューブマップ用のレンダーターゲットビュー
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 6;
	rtvDesc.Texture2DArray.FirstArraySlice = 0;
	auto rtvDescHandles = D3D12App::Instance()->CreateRTV(buff, &rtvDesc);

	cubeRenderTarget = std::make_shared<RenderTarget>(cubeMap->GetResource(), srvDescHandles, rtvDescHandles, texDesc);

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
}

void DynamicCubeMap::Clear()
{
	cubeRenderTarget->Clear(D3D12App::Instance()->GetCmdList());
	cubeDepth->Clear(D3D12App::Instance()->GetCmdList());
}

void DynamicCubeMap::DrawToCubeMap(LightManager& LigManager, const std::vector<std::weak_ptr<ModelObject>>& ModelObject)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE;

	//パイプライン未生成
	if (!PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DynamicCubeMap.hlsl", "VSmain", "vs_5_0");
		SHADERS.gs = D3D12App::Instance()->CompileShader("resource/engine/DynamicCubeMap.hlsl", "GSmain", "gs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DynamicCubeMap.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"６面分のカメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "アクティブ中のライト数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ディレクションライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ポイントライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "スポットライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "天球ライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"トランスフォームバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ベースカラーテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"メタルネステクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ノーマルマップ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"粗さ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"マテリアル基本情報バッファ"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(cubeRenderTarget->GetDesc().Format, AlphaBlendMode_Trans) };
		//パイプライン生成
		PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false) });
	}

	KuroEngine::Instance().Graphics().SetRenderTargets({ cubeRenderTarget }, cubeDepth);
	KuroEngine::Instance().Graphics().SetPipeline(PIPELINE);

	for (auto& modelPtr : ModelObject)
	{
		auto m = modelPtr.lock();

		for (auto& mesh : m->model->meshes)
		{
			KuroEngine::Instance().Graphics().ObjectRender(
				mesh.mesh->vertBuff,
				mesh.mesh->idxBuff,
				{
					VIEW_PROJ_MATRICIES,
					LigManager.GetLigNumInfo(),
					LigManager.GetLigInfo(Light::DIRECTION),
					LigManager.GetLigInfo(Light::POINT),
					LigManager.GetLigInfo(Light::SPOT),
					LigManager.GetLigInfo(Light::HEMISPHERE),
					m->GetTransformBuff(),
					mesh.material->texBuff[COLOR_TEX],
					mesh.material->texBuff[METALNESS_TEX],
					mesh.material->texBuff[NORMAL_TEX],
					mesh.material->texBuff[ROUGHNESS_TEX],
					mesh.material->buff,
				},
				{ CBV,CBV,SRV,SRV,SRV,SRV,CBV,SRV,SRV,SRV,SRV,CBV },
				m->transform.GetPos().z,
				true);
		}
	}
}