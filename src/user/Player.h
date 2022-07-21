#pragma once
#include<string>
#include"PlayerCamera.h"
#include<memory>
#include<vector>
#include"PlayerAttack.h"
#include"Collider.h"
class ModelObject;
class Camera;


class Player
{
private:
	//インスタンス生成を１個に制限するためのフラグ
	static bool s_instanced;

	//プレイヤー操作カメラ（プレイヤーが作り変えられる度に、カメラを生成しなくて済むように）
	static std::unique_ptr<PlayerCamera> s_camera;

	//ステータス
	enum STATUS { WAIT, RUN, ATTACK, STATUS_NUM };

public:
	//カメラキー
	static const std::string s_cameraKey;
	//カメラポインタ
	static const std::shared_ptr<Camera>& GetCam() 
	{ 
		if (!s_camera)s_camera = std::make_unique<PlayerCamera>();
		return s_camera->m_cam;
	}

	//入力の受け付け状態
	struct CanInput
	{
		bool m_playerControl = true;
		bool m_camControl = true;
	};
	static CanInput s_canInput;

private:
	//ステータス
	STATUS m_status = WAIT;
	STATUS m_oldStatus = WAIT;	//１フレーム前の状態（トリガー判定用）
	bool StatusTrigger(const STATUS& Status) { return m_status == Status && m_oldStatus != Status; }	//ステータスのトリガー判定

	//モデル
	std::shared_ptr<ModelObject>m_model;

	//コライダー
	std::vector<std::shared_ptr<Collider>>m_colliders;	//配列

	//攻撃処理クラス
	PlayerAttack m_attack;

	//押し戻しコールバック処理
	class PushBackColliderCallBack : public CollisionCallBack
	{
		Player* m_parent;
		void OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)override;
	public:
		Vec3<float>m_prePos;
		PushBackColliderCallBack(Player* Parent) :m_parent(Parent) {}
	}m_pushBackColliderCallBack;

	//床との押し戻しコールバック処理
	class PushBackColliderCallBack_Foot : public CollisionCallBack
	{
		Player* parent;
		void OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)override;
	public:
		PushBackColliderCallBack_Foot(Player* Parent) :parent(Parent) {}
	}m_pushBackColliderCallBack_Foot;

	//移動処理
	void Move();

	//アニメーション切り替え
	void AnimationSwitch();

public:
	Player();
	~Player() { s_instanced = false; }
	void Init();
	void Update();
	void Draw(Camera& Cam);
	std::weak_ptr<ModelObject>GetModelObj() { return m_model; }
};

