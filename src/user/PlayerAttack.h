#pragma once
#include<array>
#include<string>
#include<memory>
#include"Collider.h"
class ModelAnimator;
#include"HitParticle.h"
#include"KuroMath.h"

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
	//アニメーションのスピード
	std::array<float, m_attackAnimNum>m_animSpeed;

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

	//攻撃の勢いによる前進
	float m_momentum;
	//攻撃の勢いの方向ベクトル
	Vec3<float>m_attackVec;
	//勢いが乗るフレーム数
	std::array<int, m_attackAnimNum>m_momentumFrameNum;
	//勢いの最大値
	std::array<float, m_attackAnimNum>m_maxMomentum;
	//勢いのイージングパラメータ
	std::array<EasingParameter, m_attackAnimNum>m_momentumEaseParameters;

public:
	PlayerAttack(const std::string& AnimNameTag) : m_animNameTag(AnimNameTag)
	{
		//初期値設定
		m_canNextInputFrame.fill(10); 
		m_momentumFrameNum.fill(10);
		m_maxMomentum.fill(1.0f);
		m_animSpeed.fill(1.0f);
	}
	void Attach(std::shared_ptr<ModelAnimator>& Animator, std::shared_ptr<Collider>& AttackCollider);
	void Init();
	void Update();
	void Attack(const Vec3<float>& InputMoveVec);
	void Stop();

	//攻撃中か
	const bool& IsActive() { return m_isActive; }
	//攻撃の勢いゲッタ
	Vec3<float>GetMomentum()
	{
		return m_attackVec * m_momentum;
	}

	//imguiデバッグ機能
	void ImguiDebug();
};