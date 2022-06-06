#include "DrawFunc3D.h"
#include"KuroEngine.h"
#include"Model.h"
#include"LightManager.h"
#include"CubeMap.h"
#include"ModelAnimator.h"

//DrawLine
int DrawFunc3D::DRAW_LINE_COUNT = 0;
//DrawNonShadingModel
int DrawFunc3D::DRAW_NON_SHADING_COUNT = 0;
//DrawADSShadingModel
int DrawFunc3D::DRAW_ADS_SHADING_COUNT = 0;
//DrawPBRShadingModel
int DrawFunc3D::DRAW_PBR_SHADING_COUNT = 0;
//DrawToonModel
int DrawFunc3D::DRAW_TOON_COUNT = 0;
//DrawShadowFallModel
int DrawFunc3D::DRAW_SHADOW_FALL_COUNT = 0;


void DrawFunc3D::DrawLine(Camera& Cam, const Vec3<float>& From, const Vec3<float>& To, const Color& LineColor, const float& Thickness, const AlphaBlendMode& BlendMode)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE[AlphaBlendModeNum];
	static std::vector<std::shared_ptr<VertexBuffer>>LINE_VERTEX_BUFF;

	//DrawLine専用頂点
	class LineVertex
	{
	public:
		Vec3<float>fromPos;
		Vec3<float>toPos;
		Color color;
		float thickness;
		LineVertex(const Vec3<float>& FromPos, const Vec3<float>& ToPos, const Color& Color, const float& Thickness)
			:fromPos(FromPos), toPos(ToPos), color(Color), thickness(Thickness) {}
	};

	//パイプライン未生成
	if (!PIPELINE[BlendMode])
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		PIPELINE_OPTION.calling = false;

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DrawLine3D.hlsl", "VSmain", "vs_5_0");
		SHADERS.gs = D3D12App::Instance()->CompileShader("resource/engine/DrawLine3D.hlsl", "GSmain", "gs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DrawLine3D.hlsl", "PSmain", "ps_5_0");

		//インプットレイアウト
		static std::vector<InputLayoutParam>INPUT_LAYOUT =
		{
			InputLayoutParam("FROM_POS",DXGI_FORMAT_R32G32B32_FLOAT),
			InputLayoutParam("TO_POS",DXGI_FORMAT_R32G32B32_FLOAT),
			InputLayoutParam("COLOR",DXGI_FORMAT_R32G32B32A32_FLOAT),
			InputLayoutParam("THICKNESS",DXGI_FORMAT_R32_FLOAT),
		};

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };
		//パイプライン生成
		PIPELINE[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, INPUT_LAYOUT, ROOT_PARAMETER, RENDER_TARGET_INFO, {WrappedSampler(false, false)});
	}

	KuroEngine::Instance().Graphics().SetPipeline(PIPELINE[BlendMode]);

	if (LINE_VERTEX_BUFF.size() < (DRAW_LINE_COUNT + 1))
	{
		LINE_VERTEX_BUFF.emplace_back(D3D12App::Instance()->GenerateVertexBuffer(sizeof(LineVertex), 1, nullptr, ("DrawLine3D -" + std::to_string(DRAW_LINE_COUNT)).c_str()));
	}

	LineVertex vertex(From, To, LineColor, Thickness);
	LINE_VERTEX_BUFF[DRAW_LINE_COUNT]->Mapping(&vertex);
	Vec3<float>center = From.GetCenter(To);

	KuroEngine::Instance().Graphics().ObjectRender(
		LINE_VERTEX_BUFF[DRAW_LINE_COUNT],
		{
			Cam.GetBuff(),
		},
		{ CBV },
		center.z,
		true);

	DRAW_LINE_COUNT++;
}

