#include "EnemyManager.h"
#include"Importer.h"
#include"Enemy.h"
#include"KuroEngine.h"
#include"Model.h"
#include"ModelAnimator.h"
#include"Camera.h"
#include"CubeMap.h"

EnemyManager::EnemyManager()
{
/*--- パイプライン生成 ---*/
	//パイプライン設定
	static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//シェーダー情報
	static Shaders SHADERS;
	SHADERS.m_vs = D3D12App::Instance()->CompileShader("resource/user/Enemy.hlsl", "VSmain", "vs_5_0");
	SHADERS.m_ps = D3D12App::Instance()->CompileShader("resource/user/Enemy.hlsl", "PSmain", "ps_5_0");

	//ルートパラメータ
	static std::vector<RootParam>ROOT_PARAMETER =
	{
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"マテリアル基本情報バッファ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"トランスフォームバッファ配列（構造化バッファ）"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ボーン行列バッファ（構造化バッファ）"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, "キューブマップ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ベースカラーテクスチャ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"メタルネステクスチャ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"粗さ"),
	};

	//レンダーターゲット描画先情報
	std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_None) };

	//パイプライン生成
	m_pipeline = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false) });

/*--- エネミー型オブジェクト定義 ---*/
	//サンドバッグ（何もしてこない）
	auto sandBagModel = Importer::Instance()->LoadModel("resource/user/", "sandbag.glb");
	m_breeds[SANDBAG] = std::make_unique<EnemyBreed>(100, sandBagModel);
}

void EnemyManager::Spawn(const ENEMY_TYPE& Type, const Transform& InitTransform)
{
	//生成、配列に追加
	auto newEnemy = std::make_shared<Enemy>(*m_breeds[Type], InitTransform);
	m_enemys[Type].push_back(newEnemy);

	//上限を超えていたらエラー
	assert(m_enemys[Type].size() <= s_maxNum);

	//ワールド行列配列構造化バッファ生成
	if (!m_worldMatriciesBuff[Type])
	{
		std::array<Matrix, s_maxNum>initMat;
		initMat.fill(XMMatrixIdentity());
		m_worldMatriciesBuff[Type] = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Matrix), s_maxNum, initMat.data(), "Enemy's world matricies");
	}

	//ボーン行列配列構造化バッファ生成
	const int boneNum = m_breeds[Type]->GetModel()->m_skelton->bones.size();
	if (boneNum && !m_boneMatriciesBuff[Type])
	{
		//ボーンの最大数を超えていたらエラー
		assert(boneNum < s_maxBoneNum);

		std::vector<Matrix>initMat(s_maxBoneNum * s_maxNum);
		std::fill(initMat.begin(), initMat.end(), XMMatrixIdentity());
		m_boneMatriciesBuff[Type] = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Matrix) * s_maxBoneNum, s_maxNum, initMat.data(), "Enemy's bone matricies");
	}
}

void EnemyManager::Update()
{
	for (int enemyType = 0; enemyType < ENEMY_TYPE_NUM; ++enemyType)
	{
		EnemyArray& enemyArray = m_enemys[enemyType];

		//存在しないならスルー
		if (enemyArray.empty())continue;

		//更新
		for (auto& enemy : enemyArray)
		{
			enemy->Update();
		}

		//死んでたら削除
		std::remove_if(enemyArray.begin(), enemyArray.end(), [](const std::shared_ptr<Enemy>& enemy)
			{
				return !enemy->IsAlive();
			});
	}
}

void EnemyManager::Draw(Camera& Cam, std::shared_ptr<CubeMap>AttachCubeMap)
{
	KuroEngine::Instance().Graphics().SetGraphicsPipeline(m_pipeline);

	for (int enemyType = 0; enemyType < ENEMY_TYPE_NUM; ++enemyType)
	{
		//敵配列取得
		EnemyArray& enemyArray = m_enemys[enemyType];

		//存在しないならスルー
		if (enemyArray.empty())continue;

		//モデル取得
		auto& model = m_breeds[enemyType]->GetModel();


		//ワールド行列類更新
		static std::vector<Matrix>WORLD_MATRICIES(s_maxNum);	//行列配列は使いまわし
		std::fill(WORLD_MATRICIES.begin(), WORLD_MATRICIES.end(), XMMatrixIdentity());
		for (int enemyIdx = 0; enemyIdx < enemyArray.size(); ++enemyIdx)
		{
			WORLD_MATRICIES[enemyIdx] = enemyArray[enemyIdx]->GetWorldMat();
		}
		m_worldMatriciesBuff[enemyType]->Mapping(WORLD_MATRICIES.data());

		//ボーン行列更新
		/*
		if (boneMatriciesBuff[enemyType])	//ボーン関連情報を持っているか
		{
			const int boneNum = model->skelton->bones.size() - 1;
			std::vector<Matrix>boneMatricies;
			for (auto& enemy : enemyArray)
			{
				const auto& boneAnimMat = enemy->GetAnimator()->GetBoneMatricies();
				for (auto& mat : boneAnimMat)boneMatricies.emplace_back(mat);
			}
			boneMatriciesBuff[enemyType]->Mapping(boneMatricies.data());
		}
		*/

		const int enemyNum = enemyArray.size();	//インスタンス数取得

		for (auto& mesh : model->m_meshes)
		{
			KuroEngine::Instance().Graphics().ObjectRender(
				mesh.mesh->vertBuff,
				mesh.mesh->idxBuff,
				{
					Cam.GetBuff(),
					mesh.material->buff,
					m_worldMatriciesBuff[enemyType],
					m_boneMatriciesBuff[enemyType],
					AttachCubeMap->GetCubeMapTex(),
					mesh.material->texBuff[COLOR_TEX],
					mesh.material->texBuff[METALNESS_TEX],
					mesh.material->texBuff[ROUGHNESS_TEX],
				},
				{ CBV,CBV,SRV,SRV,SRV,SRV,SRV,SRV },
				0.0f,
				false,
				enemyNum
				);
		}
	}
}
