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
	const int m_maxHp;
	//攻撃パターンのオリジナル
	const EnemyAttack* m_attack = nullptr;
	//モデル
	std::shared_ptr<Model>m_model;

public:
	EnemyBreed(const int& Hp, const std::shared_ptr<Model>& Model, EnemyAttack* Attack = nullptr) :m_maxHp(Hp), m_model(Model), m_attack(Attack) {}
	~EnemyBreed() { delete m_attack; m_attack = nullptr; }

	//ゲッタ
	const std::shared_ptr<Model>& GetModel()const { return m_model; }
	const int& GetMaxHp()const { return m_maxHp; }
	const EnemyAttack* GetAttack()const { return m_attack; }	//あくまでクローン元なのでconst
};