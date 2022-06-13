#pragma once
#include<string>
#include"PlayerCamera.h"
#include<memory>
class ModelObject;
class Camera;



class Player
{
private:
	static bool INSTANCED;	//インスタンス生成を１個に制限するためのフラグ

	//プレイヤー操作カメラ
	static std::unique_ptr<PlayerCamera> CAMERA;

public:
	//カメラキー
	static const std::string CAMERA_KEY;
	//カメラポインタ
	static const std::shared_ptr<Camera>& GetCam() 
	{ 
		if (!CAMERA)CAMERA = std::make_unique<PlayerCamera>();
		return CAMERA->cam;
	}

	//入力の受け付け状態
	struct CanInput
	{
		bool playerControl = true;
		bool camControl = true;
	};
	static CanInput CAN_INPUT;

private:
	//モデル
	std::shared_ptr<ModelObject>model;

	//移動処理
	void Move();

public:
	Player();
	~Player() { INSTANCED = false; }
	void Init();
	void Update();
	void Draw(Camera& Cam);
};