void DrawFunc3D::DrawNonShadingModel(const std::weak_ptr<Model> Model, Transform& Transform, Camera& Cam, const AlphaBlendMode& BlendMode)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE[AlphaBlendModeNum];
	static std::vector<std::shared_ptr<ConstantBuffer>>TRANSFORM_BUFF;

	//パイプライン未生成
	if (!PIPELINE[BlendMode])
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DrawNonShadingModel.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DrawNonShadingModel.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"トランスフォームバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"カラーテクスチャ"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };
		//パイプライン生成
		PIPELINE[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, {WrappedSampler(false, false)});
	}

	KuroEngine::Instance().Graphics().SetPipeline(PIPELINE[BlendMode]);

	if (TRANSFORM_BUFF.size() < (DRAW_NON_SHADING_COUNT + 1))
	{
		TRANSFORM_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, ("DrawShadingModel_Transform -" + std::to_string(DRAW_NON_SHADING_COUNT)).c_str()));
	}

	TRANSFORM_BUFF[DRAW_NON_SHADING_COUNT]->Mapping(&Transform.GetMat());

	auto model = Model.lock();

	for (int meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx)
	{
		const auto& mesh = model->meshes[meshIdx];
		KuroEngine::Instance().Graphics().ObjectRender(
			mesh.mesh->vertBuff,
			mesh.mesh->idxBuff,
			{
				Cam.GetBuff(),
				TRANSFORM_BUFF[DRAW_NON_SHADING_COUNT],
				mesh.material->texBuff[COLOR_TEX],
			},
			{ CBV,CBV,SRV },
			Transform.GetPos().z,
			true);
	}

	DRAW_NON_SHADING_COUNT++;
}

void DrawFunc3D::DrawADSShadingModel(LightManager& LigManager, const std::weak_ptr<Model> Model, Transform& Transform, Camera& Cam, ModelAnimator* Animator, const AlphaBlendMode& BlendMode)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE[AlphaBlendModeNum];
	static std::vector<std::shared_ptr<ConstantBuffer>>TRANSFORM_BUFF;

	//パイプライン未生成
	if (!PIPELINE[BlendMode])
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DrawADSShadingModel.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DrawADSShadingModel.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "アクティブ中のライト数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ディレクションライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ポイントライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "スポットライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "天球ライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"トランスフォームバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ボーン行列バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"カラーテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ノーマルマップ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"マテリアル基本情報バッファ"),

		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };
		//パイプライン生成
		PIPELINE[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, {WrappedSampler(false, false)});
	}

	KuroEngine::Instance().Graphics().SetPipeline(PIPELINE[BlendMode]);

	if (TRANSFORM_BUFF.size() < (DRAW_ADS_SHADING_COUNT + 1))
	{
		TRANSFORM_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, ("DrawADSShadingModel_Transform -" + std::to_string(DRAW_ADS_SHADING_COUNT)).c_str()));
	}

	TRANSFORM_BUFF[DRAW_ADS_SHADING_COUNT]->Mapping(&Transform.GetMat());

	auto model = Model.lock();
	std::shared_ptr<ConstantBuffer>boneBuff;
	if(Animator)boneBuff = Animator->

	for (int meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx)
	{
		const auto& mesh = model->meshes[meshIdx];
		KuroEngine::Instance().Graphics().ObjectRender(
			mesh.mesh->vertBuff,
			mesh.mesh->idxBuff,
			{
				Cam.GetBuff(),
				LigManager.GetLigNumInfo(),
				LigManager.GetLigInfo(Light::DIRECTION),
				LigManager.GetLigInfo(Light::POINT),
				LigManager.GetLigInfo(Light::SPOT),
				LigManager.GetLigInfo(Light::HEMISPHERE),
				TRANSFORM_BUFF[DRAW_ADS_SHADING_COUNT],
				mesh.material->texBuff[COLOR_TEX],
				mesh.material->texBuff[NORMAL_TEX],
				mesh.material->buff,
			},
			{ CBV,CBV,SRV,SRV,SRV,SRV,CBV,SRV,SRV,CBV },
			Transform.GetPos().z,
			true);
	}

	DRAW_ADS_SHADING_COUNT++;
}

