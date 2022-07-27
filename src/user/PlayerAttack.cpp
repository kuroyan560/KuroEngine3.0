#include "PlayerAttack.h"
#include"Collider.h"
#include"ModelAnimator.h"

void PlayerAttack::AnimPlay()
{
	m_attachAnimator.lock()->Play(m_animName[m_nowIdx], false, false);
}

void PlayerAttack::Attach(std::shared_ptr<ModelAnimator>& Animator, std::shared_ptr<Collider>& AttackCollider)
{
	m_attachAnimator = Animator;

	AttackCollider->SetCallBack(&m_attackColliderCallBack);
	m_attackCollider = AttackCollider;
}

void PlayerAttack::Init()
{
	m_attackCollider.lock()->SetActive(false);
	m_isActive = false;
}

void PlayerAttack::Update()
{
	//攻撃中でない
	if (!m_isActive)return;

	auto animator = m_attachAnimator.lock();

	//現在の攻撃アニメーションが終了したら次のアニメーションへ
	if (!animator->IsPlay(m_animName[m_nowIdx]))
	{
		//複数の攻撃アニメーションをループ
		m_nowIdx++;
		if (s_attackAnimNum <= m_nowIdx)
		{
			Stop();
		}
		AnimPlay();
	}
}

void PlayerAttack::Start()
{
	m_nowIdx = 0;
	AnimPlay();
	m_isActive = true;
	m_callBack = true;
	m_attackCollider.lock()->SetActive(true);
}

void PlayerAttack::Stop()
{
	m_isActive = false;
	m_attackCollider.lock()->SetActive(false);
}

#include"HitEffect.h"
void PlayerAttack::AttackColliderCallBack::OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)
{
	if (m_parent->m_callBack)
	{
		HitEffect::Generate(Inter);
		m_parent->m_callBack = false;
	}
}