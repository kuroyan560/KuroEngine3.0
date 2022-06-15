#pragma once
#include"Vec.h"

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

class Collider
{
	char attribute = COLLIDER_ATTRIBUTE::NONE;

	//コールバック関数
	CollisionCallBack* callback = nullptr;

public:
};