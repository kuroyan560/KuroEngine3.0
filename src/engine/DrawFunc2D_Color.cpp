#include "DrawFunc2D_Color.h"
#include"KuroEngine.h"

//DrawExtendGraph
int DrawFunc2D_Color::DRAW_EXTEND_GRAPH_COUNT = 0;

//DrawRotaGraph
int DrawFunc2D_Color::DRAW_ROTA_GRAPH_COUNT = 0;

static std::vector<RootParam>ROOT_PARAMETER =
{
	RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "平行投影行列定数バッファ"),
	RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "テクスチャリソース"),
};

void DrawFunc2D_Color::DrawGraph(const Vec2<float>& LeftUpPos, const std::shared_ptr<TextureBuffer>& Tex, const Color& Paint, const Vec2<bool>& Miror, const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
{
	DrawExtendGraph2D(LeftUpPos, LeftUpPos + Tex->GetGraphSize().Float(), Tex, Paint, Miror, LeftUpPaintUV, RightBottomPaintUV);
}

void DrawFunc2D_Color::DrawExtendGraph2D(const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos, const std::shared_ptr<TextureBuffer>& Tex, const Color& Paint, const Vec2<bool>& Miror,
	const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
{
	if (RightBottomPaintUV.x < LeftUpPaintUV.x)assert(0);
	if (RightBottomPaintUV.y < LeftUpPaintUV.y)assert(0);

	class ExtendGraphVertex
	{
	public:
		Vec2<float>leftUpPos;
		Vec2<float>rightBottomPos;
		Color paintColor;
		Vec2<int> miror;
		Vec2<float>leftUpPaintUV;
		Vec2<float>rightBottomPaintUV;
		ExtendGraphVertex(const Vec2<float>& LeftUpPos, const Vec2<float>& RightBottomPos, const Color& Paint, const Vec2<bool>& Miror, const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
			:leftUpPos(LeftUpPos), rightBottomPos(RightBottomPos), paintColor(Paint), miror({ Miror.x ? 1 : 0 ,Miror.y ? 1 : 0 }), leftUpPaintUV(LeftUpPaintUV), rightBottomPaintUV(RightBottomPaintUV) {}
	};

	static std::shared_ptr<GraphicsPipeline>EXTEND_GRAPH_PIPELINE;
	static std::vector<std::shared_ptr<VertexBuffer>>EXTEND_GRAPH_VERTEX_BUFF;

	//パイプライン未生成
	if (!EXTEND_GRAPH_PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		PIPELINE_OPTION.depthTest = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/HLSL/Engine/DrawExtendGraph_Color.hlsl", "VSmain", "vs_5_0");
		SHADERS.gs = D3D12App::Instance()->CompileShader("resource/HLSL/Engine/DrawExtendGraph_Color.hlsl", "GSmain", "gs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/HLSL/Engine/DrawExtendGraph_Color.hlsl", "PSmain", "ps_5_0");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("POSITION_L_U",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("POSITION_R_B",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("PAINT_COLOR",DXGI_FORMAT_R32G32B32A32_FLOAT),
			InputLayoutParam("MIROR",DXGI_FORMAT_R32G32_SINT),
			InputLayoutParam("PAINT_UV_L_U",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("PAINT_UV_R_B",DXGI_FORMAT_R32G32_FLOAT)
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans) };
		//パイプライン生成
		EXTEND_GRAPH_PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(true, false) });
	}

	KuroEngine::Instance().Graphics().SetPipeline(EXTEND_GRAPH_PIPELINE);

	if (EXTEND_GRAPH_VERTEX_BUFF.size() < (DRAW_EXTEND_GRAPH_COUNT + 1))
	{
		EXTEND_GRAPH_VERTEX_BUFF.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(ExtendGraphVertex), 1, nullptr, ("DrawExtendGraph_Color -" + std::to_string(DRAW_EXTEND_GRAPH_COUNT)).c_str()));
	}

	ExtendGraphVertex vertex(LeftUpPos, RightBottomPos, Paint, Miror, LeftUpPaintUV, RightBottomPaintUV);
	EXTEND_GRAPH_VERTEX_BUFF[DRAW_EXTEND_GRAPH_COUNT]->Mapping(&vertex);

	KuroEngine::Instance().Graphics().ObjectRender(EXTEND_GRAPH_VERTEX_BUFF[DRAW_EXTEND_GRAPH_COUNT], { KuroEngine::Instance().GetParallelMatProjBuff(),Tex }, { CBV,SRV }, 0.0f, true);

	DRAW_EXTEND_GRAPH_COUNT++;
}

