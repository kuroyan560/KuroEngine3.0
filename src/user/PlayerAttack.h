#pragma once
#include<array>
#include<string>
#include<memory>
#include"Collider.h"
class ModelAnimator;
#include"HitParticle.h"

class PlayerAttack
{
private:
	static const int s_attackAnimNum = 4;
	std::array<std::string, s_attackAnimNum>m_animName =
	{
		"Attack_0","Attack_1","Attack_2","Attack_3"
	};

	class AttackColliderCallBack : public CollisionCallBack
	{
		PlayerAttack* m_parent;
		void OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)override;
	public:
		AttackColliderCallBack(PlayerAttack* Parent) :m_parent(Parent) {}
	}m_attackColliderCallBack;

	//アタッチされたアニメーターとコライダー
	std::weak_ptr<ModelAnimator>m_attachAnimator;

	//攻撃判定をとるコライダー
	std::weak_ptr<Collider>m_attackCollider;

	//攻撃中か
	bool m_isActive = false;

	//アニメーション番号
	int m_nowIdx = 0;

	//アニメーション再生
	void AnimPlay();

	//１度の攻撃で１回だけコールバック処理を呼び出すためのフラグ
	bool m_callBack = false;

public:
	PlayerAttack() :m_attackColliderCallBack(this) {}
	void Attach(std::shared_ptr<ModelAnimator>& Animator, std::shared_ptr<Collider>& AttackCollider);
	void Init();
	void Update();
	void Start();
	void Stop();
};