#pragma once
#include"Angle.h"
#include<memory>
#include"ImguiDebugInterface.h"
#include"Vec.h"

class Camera;
class Transform;
struct PlayerCamera : public ImguiDebugInterface
{
private:
	void CalculatePos(const Transform& Player);
public:
	std::shared_ptr<Camera>m_cam;
	float m_dist = 5.0f;		//プレイヤーとの距離
	bool m_mirrorX = false;		//X入力ミラー
	bool m_mirrorY = false;		//Y入力ミラー

	//位置の角度
	Angle m_posAngle;

	//カメラの高さ
	float m_height;

	PlayerCamera();
	void Init(const Transform& Player);
	void Update(const Transform& Player, const Vec2<float>& InputVec);

	void OnImguiDebug()override;
};

