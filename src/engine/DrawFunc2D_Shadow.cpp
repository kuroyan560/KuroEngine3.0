#include "DrawFunc2D_Shadow.h"
#include"KuroEngine.h"
#include"LightManager.h"

std::shared_ptr<ConstantBuffer>DrawFunc2D_Shadow::s_eyePosBuff;
std::shared_ptr<TextureBuffer>DrawFunc2D_Shadow::s_defaultTex;
std::shared_ptr<TextureBuffer>DrawFunc2D_Shadow::s_defaultNormalMap;
std::shared_ptr<TextureBuffer>DrawFunc2D_Shadow::s_defaultEmissiveMap;

//DrawExtendGraph
int DrawFunc2D_Shadow::s_drawExtendGraphCount = 0;

//DrawRotaGraph
int DrawFunc2D_Shadow::s_drawRotaGraphCount = 0;

static std::vector<RootParam>ROOT_PARAMETER =
{
	RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "平行投影行列定数バッファ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "視点座標情報"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "アクティブ中のライト数バッファ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ディレクションライト情報 (構造化バッファ)"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ポイントライト情報 (構造化バッファ)"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "スポットライト情報 (構造化バッファ)"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "天球ライト情報 (構造化バッファ)"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "テクスチャリソース"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "法線マップ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "放射マップ")
};

void DrawFunc2D_Shadow::StaticInit()
{
	if (s_eyePosBuff)return;

	//視点座標
	Vec3<float>INIT_EYE_POS = { WinApp::Instance()->GetExpandWinCenter().x,WinApp::Instance()->GetExpandWinCenter().y,-5.0f };
	s_eyePosBuff = D3D12App::Instance()->GenerateConstantBuffer(sizeof(Vec3<float>), 1, &INIT_EYE_POS, "DrawFunc_Shadow - EYE_POS");

	//白テクスチャ
	s_defaultTex = D3D12App::Instance()->GenerateTextureBuffer(Color(1.0f, 1.0f, 1.0f, 1.0f));

	// (0,0,-1) ノーマルマップ
	s_defaultNormalMap = D3D12App::Instance()->GenerateTextureBuffer(Color(0.5f, 0.5f, 1.0f, 1.0f));

	//黒テクスチャ
	s_defaultEmissiveMap = D3D12App::Instance()->GenerateTextureBuffer(Color(0.0f, 0.0f, 0.0f, 1.0f));
}

void DrawFunc2D_Shadow::SetEyePos(Vec3<float> EyePos)
{
	s_eyePosBuff->Mapping(&EyePos);
}

void DrawFunc2D_Shadow::DrawExtendGraph2D(LightManager& LigManager, const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos, const std::shared_ptr<TextureBuffer>& Tex, const std::shared_ptr<TextureBuffer>& NormalMap, const std::shared_ptr<TextureBuffer>& EmissiveMap, const float& SpriteDepth, const Vec2<bool>& Miror, const float& Diffuse, const float& Specular, const float& Lim)
{
	//DrawExtendGraph専用頂点
	class ExtendGraphVertex
	{
	public:
		Vec2<float>leftUpPos;
		Vec2<float>rightBottomPos;
		Vec2<int> miror;
		float depth;
		float diffuse;
		float specular;
		float lim;
		ExtendGraphVertex(const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos, const Vec2<bool>& Miror,
			const float& Depth, const float& Diffuse, const float& Specular, const float& Lim)
			:leftUpPos(LeftUpPos), rightBottomPos(RightBottomPos), miror({ Miror.x ? 1 : 0 ,Miror.y ? 1 : 0 }),
			depth(Depth), diffuse(Diffuse), specular(Specular), lim(Lim) {}
	};

	static std::shared_ptr<GraphicsPipeline>EXTEND_GRAPH_PIPELINE;
	static std::vector<std::shared_ptr<VertexBuffer>>EXTEND_GRAPH_VERTEX_BUFF;

	//パイプライン未生成
	if (!EXTEND_GRAPH_PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		PIPELINE_OPTION.m_depthTest = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.m_vs = D3D12App::Instance()->CompileShader("resource/engine/DrawExtendGraph_Shadow.hlsl", "VSmain", "vs_6_4");
		SHADERS.m_gs = D3D12App::Instance()->CompileShader("resource/engine/DrawExtendGraph_Shadow.hlsl", "GSmain", "gs_6_4");
		SHADERS.m_ps = D3D12App::Instance()->CompileShader("resource/engine/DrawExtendGraph_Shadow.hlsl", "PSmain", "ps_6_4");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION_L_U",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("POSITION_R_B",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("MIROR",DXGI_FORMAT_R32G32_SINT),
			InputLayoutParam("DEPTH",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("DIFFUSE",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("SPECULAR",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("LIM",DXGI_FORMAT_R32_FLOAT)
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO =
		{
			//バックバッファのフォーマット、アルファブレンド
			RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(),AlphaBlendMode_Trans),
			//エミッシブ
			RenderTargetInfo(DXGI_FORMAT_R32G32B32A32_FLOAT,AlphaBlendMode_Trans)
		};

		//パイプライン生成
		EXTEND_GRAPH_PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, true) });
	}

	StaticInit();

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(EXTEND_GRAPH_PIPELINE);

	if (EXTEND_GRAPH_VERTEX_BUFF.size() < (s_drawExtendGraphCount + 1))
	{
		EXTEND_GRAPH_VERTEX_BUFF.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(ExtendGraphVertex), 1, nullptr, ("DrawExtendGraph_Shadow -" + std::to_string(s_drawExtendGraphCount)).c_str()));
	}

	ExtendGraphVertex vertex(LeftUpPos, RightBottomPos, Miror, SpriteDepth, Diffuse, Specular, Lim);
	EXTEND_GRAPH_VERTEX_BUFF[s_drawExtendGraphCount]->Mapping(&vertex);

	auto normalMap = NormalMap ? NormalMap : s_defaultNormalMap;
	auto emissiveMap = EmissiveMap ? EmissiveMap : s_defaultEmissiveMap;

	KuroEngine::Instance().Graphics().ObjectRender(EXTEND_GRAPH_VERTEX_BUFF[s_drawExtendGraphCount],
		{
			KuroEngine::Instance().GetParallelMatProjBuff(),
			s_eyePosBuff,	//視点座標
			LigManager.GetLigNumInfo(),	//アクティブ中のライト数
			LigManager.GetLigInfo(Light::DIRECTION),	//ディレクションライト
			LigManager.GetLigInfo(Light::POINT),	//ポイントライト
			LigManager.GetLigInfo(Light::SPOT),	//スポットライト
			LigManager.GetLigInfo(Light::HEMISPHERE),	//天球ライト
			Tex,normalMap,emissiveMap
		},
		{ CBV,CBV,CBV,SRV,SRV,SRV,SRV,SRV,SRV,SRV }, 0.0f, true);

	s_drawExtendGraphCount++;
}