void DrawFunc3D::DrawPBRShadingModel(LightManager& LigManager, const std::weak_ptr<Model> Model, Transform& Transform, Camera& Cam, const std::shared_ptr<CubeMap>CubeMap, const AlphaBlendMode& BlendMode)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE[AlphaBlendModeNum];
	static std::vector<std::shared_ptr<ConstantBuffer>>TRANSFORM_BUFF;

	//パイプライン未生成
	if (!PIPELINE[BlendMode])
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DrawPBRShadingModel.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DrawPBRShadingModel.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "アクティブ中のライト数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ディレクションライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ポイントライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "スポットライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "天球ライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "キューブマップ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"トランスフォームバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ベースカラーテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"メタルネステクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ノーマルマップ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"粗さ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"マテリアル基本情報バッファ"),

		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };
		//パイプライン生成
		PIPELINE[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, {WrappedSampler(false, false)});
	}

	KuroEngine::Instance().Graphics().SetPipeline(PIPELINE[BlendMode]);

	if (TRANSFORM_BUFF.size() < (DRAW_PBR_SHADING_COUNT + 1))
	{
		TRANSFORM_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, ("DrawPBRShadingModel_Transform -" + std::to_string(DRAW_PBR_SHADING_COUNT)).c_str()));
	}

	TRANSFORM_BUFF[DRAW_PBR_SHADING_COUNT]->Mapping(&Transform.GetMat());

	auto model = Model.lock();

	for (int meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx)
	{
		const auto& mesh = model->meshes[meshIdx];
		KuroEngine::Instance().Graphics().ObjectRender(
			mesh.mesh->vertBuff,
			mesh.mesh->idxBuff,
			{
				Cam.GetBuff(),
				LigManager.GetLigNumInfo(),
				LigManager.GetLigInfo(Light::DIRECTION),
				LigManager.GetLigInfo(Light::POINT),
				LigManager.GetLigInfo(Light::SPOT),
				LigManager.GetLigInfo(Light::HEMISPHERE),
				CubeMap->GetCubeMapTex(),
				TRANSFORM_BUFF[DRAW_PBR_SHADING_COUNT],
				mesh.material->texBuff[COLOR_TEX],
				mesh.material->texBuff[METALNESS_TEX],
				mesh.material->texBuff[NORMAL_TEX],
				mesh.material->texBuff[ROUGHNESS_TEX],
				mesh.material->buff,
			},
			{ CBV,CBV,SRV,SRV,SRV,SRV,SRV,CBV,SRV,SRV,SRV,SRV,CBV },
			Transform.GetPos().z,
			true);
	}

	DRAW_PBR_SHADING_COUNT++;
}

void DrawFunc3D::DrawToonModel(const std::weak_ptr<TextureBuffer> ToonTex, LightManager& LigManager, const std::weak_ptr<Model> Model, Transform& Transform, Camera& Cam, const AlphaBlendMode& BlendMode)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE[AlphaBlendModeNum];
	static std::vector<std::shared_ptr<ConstantBuffer>>TRANSFORM_BUFF;

	//パイプライン未生成
	if (!PIPELINE[BlendMode])
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DrawToonModel.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DrawToonModel.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, "アクティブ中のライト数バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ディレクションライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "ポイントライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "スポットライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "天球ライト情報 (構造化バッファ)"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"トランスフォームバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"カラーテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ノーマルマップ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "トゥーンテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"マテリアル基本情報バッファ"),

		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };
		//パイプライン生成
		PIPELINE[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, {WrappedSampler(false, false)});
	}

	KuroEngine::Instance().Graphics().SetPipeline(PIPELINE[BlendMode]);

	if (TRANSFORM_BUFF.size() < (DRAW_TOON_COUNT + 1))
	{
		TRANSFORM_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, ("DrawShadingModel_Transform -" + std::to_string(DRAW_TOON_COUNT)).c_str()));
	}

	TRANSFORM_BUFF[DRAW_TOON_COUNT]->Mapping(&Transform.GetMat());

	auto model = Model.lock();

	for (int meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx)
	{
		const auto& mesh = model->meshes[meshIdx];
		KuroEngine::Instance().Graphics().ObjectRender(
			mesh.mesh->vertBuff,
			mesh.mesh->idxBuff,
			{
				Cam.GetBuff(),
				LigManager.GetLigNumInfo(),
				LigManager.GetLigInfo(Light::DIRECTION),
				LigManager.GetLigInfo(Light::POINT),
				LigManager.GetLigInfo(Light::SPOT),
				LigManager.GetLigInfo(Light::HEMISPHERE),
				TRANSFORM_BUFF[DRAW_TOON_COUNT],
				mesh.material->texBuff[COLOR_TEX],
				mesh.material->texBuff[NORMAL_TEX],
				ToonTex.lock(),
				mesh.material->buff,
			},
			{ CBV,CBV,SRV,SRV,SRV,SRV,CBV,SRV,SRV,SRV,CBV },
			Transform.GetPos().z,
			true);
	}

	DRAW_TOON_COUNT++;
}