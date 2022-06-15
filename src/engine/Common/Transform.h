#pragma once
#include"Vec.h"
#include<memory>
#include"Angle.h"
#include"KuroFunc.h"
#include<cmath>
#include<list>

class Transform
{
	static std::list<Transform*> TRANSFORMS;
public:
	static void DirtyReset()
	{
		for (auto& trans : TRANSFORMS)
		{
			trans->dirty = false;
		}
	}
private:
	Transform* parent = nullptr;

	Matrix mat = XMMatrixIdentity();
	Vec3<float>pos = { 0,0,0 };
	Vec3<float>scale = { 1,1,1 };
	Matrix rotate = DirectX::XMMatrixIdentity();

	bool dirty = true;

	void MatReset()
	{
		dirty = true;
	}

public:
	Transform(Transform* Parent = nullptr) {
		SetParent(Parent);
		TRANSFORMS.emplace_back(this);
	}
	~Transform() {
		(void)TRANSFORMS.remove_if([this](Transform* tmp) {
			return tmp == this;
			});
	}
	void SetParent(Transform* Parent) {

		parent = Parent;
		MatReset();
	}

	//ゲッタ
	const Vec3<float>& GetPos()const { return pos; }
	const Vec3<float>& GetScale()const{ return scale; }
	const Vec3<Angle>& GetAngle()const {
		auto sy = rotate.r[0].m128_f32[2];
		auto unlocked = std::abs(sy) < 0.99999f;
		return Vec3<Angle>(
			unlocked ? std::atan2(-rotate.r[1].m128_f32[2], rotate.r[2].m128_f32[2]) : std::atan2(rotate.r[2].m128_f32[1], rotate.r[1].m128_f32[1]),
			std::asin(sy),
			unlocked ? std::atan2(-rotate.r[0].m128_f32[1], rotate.r[0].m128_f32[0]) : 0);
	}
	const XMVECTOR& GetQuaternion()const {
		return XMQuaternionRotationMatrix(rotate);
	}
	const Matrix& GetRotate()const { return rotate; }
	Vec3<float> GetFront()const {
		XMVECTOR front = XMVectorSet(0, 0, 1, 1);
		front = XMVector3Transform(front, rotate);
		return Vec3<float>(front.m128_f32[0],front.m128_f32[1],front.m128_f32[2]);
	}
	Vec3<float> GetRight()const {
		XMVECTOR right = XMVectorSet(1, 0, 0, 1);
		right = XMVector3Transform(right, rotate);
		return Vec3<float>(right.m128_f32[0], right.m128_f32[1], right.m128_f32[2]);
	}
	Vec3<float> GetUp()const {
		XMVECTOR up = XMVectorSet(0, 1, 0, 1);
		up = XMVector3Transform(up, rotate);
		return Vec3<float>(up.m128_f32[0], up.m128_f32[1], up.m128_f32[2]);
	}

	//セッタ
	void SetPos(const Vec3<float> Pos) {
		if (pos == Pos)return;
		pos = Pos;
		MatReset();
	}
	void SetScale(const Vec3<float> Scale) { 
		if (scale == Scale)return;
		scale = Scale;
		MatReset();
	}
	void SetScale(const float Scale) {
		auto s = Vec3<float>(Scale, Scale, Scale);
		if (scale == s)return;
		scale = s;
		MatReset();
	}
	void SetRotate(const Vec3<Angle>& Rotate) { 
		rotate = KuroMath::RotateMat(Rotate);
		MatReset();
	}
	void SetRotate(const XMVECTOR& Quaternion) {
		rotate = XMMatrixRotationQuaternion(Quaternion);
		MatReset();
	}
	void SetRotate(const Vec3<float>& Axis, const Angle& Angle) {
		rotate = KuroMath::RotateMat(Axis, Angle);
		MatReset();
	}
	void SetRotate(const Matrix& RotateMat) {
		rotate = RotateMat;
		MatReset();
	}
	void SetLookAtRotate(const Vec3<float>& Target, const Vec3<float>& UpAxis = Vec3<float>(0, 1, 0)){
		Vec3<float>z = Vec3<float>(Target - pos).GetNormal();
		Vec3<float>x = UpAxis.Cross(z).GetNormal();
		Vec3<float>y = z.Cross(x).GetNormal();

		Matrix rot = XMMatrixIdentity();
		rot.r[0].m128_f32[0] = x.x; rot.r[0].m128_f32[1] = x.y; rot.r[0].m128_f32[2] = x.z;
		rot.r[1].m128_f32[0] = y.x; rot.r[1].m128_f32[1] = y.y; rot.r[1].m128_f32[2] = y.z;
		rot.r[2].m128_f32[0] = z.x; rot.r[2].m128_f32[1] = z.y; rot.r[2].m128_f32[2] = z.z;

		if (rot == rotate)return;
		rotate = rot;

		MatReset();
	}
	void SetUp(const Vec3<float>& Up)
	{
		Vec3<float> defUp = { 0,1,0 };
		Matrix rot = KuroMath::RotateMat(defUp, Up);
		if (rot == rotate)return;
		rotate = rot;

		MatReset();
	}
	void SetFront(const Vec3<float>& Front)
	{
		Vec3<float>defFront = { 0,0,1 };
		Matrix rot = KuroMath::RotateMat(defFront, Front);
		if (rot == rotate)return;
		rotate = rot;

		MatReset();
	}

	const Matrix& GetMat(const Matrix& BillBoardMat = XMMatrixIdentity());
	const bool& GetDirty() { return dirty; }
};