void DrawFunc2D_Shadow::DrawRotaGraph2D(LightManager& LigManager, const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian, const std::shared_ptr<TextureBuffer>& Tex, const std::shared_ptr<TextureBuffer>& NormalMap, const std::shared_ptr<TextureBuffer>& EmissiveMap, const float& SpriteDepth, const Vec2<float>& RotaCenterUV, const Vec2<bool>& Miror, const float& Diffuse, const float& Specular, const float& Lim)
{
	//DrawRotaGraph専用頂点
	class RotaGraphVertex
	{
	public:
		Vec2<float>center;
		Vec2<float> extRate;
		float radian;
		Vec2<float>rotaCenterUV;
		Vec2<int> miror;
		float depth;
		float diffuse;
		float specular;
		float lim;
		RotaGraphVertex(const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian,
			const Vec2<float>& RotaCenterUV, const Vec2<bool>& Miror,
			const float& Depth, const float& Diffuse, const float& Specular, const float& Lim)
			:center(Center), extRate(ExtRate), radian(Radian), rotaCenterUV(RotaCenterUV), miror({ Miror.x ? 1 : 0,Miror.y ? 1 : 0 }),
			depth(Depth), diffuse(Diffuse), specular(Specular), lim(Lim) {}
	};

	static std::shared_ptr<GraphicsPipeline>ROTA_GRAPH_PIPELINE;
	static std::vector<std::shared_ptr<VertexBuffer>>ROTA_GRAPH_VERTEX_BUFF;

	//パイプライン未生成
	if (!ROTA_GRAPH_PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		PIPELINE_OPTION.m_depthTest = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.m_vs = D3D12App::Instance()->CompileShader("resource/engine/DrawRotaGraph_Shadow.hlsl", "VSmain", "vs_6_4");
		SHADERS.m_gs = D3D12App::Instance()->CompileShader("resource/engine/DrawRotaGraph_Shadow.hlsl", "GSmain", "gs_6_4");
		SHADERS.m_ps = D3D12App::Instance()->CompileShader("resource/engine/DrawRotaGraph_Shadow.hlsl", "PSmain", "ps_6_4");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("CENTER",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("EXT_RATE",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("RADIAN",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("ROTA_CENTER_UV",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("MIROR",DXGI_FORMAT_R32G32_SINT),
			InputLayoutParam("DEPTH",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("DIFFUSE",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("SPECULAR",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("LIM",DXGI_FORMAT_R32_FLOAT)
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO =
		{
			//バックバッファのフォーマット、アルファブレンド
			RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(),AlphaBlendMode_Trans),
			//エミッシブ
			RenderTargetInfo(DXGI_FORMAT_R32G32B32A32_FLOAT,AlphaBlendMode_Trans)
		};

		//パイプライン生成
		ROTA_GRAPH_PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false,true) });
	}

	StaticInit();

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(ROTA_GRAPH_PIPELINE);

	if (ROTA_GRAPH_VERTEX_BUFF.size() < (s_drawRotaGraphCount + 1))
	{
		ROTA_GRAPH_VERTEX_BUFF.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(RotaGraphVertex), 1, nullptr, ("DrawRotaGraph_Shadow -" + std::to_string(s_drawRotaGraphCount)).c_str()));
	}

	RotaGraphVertex vertex(Center, ExtRate, Radian, RotaCenterUV, Miror, SpriteDepth, Diffuse, Specular, Lim);
	ROTA_GRAPH_VERTEX_BUFF[s_drawRotaGraphCount]->Mapping(&vertex);

	auto normalMap = NormalMap ? NormalMap : s_defaultNormalMap;
	auto emissiveMap = EmissiveMap ? EmissiveMap : s_defaultEmissiveMap;

	KuroEngine::Instance().Graphics().ObjectRender(ROTA_GRAPH_VERTEX_BUFF[s_drawRotaGraphCount],
		{
			KuroEngine::Instance().GetParallelMatProjBuff(),
			s_eyePosBuff,	//視点座標
			LigManager.GetLigNumInfo(),	//アクティブ中のライト数
			LigManager.GetLigInfo(Light::DIRECTION),	//ディレクションライト
			LigManager.GetLigInfo(Light::POINT),	//ポイントライト
			LigManager.GetLigInfo(Light::SPOT),	//スポットライト
			LigManager.GetLigInfo(Light::HEMISPHERE),	//天球ライト
			Tex,normalMap,emissiveMap
		},
		{ CBV,CBV,CBV,SRV,SRV,SRV,SRV,SRV,SRV,SRV }, 0.0f, true);

	s_drawRotaGraphCount++;
}