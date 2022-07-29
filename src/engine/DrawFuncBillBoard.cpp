#include "DrawFuncBillBoard.h"
#include"KuroEngine.h"
#include"Camera.h"

//DrawBox
int DrawFuncBillBoard::s_drawBoxCount;

void DrawFuncBillBoard::Box(Camera& Cam, const Vec3<float>& Pos, const Vec2<float>& Size, const Color& BoxColor, const AlphaBlendMode& BlendMode)
{
	static std::shared_ptr<GraphicsPipeline>s_pipeline[AlphaBlendModeNum];
	static std::vector<std::shared_ptr<VertexBuffer>>s_boxVertBuff;

	//DrawBox専用頂点
	class BoxVertex
	{
	public:
		Vec3<float>m_pos;
		Vec2<float>m_size;
		Color m_col;
		BoxVertex(const Vec3<float>& Pos, const Vec2<float>& Size, const Color& Color)
			:m_pos(Pos), m_size(Size), m_col(Color) {}
	};

	//パイプライン未生成
	if (!s_pipeline[BlendMode])
	{
		//パイプライン設定
		static PipelineInitializeOption s_pipelineOption(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		s_pipelineOption.m_calling = false;

		//シェーダー情報
		static Shaders s_shaders;
		s_shaders.m_vs = D3D12App::Instance()->CompileShader("resource/engine/DrawBoxBillBoard.hlsl", "VSmain", "vs_6_4");
		s_shaders.m_gs = D3D12App::Instance()->CompileShader("resource/engine/DrawBoxBillBoard.hlsl", "GSmain", "gs_6_4");
		s_shaders.m_ps = D3D12App::Instance()->CompileShader("resource/engine/DrawBoxBillBoard.hlsl", "PSmain", "ps_6_4");

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
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>s_renderTargetInfo = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };
		//パイプライン生成
		s_pipeline[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(s_pipelineOption, s_shaders, s_inputLayOut, s_rootParams, s_renderTargetInfo, { WrappedSampler(false, false) });
	}

	KuroEngine::Instance()->Graphics().SetGraphicsPipeline(s_pipeline[BlendMode]);

	if (s_boxVertBuff.size() < (s_drawBoxCount + 1))
	{
		s_boxVertBuff.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(BoxVertex), 1, nullptr, ("DrawBoxBillBoard -" + std::to_string(s_drawBoxCount)).c_str()));
	}

	BoxVertex vertex(Pos, Size, BoxColor);
	s_boxVertBuff[s_drawBoxCount]->Mapping(&vertex);

	KuroEngine::Instance()->Graphics().ObjectRender(
		s_boxVertBuff[s_drawBoxCount],
		{
			Cam.GetBuff(),
		},
		{ CBV },
		Pos.z,
		true);

	s_drawBoxCount++;
}
