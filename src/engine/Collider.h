#pragma once
#include<vector>
#include"Vec.h"
#include<memory>
#include"Collision.h"

//振る舞い（ビット演算）
enum COLLIDER_ATTRIBUTE
{
	NONE = 0b11111111,
	ENEMY = 0b00000001,
	PLAYER = 0b00000010,
	FLOOR = 0b00000100,
	FOOT_POINT = 0b00001000,
};

//衝突判定があった際に呼び出される
class CollisionCallBack
{
private:
	friend class Collider;
	std::weak_ptr<Collider>m_attachCollider;

protected:
	const std::weak_ptr<Collider>& GetAttachCollider() { return m_attachCollider; }
	/// <summary>
	/// 衝突時呼び出される
	/// </summary>
	/// <param name="Inter">衝突点</param>
	/// <param name="OthersAttribute">衝突した相手のAttribute</param>
	virtual void OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider) = 0;
};

class Collider : public std::enable_shared_from_this<Collider>
{
private:
	static std::list<std::weak_ptr<Collider>>s_colliderList;

public:
	static std::shared_ptr<Collider>Generate(const std::shared_ptr<CollisionPrimitive>& Primitive);
	static void UpdateAllColliders();
	static void DebugDrawAllColliders(Camera& Cam);

private:
	//自身の振る舞い
	char m_myAttribute = COLLIDER_ATTRIBUTE::NONE;

	//衝突判定を行う相手の振る舞い
	char m_hitCheckAttribute = COLLIDER_ATTRIBUTE::NONE;

	//コールバック関数
	CollisionCallBack* m_callBack = nullptr;

	//衝突判定用プリミティブ
	std::shared_ptr<CollisionPrimitive>m_primitive;

	//有効フラグ
	bool m_isActive = true;

	//当たり判定があったかフラグ
	bool m_isHit = false;

public:
	Collider(const std::shared_ptr<CollisionPrimitive>& Primitive) :m_primitive(Primitive) {  }

	//当たり判定（衝突点を返す）
	bool CheckHitCollision(std::weak_ptr<Collider> Other, Vec3<float>* Inter = nullptr);

	//当たり判定描画
	void DebugDraw(Camera& Cam);

	//セッタ
	void SetCallBack(CollisionCallBack* CallBack) 
	{ 
		m_callBack = CallBack; 
		m_callBack->m_attachCollider = weak_from_this();
	}
	void SetMyAttribute(const COLLIDER_ATTRIBUTE& Attribute) { m_myAttribute = Attribute; }
	void SetHitCheckAttribute(const COLLIDER_ATTRIBUTE& Attribute) { m_hitCheckAttribute = Attribute; }
	void SetActive(const bool& Active) { m_isActive = Active; }

	//ゲッタ
	const bool& GetIsHit()const { return m_isHit; }
	const std::weak_ptr<CollisionPrimitive>GetColliderPrimitive() { return m_primitive; }
};