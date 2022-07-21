#pragma once
#include"Camera.h"
#include<memory>
class DebugCamera
{
	std::shared_ptr<Camera>m_cam;

	//カメラ～注視点までの距離
	float m_dist = 20.0f;
	//スケーリング
	Vec2<float>m_scale;
	//回転行列
	Matrix m_matRot = XMMatrixIdentity();

	void MoveXMVector(const XMVECTOR& MoveVector);

public:
	DebugCamera();
	void Init(const Vec3<float>& InitPos, const Vec3<float>& Target);
	void Move();

	operator std::shared_ptr<Camera>& ()
	{
		return m_cam;
	}

	operator Camera& ()
	{
		return *m_cam;
	}
};