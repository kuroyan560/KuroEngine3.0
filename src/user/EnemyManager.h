#pragma once
#include<vector>
#include<array>
#include<memory>
#include"Singleton.h"
#include"EnemyBreed.h"
class Enemy;
#include"Transform.h"
class GraphicsPipeline;
class StructuredBuffer;
class Camera;
class CubeMap;

//エネミー種別
const enum ENEMY_TYPE { SANDBAG, ENEMY_TYPE_NUM };

class EnemyManager : public Singleton<EnemyManager>
{
private:
	friend class Singleton<EnemyManager>;
	EnemyManager();
	typedef std::vector<std::shared_ptr<Enemy>> EnemyArray;

private:
	//各種別ごとに生成できる最大数
	static const int MAX_NUM = 100;
	//各モデルのボーン最大数
	static const int MAX_BONE_NUM = 32;
	//描画パイプライン（インスタンシング描画）
	std::shared_ptr<GraphicsPipeline>pipeline;

	//系統（型オブジェクト）
	std::array<std::unique_ptr<EnemyBreed>, ENEMY_TYPE_NUM>breeds;
	//生成済エネミー配列
	std::array<EnemyArray, ENEMY_TYPE_NUM>enemys;
	//ワールド行列配列用構造化バッファ
	std::array<std::shared_ptr<StructuredBuffer>, ENEMY_TYPE_NUM>worldMatriciesBuff;
	//ボーン行列配列用構造化バッファ
	std::array<std::shared_ptr<StructuredBuffer>, ENEMY_TYPE_NUM>boneMatriciesBuff;

public:
	void Spawn(const ENEMY_TYPE& Type, const Transform& InitTransform);
	void Update();
	void Draw(Camera& Cam, std::shared_ptr<CubeMap>AttachCubeMap);

	std::shared_ptr<Model>GetModel(const ENEMY_TYPE& Type)
	{
		return breeds[Type]->GetModel();
	}
};