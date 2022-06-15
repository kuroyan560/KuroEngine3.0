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
};

//衝突判定があった際に呼び出される
class CollisionCallBack
{
protected:
	friend class Collider;
	/// <summary>
	/// 衝突時呼び出される
	/// </summary>
	/// <param name="Inter">衝突点</param>
	/// <param name="OthersAttribute">衝突した相手のAttribute</param>
	virtual void OnCollision(const Vec3<float>& Inter, const COLLIDER_ATTRIBUTE& OthersAttribute) = 0;
};

class Collider : public std::enable_shared_from_this<Collider>
{
private:
	static std::list<std::weak_ptr<Collider>>COLLIDERS;

public:
	static void UpdateAllColliders();

private:
	//自身の振る舞い
	char myAttribute = COLLIDER_ATTRIBUTE::NONE;

	//衝突判定を行う相手の振る舞い
	char hitCheckAttribute = COLLIDER_ATTRIBUTE::NONE;

	//コールバック関数
	CollisionCallBack* callBack = nullptr;

	//衝突判定用プリミティブ
	std::shared_ptr<CollisionPrimitive>primitive;

	//有効フラグ
	bool isActive = true;

	//当たり判定があったかフラグ
	bool isHit = false;

public:
	Collider() = delete;
	Collider(const Collider& arg) = delete;
	Collider(Collider&& arg) = delete;
	Collider(const std::shared_ptr<CollisionPrimitive>& Primitive) :primitive(Primitive) { COLLIDERS.emplace_back(shared_from_this()); }

	//当たり判定（衝突点を返す）
	void CheckHitCollision(std::weak_ptr<Collider>& Other);

	//セッタ
	void SetCallBack(CollisionCallBack* CallBack) { callBack = CallBack; }
	void SetMyAttribute(const COLLIDER_ATTRIBUTE& Attribute) { myAttribute = Attribute; }
	void SetHitCheckAttribute(const COLLIDER_ATTRIBUTE& Attribute) { hitCheckAttribute = Attribute; }
	void SetActive(const bool& Active) { isActive = Active; }

	//ゲッタ
	const bool& GetIsHit()const { return isHit; }
};