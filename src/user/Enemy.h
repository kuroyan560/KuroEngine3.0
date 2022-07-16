#pragma once
#include<memory>
class EnemyAttack;
#include"Transform.h"
class ModelAnimator;
class EnemyBreed;
class Collider;

class Enemy
{
private:
	friend class EnemyManager;

private:
	//系統（型オブジェクト）、普遍的な敵の情報
	const EnemyBreed& m_breed;
	//トランスフォーム
	Transform m_transform;
	//攻撃パターン
	std::shared_ptr<EnemyAttack> m_attack = nullptr;

	//モデルアニメータ
	std::shared_ptr<ModelAnimator>m_animator;

	//コライダー
	std::shared_ptr<Collider>m_collider;

	int m_hp;

public:
	Enemy(const EnemyBreed& Breed, const Transform& InitTransform);
	void Init();
	void Update();
	void Damage(const int& Amount);
	bool IsAlive()const { return 0 < m_hp; }

	//ゲッタ
	const Matrix& GetWorldMat() { return m_transform.GetMat(); }
	const Transform& GetTransform() { return m_transform; }
	const std::shared_ptr<ModelAnimator>& GetAnimator() { return m_animator; }
};