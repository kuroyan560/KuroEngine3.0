#pragma once
#include<memory>
class Model;

//エネミー攻撃パターン
class EnemyAttack
{
public:
	//初期化
	virtual void Init() = 0;
	//更新
	virtual void Update() = 0;

	//継承後の攻撃パターンのクローンを生成させる
	virtual EnemyAttack* Clone()const = 0;
};

//エネミーの系統（型オブジェクト）
class EnemyBreed
{
	//HP最大値
	const int maxHp;
	//攻撃パターンのオリジナル
	const EnemyAttack* attack = nullptr;
	//モデル
	std::shared_ptr<Model>model;

public:
	EnemyBreed(const int& Hp, const std::shared_ptr<Model>& Model, EnemyAttack* Attack = nullptr) :maxHp(Hp), model(Model), attack(Attack) {}
	~EnemyBreed() { delete attack; attack = nullptr; }

	//ゲッタ
	const std::shared_ptr<Model>& GetModel()const { return model; }
	const int& GetMaxHp()const { return maxHp; }
	const EnemyAttack* GetAttack()const { return attack; }	//あくまでクローン元なのでconst
};