void DrawFunc2D_Color::DrawRotaGraph2D(const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian, const std::shared_ptr<TextureBuffer>& Tex, const Color& Paint, const Vec2<float>& RotaCenterUV, const Vec2<bool>& Miror,
	const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
{
	static std::shared_ptr<GraphicsPipeline>ROTA_GRAPH_PIPELINE;
	static std::vector<std::shared_ptr<VertexBuffer>>ROTA_GRAPH_VERTEX_BUFF;

	if (RightBottomPaintUV.x < LeftUpPaintUV.x)assert(0);
	if (RightBottomPaintUV.y < LeftUpPaintUV.y)assert(0);

	//DrawRotaGraph専用頂点
	class RotaGraphVertex
	{
	public:
		Vec2<float>center;
		Vec2<float> extRate;
		float radian;
		Color paintColor;
		Vec2<float>rotaCenterUV;
		Vec2<int> miror;
		Vec2<float>leftUpPaintUV;
		Vec2<float>rightBottomPaintUV;
		RotaGraphVertex(const Vec2<float>& Center, const Vec2<float>& ExtRate, const float& Radian,
			const Color& Paint, const Vec2<float>& RotaCenterUV, const Vec2<bool>& Miror,
			const Vec2<float>& LeftUpPaintUV, const Vec2<float>& RightBottomPaintUV)
			:center(Center), extRate(ExtRate), radian(Radian), rotaCenterUV(RotaCenterUV), paintColor(Paint), miror({ Miror.x ? 1 : 0,Miror.y ? 1 : 0 }),
			leftUpPaintUV(LeftUpPaintUV), rightBottomPaintUV(RightBottomPaintUV) {}
	};

	//パイプライン未生成
	if (!ROTA_GRAPH_PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		PIPELINE_OPTION.depthTest = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/HLSL/Engine/DrawRotaGraph_Color.hlsl", "VSmain", "vs_5_0");
		SHADERS.gs = D3D12App::Instance()->CompileShader("resource/HLSL/Engine/DrawRotaGraph_Color.hlsl", "GSmain", "gs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/HLSL/Engine/DrawRotaGraph_Color.hlsl", "PSmain", "ps_5_0");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("CENTER",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("EXT_RATE",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("RADIAN",DXGI_FORMAT_R32_FLOAT),
			InputLayoutParam("PAINT_COLOR",DXGI_FORMAT_R32G32B32A32_FLOAT),
			InputLayoutParam("ROTA_CENTER_UV",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("MIROR",DXGI_FORMAT_R32G32_SINT),
			InputLayoutParam("PAINT_UV_L_U",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("PAINT_UV_R_B",DXGI_FORMAT_R32G32_FLOAT)
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans) };
		//パイプライン生成
		ROTA_GRAPH_PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(true, false) });
	}

	KuroEngine::Instance().Graphics().SetPipeline(ROTA_GRAPH_PIPELINE);

	if (ROTA_GRAPH_VERTEX_BUFF.size() < (DRAW_ROTA_GRAPH_COUNT + 1))
	{
		ROTA_GRAPH_VERTEX_BUFF.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(RotaGraphVertex), 1, nullptr, ("DrawRotaGraph_Color -" + std::to_string(DRAW_ROTA_GRAPH_COUNT)).c_str()));
	}

	RotaGraphVertex vertex(Center, ExtRate, Radian, Paint, RotaCenterUV, Miror, LeftUpPaintUV, RightBottomPaintUV);
	ROTA_GRAPH_VERTEX_BUFF[DRAW_ROTA_GRAPH_COUNT]->Mapping(&vertex);

	KuroEngine::Instance().Graphics().ObjectRender(ROTA_GRAPH_VERTEX_BUFF[DRAW_ROTA_GRAPH_COUNT], { KuroEngine::Instance().GetParallelMatProjBuff(),Tex }, { CBV,SRV }, 0.0f, true);

	DRAW_ROTA_GRAPH_COUNT++;
}

void DrawFunc2D_Color::DrawLine2DGraph(const Vec2<float>& FromPos, const Vec2<float>& ToPos, const std::shared_ptr<TextureBuffer>& Tex, const Color& Paint, const int& Thickness, const Vec2<bool>& Mirror)
{
	float distance = FromPos.Distance(ToPos);
	Vec2<float> vec = (ToPos - FromPos).GetNormal();

	auto graphSize = Tex->GetGraphSize().Float();
	Vec2<float>expRate = { distance / graphSize.x,Thickness / graphSize.y };
	Vec2<float>centerPos = FromPos + vec * distance / 2;

	DrawRotaGraph2D(centerPos, expRate, KuroMath::GetAngle(vec), Tex, Paint, { 0.5f,0.5f }, Mirror);
}
