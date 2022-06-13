#pragma once
#include<memory>
class EnemyAttack;
#include"Transform.h"
class ModelAnimator;
class EnemyBreed;

class Enemy
{
private:
	friend class EnemyManager;

private:
	//系統（型オブジェクト）、普遍的な敵の情報
	const EnemyBreed& breed;
	//トランスフォーム
	Transform transform;
	//攻撃パターン
	std::shared_ptr<EnemyAttack> attack = nullptr;

	//モデルアニメータ
	std::shared_ptr<ModelAnimator>animator;

	int hp;

public:
	Enemy(const EnemyBreed& Breed, const Transform& InitTransform);
	void Init();
	void Update();
	void Damage(const int& Amount);
	bool IsAlive()const { return 0 < hp; }

	//ゲッタ
	const Matrix& GetWorldMat() { return transform.GetMat(); }
	const Transform& GetTransform() { return transform; }
	const std::shared_ptr<ModelAnimator>& GetAnimator() { return animator; }
};