#include "Enemy.h"
#include"EnemyBreed.h"
#include"Model.h"
#include"ModelAnimator.h"
#include"Collision.h"
#include"Collider.h"

Enemy::Enemy(const EnemyBreed& Breed, const Transform& InitTransform) : m_breed(Breed), m_transform(InitTransform)
{
	//系統クラスから攻撃パターンのクローンを受け取る
	if (m_breed.GetAttack())
	{
		//attack = std::make_shared<EnemyAttack>(breed.GetAttack());
	}
	//アニメーション情報を持つならアニメーター生成
	if (!m_breed.GetModel()->m_skelton->animations.empty())
	{
		m_animator = std::make_shared<ModelAnimator>(m_breed.GetModel());
	}

	//コライダー生成
	const float COLLIDER_RADIUS = 7.0f;
	auto colSphere = std::make_shared<CollisionSphere>(COLLIDER_RADIUS, &m_transform);
	colSphere->m_offset = { 0.0f, 0.5f, 0.0f };
	m_collider = Collider::Generate(colSphere);
	m_collider->SetMyAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	m_collider->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::PLAYER);

	Init();
}

void Enemy::Init()
{
	m_hp = m_breed.GetMaxHp();
	if (m_attack)m_attack->Init();
}

void Enemy::Update()
{
	//死んでいる
	if (!IsAlive())return;

	if (m_attack)m_attack->Update();
}

void Enemy::Damage(const int& Amount)
{
	m_hp -= Amount;
}
