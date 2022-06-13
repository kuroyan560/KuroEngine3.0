#include "Enemy.h"
#include"EnemyBreed.h"
#include"Model.h"
#include"ModelAnimator.h"

Enemy::Enemy(const EnemyBreed& Breed, const Transform& InitTransform) : breed(Breed), transform(InitTransform)
{
	//系統クラスから攻撃パターンのクローンを受け取る
	if (breed.GetAttack())
	{
		//attack = std::make_shared<EnemyAttack>(breed.GetAttack());
	}
	//アニメーション情報を持つならアニメーター生成
	if (!breed.GetModel()->skelton->animations.empty())
	{
		animator = std::make_shared<ModelAnimator>(breed.GetModel());
	}

	Init();
}

void Enemy::Init()
{
	hp = breed.GetMaxHp();
	if (attack)attack->Init();
}

void Enemy::Update()
{
	//死んでいる
	if (!IsAlive())return;

	if (attack)attack->Update();
}

void Enemy::Damage(const int& Amount)
{
	hp -= Amount;
}
