#include "ShadowMapDevice.h"
#include"KuroEngine.h"
#include"GaussianBlur.h"
#include"Object.h"
#include"Model.h"
#include"ModelAnimator.h"

ShadowMapDevice::ShadowMapDevice() :lightCamera("LightCamera")
{
	//シャドウマップ関連
	shadowMap = D3D12App::Instance()->GenerateRenderTarget(DXGI_FORMAT_R32G32_FLOAT, Color(), { 2048,2048 }, L"ShadowMap");
	shadowMapDepth = D3D12App::Instance()->GenerateDepthStencil({ 2048,2048 });
	lightCamera.SetPos({ 0, 10, 0 });
	lightCamera.SetTarget({ 0,0,0 });
	lightCamera.SetUp({ 1,0,0 });
	lightCamera.SetAngleOfView(Angle(60));

	gaussianBlur = std::make_shared<GaussianBlur>(Vec2<int>(2048, 2048), DXGI_FORMAT_R32G32_FLOAT);
}

void ShadowMapDevice::DrawShadowMap(const std::vector<std::weak_ptr<ModelObject>>& Models)
{
	static std::shared_ptr<GraphicsPipeline>PIPELINE;
	static std::vector<std::shared_ptr<ConstantBuffer>>TRANSFORM_BUFF;

	//パイプライン未生成
	if (!PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DrawShadowMapModel.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DrawShadowMapModel.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ライトカメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"トランスフォームバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ボーン行列バッファ"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(DXGI_FORMAT_R32G32_FLOAT, AlphaBlendMode_None) };
		//パイプライン生成
		PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false) });
	}

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE);

	if (TRANSFORM_BUFF.size() < Models.size())
	{
		TRANSFORM_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, ("DrawShadowMapMode_Transform -" + std::to_string(TRANSFORM_BUFF.size())).c_str()));
	}

	shadowMap->Clear(D3D12App::Instance()->GetCmdList());
	shadowMapDepth->Clear(D3D12App::Instance()->GetCmdList());

	//シャドウマップ書き込み
	KuroEngine::Instance().Graphics().SetRenderTargets({ shadowMap }, shadowMapDepth);

	for (int i = 0; i < Models.size(); ++i)
	{
		auto obj = Models[i].lock();
		std::shared_ptr<ConstantBuffer>boneBuff;
		if (obj->animator)boneBuff = obj->animator->GetBoneMatBuff();

		TRANSFORM_BUFF[i]->Mapping(&obj->transform.GetMat());

		for (int meshIdx = 0; meshIdx < obj->model->meshes.size(); ++meshIdx)
		{
			const auto& mesh = obj->model->meshes[meshIdx];
			KuroEngine::Instance().Graphics().ObjectRender(
				mesh.mesh->vertBuff,
				mesh.mesh->idxBuff,
				{
					lightCamera.GetBuff(),
					TRANSFORM_BUFF[i],
					boneBuff,
				},
				{ CBV,CBV,CBV },
				obj->transform.GetPos().z,
				true);
		}
	}

	gaussianBlur->Register(shadowMap);

}

void ShadowMapDevice::DrawShadowReceiver(const std::vector<std::weak_ptr<ModelObject>>& Models, Camera& GameCamera, const AlphaBlendMode& BlendMode)
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
		SHADERS.vs = D3D12App::Instance()->CompileShader("resource/engine/DrawShadowFallModel.hlsl", "VSmain", "vs_5_0");
		SHADERS.ps = D3D12App::Instance()->CompileShader("resource/engine/DrawShadowFallModel.hlsl", "PSmain", "ps_5_0");

		//ルートパラメータ
		static std::vector<RootParam>ROOT_PARAMETER =
		{
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"トランスフォームバッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"カラーテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"シャドウマップ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ライトカメラ情報バッファ"),
		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), BlendMode) };

		//シャドウマップサンプリング用サンプラー
		auto shadowMapSampler = WrappedSampler(false, false);
		shadowMapSampler.sampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		shadowMapSampler.sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
		shadowMapSampler.sampler.MaxAnisotropy = 1;
		//パイプライン生成
		PIPELINE[BlendMode] = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false),shadowMapSampler });
	}

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE[BlendMode]);

	if (TRANSFORM_BUFF.size() < Models.size())
	{
		TRANSFORM_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, ("DrawShadowReceiver_Transform -" + std::to_string(TRANSFORM_BUFF.size() - 1)).c_str()));
	}


	for (int i = 0; i < Models.size(); ++i)
	{
		auto obj = Models[i].lock();

		TRANSFORM_BUFF[i]->Mapping(&obj->transform.GetMat());

		auto model = obj->model;

		for (int meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx)
		{
			const auto& mesh = model->meshes[meshIdx];
			KuroEngine::Instance().Graphics().ObjectRender(
				mesh.mesh->vertBuff,
				mesh.mesh->idxBuff,
				{
					GameCamera.GetBuff(),
					TRANSFORM_BUFF[i],
					mesh.material->texBuff[COLOR_TEX],
					gaussianBlur->GetResultTex(),
					lightCamera.GetBuff()
				},
				{ CBV,CBV,SRV,SRV,CBV },
				obj->transform.GetPos().z,
				true);
		}
	}
}

void ShadowMapDevice::SetHeight(const float& Height)
{
	auto pos = lightCamera.GetPos();
	pos.y = Height;
	lightCamera.SetPos(pos);
}

void ShadowMapDevice::SetBlurPower(const float& BlurPower)
{
	gaussianBlur->SetBlurPower(BlurPower);
}
