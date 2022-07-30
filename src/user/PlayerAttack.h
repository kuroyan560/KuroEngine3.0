#pragma once
#include<array>
#include<string>
#include<memory>
#include"Collider.h"
class ModelAnimator;
#include"HitParticle.h"

class PlayerAttack : public CollisionCallBack
{
private:
	//攻撃アニメーション数
	static const int m_attackAnimNum = 4;	
	//アニメーション名の先頭文字列
	std::string m_animNameTag = "Attack_";
	//インデックスからアニメーション名取得
	std::string GetAnimName(const int& AnimIdx)
	{
		return m_animNameTag + std::to_string(AnimIdx);
	}

	//アタッチされたアニメーターとコライダー
	std::weak_ptr<ModelAnimator>m_attachAnimator;

	//攻撃判定をとるコライダー
	std::weak_ptr<Collider>m_attackCollider;

	//攻撃中か
	bool m_isActive = false;
	//次の攻撃への予約入力
	bool m_nextAttack = false;	
	//攻撃アニメーションが始まってからのフレーム数
	int m_attackFrame;
	//次の攻撃への予約入力を受け付けるフレーム
	std::array<int, m_attackAnimNum>m_canNextInputFrame;

	//アニメーション番号
	int m_nowIdx = 0;
	//アニメーション再生
	void AnimPlay();

	//１度の攻撃で１回だけコールバック処理を呼び出すためのフラグ
	bool m_callBack = false;
	//攻撃の当たり判定用コールバック関数
	void OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)override;


public:
	PlayerAttack() { m_canNextInputFrame.fill(10); }
	void Attach(std::shared_ptr<ModelAnimator>& Animator, std::shared_ptr<Collider>& AttackCollider);
	void Init();
	void Update();
	void Attack();
	void Stop();

	const bool& IsActive() { return m_isActive; }

	//imguiデバッグ機能
	void ImguiDebug();
};