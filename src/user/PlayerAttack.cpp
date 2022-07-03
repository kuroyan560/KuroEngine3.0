#include "PlayerAttack.h"
#include"Collider.h"
#include"ModelAnimator.h"

void PlayerAttack::AnimPlay()
{
	attachAnimator.lock()->Play(animName[nowIdx], false, false);
}

void PlayerAttack::Attach(std::shared_ptr<ModelAnimator>& Animator, std::shared_ptr<Collider>& LeftHandCollider, std::shared_ptr<Collider>& RightHandCollider)
{
	attachAnimator = Animator;
	leftHandCol = LeftHandCollider;
	rightHandCol = RightHandCollider;
	leftHandCol.lock()->SetCallBack(&attackColliderCallBack);
	rightHandCol.lock()->SetCallBack(&attackColliderCallBack);
}

void PlayerAttack::Init()
{
	leftHandCol.lock()->SetActive(false);
	rightHandCol.lock()->SetActive(false);
	isActive = false;
}

void PlayerAttack::Update()
{
	//攻撃中でない
	if (!isActive)return;

	auto animator = attachAnimator.lock();

	//現在の攻撃アニメーションが終了したら次のアニメーションへ
	if (!animator->IsPlay(animName[nowIdx]))
	{
		//一番最初のアニメーション
		if(readyAnim)
		{
			leftHandCol.lock()->SetActive(true);
			rightHandCol.lock()->SetActive(true);
			readyAnim = false;
		}

		emitHitEffect = true;

		//複数の攻撃アニメーションをループ
		nowIdx++;
		if (ATTACK_ANIM_NUM <= nowIdx)nowIdx = 0;
		AnimPlay();
	}
}

void PlayerAttack::Start()
{
	nowIdx = 0;
	AnimPlay();
	isActive = true;
	readyAnim = true;
}

void PlayerAttack::Stop()
{
	isActive = false;
	leftHandCol.lock()->SetActive(false);
	rightHandCol.lock()->SetActive(false);
}

#include"HitEffect.h"
void PlayerAttack::AttackColliderCallBack::OnCollision(const Vec3<float>& Inter, const COLLIDER_ATTRIBUTE& OthersAttribute)
{
	if (parent->emitHitEffect)
	{
		HitEffect::Generate(Inter);
		parent->emitHitEffect = false;
	}
}