#include "PlayerAttack.h"
#include"Collider.h"
#include"ModelAnimator.h"

void PlayerAttack::AnimPlay()
{
	m_attachAnimator.lock()->Play(m_animName[m_nowIdx], false, false);
}

void PlayerAttack::Attach(std::shared_ptr<ModelAnimator>& Animator, std::shared_ptr<Collider>& LeftHandCollider, std::shared_ptr<Collider>& RightHandCollider)
{
	m_attachAnimator = Animator;
	m_leftHandCol = LeftHandCollider;
	m_rightHandCol = RightHandCollider;
	m_leftHandCol.lock()->SetCallBack(&m_attackColliderCallBack);
	m_rightHandCol.lock()->SetCallBack(&m_attackColliderCallBack);
}

void PlayerAttack::Init()
{
	m_leftHandCol.lock()->SetActive(false);
	m_rightHandCol.lock()->SetActive(false);
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
		//一番最初のアニメーション
		if(m_readyAnim)
		{
			m_leftHandCol.lock()->SetActive(true);
			m_rightHandCol.lock()->SetActive(true);
			m_readyAnim = false;
		}

		m_emitHitEffect = true;

		//複数の攻撃アニメーションをループ
		m_nowIdx++;
		if (s_attackAnimNum <= m_nowIdx)m_nowIdx = 0;
		AnimPlay();
	}
}

void PlayerAttack::Start()
{
	m_nowIdx = 0;
	AnimPlay();
	m_isActive = true;
	m_readyAnim = true;
}

void PlayerAttack::Stop()
{
	m_isActive = false;
	m_leftHandCol.lock()->SetActive(false);
	m_rightHandCol.lock()->SetActive(false);
}

#include"HitEffect.h"
void PlayerAttack::AttackColliderCallBack::OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)
{
	if (m_parent->m_emitHitEffect)
	{
		HitEffect::Generate(Inter);
		m_parent->m_emitHitEffect = false;
	}
}