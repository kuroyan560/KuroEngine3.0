#include "BasicDraw.h"
#include"Model.h"
#include"KuroEngine.h"
#include"Object.h"
#include"ModelAnimator.h"
#include"CubeMap.h"
#include"Camera.h"
#include"LightManager.h"

int BasicDraw::s_drawCount = 0;

void BasicDraw::Draw(LightManager& LigManager, const std::weak_ptr<ModelObject> ModelObject, Camera& Cam, std::shared_ptr<CubeMap> AttachCubeMap)
{
	auto obj = ModelObject.lock();

	static std::shared_ptr<GraphicsPipeline>PIPELINE;
	static std::vector<std::shared_ptr<ConstantBuffer>>TRANSFORM_BUFF;

	//パイプライン未生成
	if (!PIPELINE)
	{
		//パイプライン設定
		static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//シェーダー情報
		static Shaders SHADERS;
		SHADERS.m_vs = D3D12App::Instance()->CompileShader("resource/user/shaders/BasicShader.hlsl", "VSmain", "vs_6_4");
		SHADERS.m_ps = D3D12App::Instance()->CompileShader("resource/user/shaders/BasicShader.hlsl", "PSmain", "ps_6_4");

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
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"ボーン行列バッファ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ベースカラーテクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"メタルネステクスチャ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ノーマルマップ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"粗さ"),
			RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"マテリアル基本情報バッファ"),

		};

		//レンダーターゲット描画先情報
		std::vector<RenderTargetInfo>RENDER_TARGET_INFO = 
		{
			RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans),	//通常描画
			RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_Trans),	//エミッシブマップ
			RenderTargetInfo(DXGI_FORMAT_R32_FLOAT, AlphaBlendMode_None),	//深度マップ
		};
		//パイプライン生成
		PIPELINE = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(true, true) });
	}

	KuroEngine::Instance().Graphics().SetGraphicsPipeline(PIPELINE);

	if (TRANSFORM_BUFF.size() < (s_drawCount + 1))
	{
		TRANSFORM_BUFF.emplace_back(D3D12App::Instance()->GenerateConstantBuffer(sizeof(Matrix), 1, nullptr, ("BasicDraw - Transform -" + std::to_string(s_drawCount)).c_str()));
	}

	TRANSFORM_BUFF[s_drawCount]->Mapping(&obj->m_transform.GetMat());


	//ボーン行列バッファ取得（アニメーターがnullptrなら空）
	auto model = obj->m_model;
	std::shared_ptr<ConstantBuffer>boneBuff;
	if (obj->m_animator)boneBuff = obj->m_animator->GetBoneMatBuff();

	//キューブマップ（nullptrならデフォルトの静的キューブマップ）
	std::shared_ptr<CubeMap>cubeMap = StaticallyCubeMap::GetDefaultCubeMap();
	if (AttachCubeMap)cubeMap = AttachCubeMap;

	for (int meshIdx = 0; meshIdx < model->m_meshes.size(); ++meshIdx)
	{
		const auto& mesh = model->m_meshes[meshIdx];
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
				cubeMap->GetCubeMapTex(),
				TRANSFORM_BUFF[s_drawCount],
				boneBuff,
				mesh.material->texBuff[COLOR_TEX],
				mesh.material->texBuff[METALNESS_TEX],
				mesh.material->texBuff[NORMAL_TEX],
				mesh.material->texBuff[ROUGHNESS_TEX],
				mesh.material->buff,
			},
			{ CBV,CBV,SRV,SRV,SRV,SRV,SRV,CBV,CBV,SRV,SRV,SRV,SRV,CBV },
			obj->m_transform.GetPos().z,
			true);
	}

	s_drawCount++;
}
