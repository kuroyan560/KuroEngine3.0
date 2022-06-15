#pragma once
#include"Vec.h"
#include<cmath>
#include"Common/KuroFunc.h"
#include"Angle.h"
#include<list>
class Transform2D
{
	static std::list<Transform2D*> TRANSFORMS;
public:
	static void DirtyReset()
	{
		for (auto& trans : TRANSFORMS)
		{
			trans->dirty = false;
		}
	}
private:
	Transform2D* parent = nullptr;

	Matrix mat = XMMatrixIdentity();
	Vec2<float>pos = { 0,0 };
	Vec2<float>scale = { 1,1 };
	Matrix rotate = XMMatrixIdentity();

	bool dirty = true;

	void MatReset()
	{
		dirty = true;
	}

public:
	Transform2D(Transform2D* Parent = nullptr) {
		SetParent(Parent);
		TRANSFORMS.emplace_back(this);
	}
	~Transform2D() {
		(void)TRANSFORMS.remove_if([this](Transform2D* tmp) {
			return tmp == this;
			});
	}
	void SetParent(Transform2D* Parent) {
		parent = Parent;
		MatReset();
	}

	//ゲッタ
	const Vec2<float>& GetPos() { return pos; }
	const Vec2<float>& GetScale() { return scale; }
	const Vec3<Angle>& GetAngle() {
		auto sy = rotate.r[0].m128_f32[2];
		auto unlocked = std::abs(sy) < 0.99999f;
		return Vec3<Angle>(
			unlocked ? std::atan2(-rotate.r[1].m128_f32[2], rotate.r[2].m128_f32[2]) : std::atan2(rotate.r[2].m128_f32[1], rotate.r[1].m128_f32[1]),
			std::asin(sy),
			unlocked ? std::atan2(-rotate.r[0].m128_f32[1], rotate.r[0].m128_f32[0]) : 0);
	}
	const XMVECTOR& GetQuaternion() {
		return XMQuaternionRotationMatrix(rotate);
	}
	const Vec3<float>GetAxis() {
		XMVECTOR quat = GetQuaternion();
		return Vec3<float>(quat.m128_f32[0], quat.m128_f32[1], quat.m128_f32[2]);
	}

	//セッタ
	void SetPos(const Vec2<float> Pos) {
		if (pos == Pos)return;
		pos = Pos;
		MatReset();
	}
	void SetScale(const Vec2<float> Scale) {
		if (scale == Scale)return;
		scale = Scale;
		MatReset();
	}
	void SetRotate(const Vec3<Angle>& Rotate) {
		rotate = XMMatrixRotationRollPitchYaw(Rotate.x, Rotate.y, Rotate.z);
		MatReset();
	}
	void SetRotate(const XMVECTOR& Quaternion) {
		rotate = XMMatrixRotationQuaternion(Quaternion);
		MatReset();
	}
	void SetRotate(const Vec3<float>& Axis, const Angle& Angle) {
		Vec3<float>axis = Axis;
		if (1.0f < axis.Length())axis.Normalize();
		XMVECTOR vec = XMVectorSet(axis.x, axis.y, axis.z, 1.0f);
		rotate = XMMatrixRotationQuaternion(XMQuaternionRotationAxis(vec, Angle));
		MatReset();
	}
	const Matrix& GetMat();
	const bool& GetDirty() { return dirty; }
};