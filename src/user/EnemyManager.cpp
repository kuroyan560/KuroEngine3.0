#include "EnemyManager.h"
#include"Importer.h"
#include"Enemy.h"
#include"KuroEngine.h"
#include"Model.h"
#include"ModelAnimator.h"
#include"Camera.h"

EnemyManager::EnemyManager()
{
/*--- パイプライン生成 ---*/
	//パイプライン設定
	static PipelineInitializeOption PIPELINE_OPTION(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//シェーダー情報
	static Shaders SHADERS;
	SHADERS.vs = D3D12App::Instance()->CompileShader("resource/user/Enemy.hlsl", "VSmain", "vs_5_0");
	SHADERS.ps = D3D12App::Instance()->CompileShader("resource/user/Enemy.hlsl", "PSmain", "ps_5_0");

	//ルートパラメータ
	static std::vector<RootParam>ROOT_PARAMETER =
	{
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,"カメラ情報バッファ"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"トランスフォームバッファ配列（構造化バッファ）"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"ボーン行列バッファ（構造化バッファ）"),
		RootParam(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,"カラーテクスチャ"),
	};

	//レンダーターゲット描画先情報
	std::vector<RenderTargetInfo>RENDER_TARGET_INFO = { RenderTargetInfo(D3D12App::Instance()->GetBackBuffFormat(), AlphaBlendMode_None) };

	//パイプライン生成
	pipeline = D3D12App::Instance()->GenerateGraphicsPipeline(PIPELINE_OPTION, SHADERS, ModelMesh::Vertex_Model::GetInputLayout(), ROOT_PARAMETER, RENDER_TARGET_INFO, { WrappedSampler(false, false) });

/*--- エネミー型オブジェクト定義 ---*/
	//サンドバッグ（何もしてこない）
	auto sandBagModel = Importer::Instance()->LoadModel("resource/user/", "sandbag.glb");
	breeds[SANDBAG] = std::make_unique<EnemyBreed>(100, sandBagModel);
}

void EnemyManager::Spawn(const ENEMY_TYPE& Type, const Transform& InitTransform)
{
	//生成、配列に追加
	auto newEnemy = std::make_shared<Enemy>(*breeds[Type], InitTransform);
	enemys[Type].push_back(newEnemy);

	//上限を超えていたらエラー
	assert(enemys[Type].size() <= MAX_NUM);

	//ワールド行列配列構造化バッファ生成
	if (!worldMatriciesBuff[Type])
	{
		std::array<Matrix, MAX_NUM>initMat;
		initMat.fill(XMMatrixIdentity());
		worldMatriciesBuff[Type] = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Matrix), MAX_NUM, initMat.data(), "Enemy's world matricies");
	}

	//ボーン行列配列構造化バッファ生成
	const int boneNum = breeds[Type]->GetModel()->skelton->bones.size();
	if (boneNum && !boneMatriciesBuff[Type])
	{
		//ボーンの最大数を超えていたらエラー
		assert(boneNum < MAX_BONE_NUM);

		std::vector<Matrix>initMat(MAX_BONE_NUM * MAX_NUM);
		std::fill(initMat.begin(), initMat.end(), XMMatrixIdentity());
		boneMatriciesBuff[Type] = D3D12App::Instance()->GenerateStructuredBuffer(sizeof(Matrix) * MAX_BONE_NUM, MAX_NUM, initMat.data(), "Enemy's bone matricies");
	}
}

void EnemyManager::Update()
{
	for (int enemyType = 0; enemyType < ENEMY_TYPE_NUM; ++enemyType)
	{
		EnemyArray& enemyArray = enemys[enemyType];

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

void EnemyManager::Draw(Camera& Cam)
{
	KuroEngine::Instance().Graphics().SetPipeline(pipeline);

	for (int enemyType = 0; enemyType < ENEMY_TYPE_NUM; ++enemyType)
	{
		//モデル取得
		auto& model = breeds[enemyType]->GetModel();

		//敵配列取得
		EnemyArray& enemyArray = enemys[enemyType];

		//ワールド行列類更新
		std::vector<Matrix>worldMatricies;
		for (auto& enemy : enemyArray)
		{
			worldMatricies.emplace_back(enemy->GetWorldMat());
		}
		worldMatriciesBuff[enemyType]->Mapping(worldMatricies.data());

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

		for (auto& mesh : model->meshes)
		{
			KuroEngine::Instance().Graphics().ObjectRender(
				mesh.mesh->vertBuff,
				mesh.mesh->idxBuff,
				{
					Cam.GetBuff(),
					worldMatriciesBuff[enemyType],
					boneMatriciesBuff[enemyType],
					mesh.material->texBuff[COLOR_TEX],
				},
				{ CBV,SRV,SRV,SRV },
				0.0f,
				false,
				enemyNum
				);
		}
	}
}
