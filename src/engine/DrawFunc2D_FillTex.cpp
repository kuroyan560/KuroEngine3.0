#include "DrawFunc2D_FillTex.h"
#include"KuroEngine.h"

//DrawExtendGraph
int DrawFunc2D_FillTex::s_DrawExtendGraphCount = 0;

//DrawRotaGraph
int DrawFunc2D_FillTex::s_DrawRotaGraphCount = 0;

static std::vector<RootParam>ROOT_PARAMETER =
{
	RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "平行投影行列定数バッファ"),
	RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "元のテクスチャリソース"),
	RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "塗りつぶす用テクスチャリソース"),
};

void DrawFunc2D_FillTex::DrawGraph(const Vec2<float>& LeftUpPos, const std::shared_ptr<TextureBuffer>& DestTex, const std::shared_ptr<TextureBuffer>& SrcTex, 
	const float& SrcAlpha, const Vec2<bool>& Miror, const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
{
	DrawExtendGraph2D(LeftUpPos, LeftUpPos + DestTex->GetGraphSize().Float(), DestTex, SrcTex, SrcAlpha, Miror, LeftUpPaintUV, RightBottomPaintUV);
}

void DrawFunc2D_FillTex::DrawExtendGraph2D(const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos, const std::shared_ptr<TextureBuffer>& DestTex, const std::shared_ptr<TextureBuffer>& SrcTex,
	const float& SrcAlpha, const Vec2<bool>& Miror, const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
{
	if (RightBottomPaintUV.x < LeftUpPaintUV.x)assert(0);
	if (RightBottomPaintUV.y < LeftUpPaintUV.y)assert(0);

	class ExtendGraphVertex
	{
	public:
		Vec2<float>leftUpPos;
		Vec2<float>rightBottomPos;
		float alpha;
		Vec2<int> miror;
		Vec2<float>leftUpPaintUV;
		Vec2<float>rightBottomPaintUV;
		ExtendGraphVertex(const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos, const float& Alpha, const Vec2<bool>& Miror, const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
			:leftUpPos(LeftUpPos), rightBottomPos(RightBottomPos), alpha(Alpha), miror({ Miror.x ? 1 : 0 ,Miror.y ? 1 : 0 }), leftUpPaintUV(LeftUpPaintUV), rightBottomPaintUV(RightBottomPaintUV) {}
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
		SHADERS.m_vs = D3D12App::Instance()->CompileShader("resource/engine/DrawExtendGraph_FillTex.hlsl", "VSmain", "vs_6_4");
		SHADERS.m_gs = D3D12App::Instance()->CompileShader("resource/engine/DrawExtendGraph_FillTex.hlsl", "GSmain", "gs_6_4");
		SHADERS.m_ps = D3D12App::Instance()->CompileShader("resource/engine/DrawExtendGraph_FillTex.hlsl", "PSmain", "ps_6_4");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION_L_U",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("POSITION_R_B",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("SRC_ALPHA",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("MIROR",DXGI_FORMAT_R32G32_SINT),
			InputLayoutParam("PAINT_UV_L_U",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("PAINT_UV_R_B",DXGI_FORMAT_R32G32_FLOAT)
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans) };
		//パイプライン生成
		EXTEND_GRAPH_PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(true, false) });
	}

	KuroEngine::Instance()->Graphics().SetGraphicsPipeline(EXTEND_GRAPH_PIPELINE);

	if (EXTEND_GRAPH_VERTEX_BUFF.size() < (s_DrawExtendGraphCount + 1))
	{
		EXTEND_GRAPH_VERTEX_BUFF.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(ExtendGraphVertex), 1, nullptr, ("DrawExtendGraph_FillTex -" + std::to_string(s_DrawExtendGraphCount)).c_str()));
	}

	ExtendGraphVertex vertex(LeftUpPos, RightBottomPos, SrcAlpha, Miror, LeftUpPaintUV, RightBottomPaintUV);
	EXTEND_GRAPH_VERTEX_BUFF[s_DrawExtendGraphCount]->Mapping(&vertex);

	KuroEngine::Instance()->Graphics().ObjectRender(EXTEND_GRAPH_VERTEX_BUFF[s_DrawExtendGraphCount], 
		{ KuroEngine::Instance()->GetParallelMatProjBuff(),DestTex,SrcTex }, { CBV,SRV,SRV }, 0.0f, true);

	s_DrawExtendGraphCount++;
}

