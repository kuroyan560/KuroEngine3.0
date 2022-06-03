#include "CubeMap.h"
#include"KuroEngine.h"
#include"Camera.h"

std::shared_ptr<TextureBuffer>CubeMap::DEFAULT_CUBE_MAP_TEX;

CubeMap::CubeMap()
{
	//デフォルトのテクスチャ
	if (!DEFAULT_CUBE_MAP_TEX)
	{
		DEFAULT_CUBE_MAP_TEX = D3D12App::Instance()->GenerateTextureBuffer(Color(1.0f, 1.0f, 1.0f, 1.0f), true);
	}

	cubeMap = DEFAULT_CUBE_MAP_TEX;
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
	static std::shared_ptr<TextureBuffer>DEFAULT_TEX = D3D12App::Instance()->GenerateTextureBuffer(Color(0.0f, 0.0f, 0.0f, 1.0f));

	//メッシュ名に付与するタグ
	static const std::array<std::string, SURFACE_NUM>NAME_TAG =
	{
		"- Front(+Z)",
		"- Back(-Z)",
		"- Right(+X)",
		"- Left(-X)",
		"- Up(+Y)",
		"- Down(-Y)",
	};

	ResetMeshVertices();

	for (int surfaceIdx = 0; surfaceIdx < SURFACE_NUM; ++surfaceIdx)
	{
		//面ごとのメッシュの名前設定
		surfaces[surfaceIdx].mesh.name = name + NAME_TAG[surfaceIdx];

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