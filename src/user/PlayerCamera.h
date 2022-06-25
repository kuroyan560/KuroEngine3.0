#pragma once
#include"Angle.h"
#include<memory>
#include"ImguiDebugInterface.h"

class Camera;
class Transform;
struct PlayerCamera : public ImguiDebugInterface
{
private:
	void CalculatePos(const Transform& Player);
public:
	std::shared_ptr<Camera>cam;
	float dist = 5.0f;		//プレイヤーとの距離
	bool mirrorX = false;		//X入力ミラー
	bool mirrorY = false;		//Y入力ミラー

	//位置の角度
	Angle posAngle;

	//カメラの高さ
	float height;

	PlayerCamera();
	void Init(const Transform& Player);
	void Update(const Transform& Player);

	void OnImguiDebug()override;
};