void DrawFunc2D_FillTex::DrawRotaGraph2D(const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian, 
	const std::shared_ptr<TextureBuffer>& DestTex, const std::shared_ptr<TextureBuffer>& SrcTex, const float& SrcAlpha,
	const Vec2<float>& RotaCenterUV, const Vec2<bool>& Miror,	const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
{
	if (RightBottomPaintUV.x < LeftUpPaintUV.x)assert(0);
	if (RightBottomPaintUV.y < LeftUpPaintUV.y)assert(0);

	//DrawRotaGraph専用頂点
	class RotaGraphVertex
	{
	public:
		Vec2<float>center;
		Vec2<float> extRate;
		float radian;
		Vec2<float>rotaCenterUV;
		float alpha;
		Vec2<int> miror;
		Vec2<float>leftUpPaintUV;
		Vec2<float>rightBottomPaintUV;
		RotaGraphVertex(const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian,
			const Vec2<float>& RotaCenterUV, const float& Alpha, const Vec2<bool>& Miror,
			const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
			:center(Center), extRate(ExtRate), radian(Radian), rotaCenterUV(RotaCenterUV), alpha(Alpha), miror({ Miror.x ? 1 : 0,Miror.y ? 1 : 0 }),
			leftUpPaintUV(LeftUpPaintUV), rightBottomPaintUV(RightBottomPaintUV) {}
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
		SHADERS.m_vs = D3D12App::Instance()->CompileShader("resource/engine/DrawRotaGraph_FillTex.hlsl", "VSmain", "vs_6_4");
		SHADERS.m_gs = D3D12App::Instance()->CompileShader("resource/engine/DrawRotaGraph_FillTex.hlsl", "GSmain", "gs_6_4");
		SHADERS.m_ps = D3D12App::Instance()->CompileShader("resource/engine/DrawRotaGraph_FillTex.hlsl", "PSmain", "ps_6_4");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("CENTER",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("EXT_RATE",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("RADIAN",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("ROTA_CENTER_UV",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("SRC_ALPHA",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("MIROR",DXGI_FORMAT_R32G32_SINT),
			InputLayoutParam("PAINT_UV_L_U",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("PAINT_UV_R_B",DXGI_FORMAT_R32G32_FLOAT)
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans) };
		//パイプライン生成
		ROTA_GRAPH_PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(true, false) });
	}

	KuroEngine::Instance()->Graphics().SetGraphicsPipeline(ROTA_GRAPH_PIPELINE);

	if (ROTA_GRAPH_VERTEX_BUFF.size() < (s_DrawRotaGraphCount + 1))
	{
		ROTA_GRAPH_VERTEX_BUFF.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(RotaGraphVertex), 1, nullptr, ("DrawRotaGraph_FillTex -" + std::to_string(s_DrawRotaGraphCount)).c_str()));
	}

	RotaGraphVertex vertex(Center, ExtRate, Radian, RotaCenterUV, SrcAlpha, Miror, LeftUpPaintUV, RightBottomPaintUV);
	ROTA_GRAPH_VERTEX_BUFF[s_DrawRotaGraphCount]->Mapping(&vertex);

	KuroEngine::Instance()->Graphics().ObjectRender(ROTA_GRAPH_VERTEX_BUFF[s_DrawRotaGraphCount],
		{ KuroEngine::Instance()->GetParallelMatProjBuff(),DestTex,SrcTex }, { CBV,SRV,SRV }, 0.0f, true);

	s_DrawRotaGraphCount++;
}
