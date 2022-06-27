#pragma once
#include<array>
#include<string>
#include<memory>
class ModelAnimator;
class Collider;

class PlayerAttack
{
private:
	static const int ATTACK_ANIM_NUM = 4;
	std::array<std::string, ATTACK_ANIM_NUM>animName =
	{
		"Attack_0","Attack_1","Attack_2","Attack_3"
	};

	//アタッチされたアニメーターとコライダー
	std::weak_ptr<ModelAnimator>attachAnimator;
	std::weak_ptr<Collider>leftHandCol;
	std::weak_ptr<Collider>rightHandCol;

	//一番最初の攻撃アニメーションか（最初の攻撃は振りかぶってるだけ）
	bool readyAnim = false;

	//攻撃中か
	bool isActive = false;

	//アニメーション番号
	int nowIdx = 0;

	//アニメーション再生
	void AnimPlay();

public:
	PlayerAttack() {}
	void Attach(std::shared_ptr<ModelAnimator>& Animator, std::shared_ptr<Collider>& LeftHandCollider, std::shared_ptr<Collider>& RightHandCollider);
	void Init();
	void Update();
	void Start();
	void Stop();
};