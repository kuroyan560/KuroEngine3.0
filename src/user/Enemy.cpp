#include "Enemy.h"
#include"EnemyBreed.h"
#include"Model.h"
#include"ModelAnimator.h"
#include"Collision.h"
#include"Collider.h"

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

	//コライダー生成
	const float COLLIDER_RADIUS = 7.0f;
	auto colSphere = std::make_shared<CollisionSphere>(COLLIDER_RADIUS, &transform);
	colSphere->offset = { 0.0f, 0.5f, 0.0f };
	collider = Collider::Generate(colSphere);
	collider->SetMyAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	collider->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::PLAYER);

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
