#include "Collider.h"

std::list<std::weak_ptr<Collider>>Collider::COLLIDERS;

std::shared_ptr<Collider> Collider::Generate(const std::shared_ptr<CollisionPrimitive>& Primitive)
{
	auto instance = std::make_shared<Collider>(Primitive);
	COLLIDERS.emplace_back(instance);
	return instance;
}

void Collider::UpdateAllColliders()
{
	//既に寿命切れのコライダーを削除
	COLLIDERS.remove_if([](std::weak_ptr<Collider>& col) {return col.expired(); });

	//当たり判定記録リセット
	for (auto& col : COLLIDERS)col.lock()->isHit = false;

	//総当り衝突判定
	for (auto itrA = COLLIDERS.begin(); itrA != COLLIDERS.end(); ++itrA)
	{
		auto colA = itrA->lock();

		auto itrB = itrA;
		++itrB;
		for (; itrB != COLLIDERS.end(); ++itrB)
		{
			colA->CheckHitCollision(*itrB);
		}
	}
}

void Collider::DebugDrawAllColliders(Camera& Cam)
{
	for (auto& col : COLLIDERS)
	{
		if (!col.lock()->isActive)continue;
		col.lock()->DebugDraw(Cam);
	}
}

void Collider::CheckHitCollision(std::weak_ptr<Collider>& Other)
{
	auto other = Other.lock();

	//いずれかのコライダーが有効でない
	if (!this->isActive || !other->isActive)return;

	//衝突判定を行う相手ではない
	if (!(this->hitCheckAttribute & other->myAttribute))return;
	if (!(this->myAttribute & other->hitCheckAttribute))return;

	//判定
	Vec3<float>inter;
	bool hit = Collision::CheckPrimitiveHit(this->primitive.get(), other->primitive.get(),&inter);

	//衝突していたら
	if (hit)
	{
		this->isHit = true;
		other->isHit = true;
		//コールバック関数呼び出し
		if (this->callBack)this->callBack->OnCollision(inter, (COLLIDER_ATTRIBUTE)other->myAttribute);
		if (other->callBack)other->callBack->OnCollision(inter, (COLLIDER_ATTRIBUTE)this->myAttribute);
	}
}

void Collider::DebugDraw(Camera& Cam)
{
	if (!isActive)return;
	primitive->DebugDraw(isHit, Cam);
}
