#include "DrawFuncBillBoard.h"
#include"KuroEngine.h"
#include"Camera.h"

//DrawBox
int DrawFuncBillBoard::s_drawBoxCount = 0;
//DrawGraph
int DrawFuncBillBoard::s_drawGraphCount = 0;

std::array<std::shared_ptr<GraphicsPipeline>, AlphaBlendModeNum>DrawFuncBillBoard::s_pipeline;


void DrawFuncBillBoard::GeneratePipeline(const AlphaBlendMode& BlendMode)
{
	//パイプライン未生成
	if (!s_pipeline[BlendMode])
	{
		//パイプライン設定
		static PipelineInitializeOption s_pipelineOption(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		s_pipelineOption.m_calling = false;

		//シェーダー情報
		static Shaders s_shaders;
		s_shaders.m_vs = D3D12App::Instance()->CompileShader("resource/engine/DrawBillBoard.hlsl", "VSmain", "vs_6_4");
		s_shaders.m_gs = D3D12App::Instance()->CompileShader("resource/engine/DrawBillBoard.hlsl", "GSmain", "gs_6_4");
		s_shaders.m_ps = D3D12App::Instance()->CompileShader("resource/engine/DrawBillBoard.hlsl", "PSmain", "ps_6_4");

		//インプットレイアウト
		static std::vector<InputLayoutParam>s_inputLayOut =
		{
			InputLayoutParam("POS",DXGI_FORMAT_R32G32B32_FLOAT),
			InputLayoutParam("SIZE",DXGI_FORMAT_R32G32_FLOAT),
			InputLayoutParam("COLOR",DXGI_FORMAT_R32G32B32A32_FLOAT),
		};

		//ルートパラメータ
		static std::vector<RootParam>s_rootParams =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"テクスチャ"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>s_renderTargetInfo = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };
		//パイプライン生成
		s_pipeline[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(s_pipelineOption, s_shaders, s_inputLayOut, s_rootParams, s_renderTargetInfo, { WrappedSampler(false, false) });
	}
}

void DrawFuncBillBoard::Box(Camera& Cam, const Vec3<float>& Pos, const Vec2<float>& Size, const Color& BoxColor, const AlphaBlendMode& BlendMode)
{
	static std::vector<std::shared_ptr<VertexBuffer>>s_boxVertBuff;
	static std::shared_ptr<TextureBuffer>s_defaultTex = D3D12App::Instance()->GenerateTextureBuffer(Color());

	if (!s_pipeline[BlendMode])GeneratePipeline(BlendMode);

	KuroEngine::Instance()->Graphics().SetGraphicsPipeline(s_pipeline[BlendMode]);

	if (s_boxVertBuff.size() < (s_drawBoxCount + 1))
	{
		s_boxVertBuff.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vertex), 1, nullptr, ("DrawBoxBillBoard -" + std::to_string(s_drawBoxCount)).c_str()));
	}

	Vertex vertex(Pos, Size, BoxColor);
	s_boxVertBuff[s_drawBoxCount]->Mapping(&vertex);

	KuroEngine::Instance()->Graphics().ObjectRender(
		s_boxVertBuff[s_drawBoxCount],
		{
			Cam.GetBuff(),
			s_defaultTex
		},
		{ CBV,SRV },
		Pos.z,
		true);

	s_drawBoxCount++;
}

void DrawFuncBillBoard::Graph(Camera& Cam, const Vec3<float>& Pos, const Vec2<float>& Size, std::shared_ptr<TextureBuffer> Tex, const AlphaBlendMode& BlendMode)
{
	static std::vector<std::shared_ptr<VertexBuffer>>s_graphVertBuff;

	if (!s_pipeline[BlendMode])GeneratePipeline(BlendMode);

	KuroEngine::Instance()->Graphics().SetGraphicsPipeline(s_pipeline[BlendMode]);

	if (s_graphVertBuff.size() < (s_drawGraphCount + 1))
	{
		s_graphVertBuff.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(Vertex), 1, nullptr, ("DrawGraphBillBoard -" + std::to_string(s_drawGraphCount)).c_str()));
	}

	Vertex vertex(Pos, Size, Color());
	s_graphVertBuff[s_drawGraphCount]->Mapping(&vertex);

	KuroEngine::Instance()->Graphics().ObjectRender(
		s_graphVertBuff[s_drawGraphCount],
		{
			Cam.GetBuff(),
			Tex
		},
		{ CBV,SRV },
		Pos.z,
		true);

	s_drawGraphCount++;
}