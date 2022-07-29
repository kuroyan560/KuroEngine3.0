#pragma once
#include"Vec.h"
#include"KuroMath.h"
#include<memory>
class ConstantBuffer;
#include<string>

class Camera
{
public:
	enum ProjMatMode
	{
		Perspective,		//透視射影行列
		Ortho,			//平行投影
	};
private:
	class ConstData
	{
	public:
		Matrix matView = XMMatrixIdentity(); // ビュー行列
		Matrix matProjection = XMMatrixIdentity();	//プロジェクション行列
		Matrix billboardMat = XMMatrixIdentity();
		Matrix billboardMatY = XMMatrixIdentity();
		Vec3<float> eye = { 0,0,0 }; // カメラ座標（ワールド座標）
	}cameraInfo;
	std::shared_ptr<ConstantBuffer>buff;

	//プロジェクション行列が透視射影か平行投影かどうか
	ProjMatMode projMatMode = Perspective;

	Vec3<float>pos = { 0,0,-10 };
	Vec3<float>target = { 0,0,0 };
	Vec3<float>up = { 0,1,0 };
	Angle angleOfView = Angle(60);	//画角
	float nearZ = 0.1f;
	float farZ = 3000.0f;
	float aspect = 0.0f;

	//平行投影用
	float width = 1280.0f;
	float height = 720.0f;

	Matrix viewInvMat; //ビュー行列の逆行列

	//マッピングの判断用ダーティフラグ
	bool dirty = true;
	void CameraInfoUpdate();

public:
	const std::string name;
	Camera(const std::string& Name);

	//セッタ
	void SetProjMatMode(const ProjMatMode& Mode)
	{
		projMatMode = Mode;
		dirty = true;
	}
	void SetNearZ(const float& NearZ)
	{
		nearZ = NearZ;
		dirty = true;
	}
	void SetFarZ(const float& FarZ)
	{
		farZ = FarZ;
		dirty = true;
	}
	void SetPos(const Vec3<float>& Pos) 
	{
		pos = Pos;
		dirty = true;
	}
	void SetUp(const Vec3<float>& Up)
	{
		up = Up;
		dirty = true;
	}
	void SetTarget(const Vec3<float>& Target) 
	{
		target = Target;
		dirty = true;
	}
	void SetAngleOfView(const Angle& Angle)
	{
		angleOfView = Angle;
		dirty = true;
	}
	void SetWidth(const float& Width)
	{
		width = Width;
		dirty = true;
	}
	void SetHeight(const float& Height)
	{
		height = Height;
		dirty = true;
	}
	void SetAspect(const float& Aspect)
	{
		aspect = Aspect;
		dirty = true;
	}

	//ゲッタ
	const Vec3<float>& GetPos() { return pos; }
	const Vec3<float>& GetUp() { return up; }
	const Vec3<float>& GetTarget() { return target; }
	const Angle& GetAngleOfView() { return angleOfView; }
	const Matrix& GetViewMat() { CameraInfoUpdate(); return cameraInfo.matView; }
	const Matrix& GetProjectionMat() { CameraInfoUpdate(); return cameraInfo.matProjection; }
	const Vec3<float>GetForward();
	const Vec3<float>GetRight();
	const float& GetNearZ() { return nearZ; }
	const float& GetFarZ() { return farZ; }

	const std::shared_ptr<ConstantBuffer>&GetBuff();
};

