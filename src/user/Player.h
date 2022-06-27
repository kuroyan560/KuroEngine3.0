#pragma once
#include<string>
#include"PlayerCamera.h"
#include<memory>
#include<vector>
#include"PlayerAttack.h"
class ModelObject;
class Camera;
class Collider;

class Player
{
private:
	//インスタンス生成を１個に制限するためのフラグ
	static bool INSTANCED;

	//プレイヤー操作カメラ（プレイヤーが作り変えられる度に、カメラを生成しなくて済むように）
	static std::unique_ptr<PlayerCamera> CAMERA;

	//ステータス
	enum STATUS { WAIT, RUN, ATTACK, STATUS_NUM };

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
	//ステータス
	STATUS status = WAIT;
	STATUS oldStatus = WAIT;	//１フレーム前の状態（トリガー判定用）
	bool StatusTrigger(const STATUS& Status) { return status == Status && oldStatus != Status; }	//ステータスのトリガー判定

	//モデル
	std::shared_ptr<ModelObject>model;

	//コライダー
	std::vector<std::shared_ptr<Collider>>colliders;

	//攻撃処理クラス
	PlayerAttack attack;

	//移動処理
	void Move();

	//アニメーション切り替え
	void AnimationSwitch();

public:
	Player();
	~Player() { INSTANCED = false; }
	void Init();
	void Update();
	void Draw(Camera& Cam);
	std::weak_ptr<ModelObject>GetModelObj() { return model; }
};

