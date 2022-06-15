#include "Collider.h"

void Collider::CheckHitCollision(std::weak_ptr<Collider> Other)
{
	auto other = Other.lock();

	//いずれかのコライダーが有効でない
	if (!this->isActive || !other->isActive)return;

	//衝突判定を行う相手ではない
	if (!(this->hitCheckAttribute & other->myAttribute))return;
	if (!(this->myAttribute & other->hitCheckAttribute))return;

	//判定
	Vec3<float>inter;
	bool hit = Collision::CheckPrimitiveHit(this->primitive.get(), other->primitive.get(), &inter);

	//衝突していたら
	if (hit)
	{
		//コールバック関数呼び出し
		if (this->callBack)this->callBack->OnCollision(inter, (COLLIDER_ATTRIBUTE)other->myAttribute);
		if (other->callBack)other->callBack->OnCollision(inter, (COLLIDER_ATTRIBUTE)this->myAttribute);
	}
}
