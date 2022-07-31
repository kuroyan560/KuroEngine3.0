#pragma once
#include<string>
#include"PlayerCamera.h"
#include<memory>
#include<vector>
#include"PlayerAttack.h"
#include"Collider.h"
#include"PlayerStatus.h"
#include"KuroMath.h"
class ModelObject;
class Camera;
class UsersInput;
class ControllerConfig;

class Player
{
private:
	//インスタンス生成を１個に制限するためのフラグ
	static bool s_instanced;

	//プレイヤー操作カメラ（プレイヤーが作り変えられる度に、カメラを生成しなくて済むように）
	static std::unique_ptr<PlayerCamera> s_camera;

public:
	//カメラキー
	static const std::string s_cameraKey;
	//カメラポインタ
	static const std::shared_ptr<Camera>& GetCam()
	{
		if (!s_camera)s_camera = std::make_unique<PlayerCamera>();
		return s_camera->GetCam();
	}

private:
	//ステータス管理
	PlayerStatus m_statusMgr;
	//ステータスのトリガーを感知して実行する処理のまとめ
	void OnStatusTriggerUpdate(const Vec3<float>& InputMoveVec);

	//モデル
	std::shared_ptr<ModelObject>m_model;

	//コライダー
	std::vector<std::shared_ptr<Collider>>m_colliders;	//配列

	//移動量
	Vec3<float>m_move;

	//通常移動の速さ
	float m_inputMoveSpeed = 0.6f;

/*--- ジャンプ関連 ---*/
	//ジャンプ力
	float m_jumpPower = 1.0f;
	//接地フラグ
	bool m_onGround = true;
	//落下速度
	float m_fallSpeed = 0.0f;

/*--- 回避 ---*/
	//回避方向の記録用
	Vec3<float>m_dodgeMoveVec;
	//回避のイージングパラメータ
	EasingParameter m_dodgeEasingParameter;
	//回避フレーム計測用
	int m_dodgeFrame = 0;
	//回避にかかるフレーム数
	int m_dodgeFrameNum = 30;
	//回避力
	float m_dodgePower = 0.5f;

/*--- コールバック ---*/
		//押し戻し
	class PushBackColliderCallBack : public CollisionCallBack
	{
		Player* m_parent;
		void OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)override;
	public:
		Vec3<float>m_prePos;
		PushBackColliderCallBack(Player* Parent) :m_parent(Parent) {}
	}m_pushBackColliderCallBack;

	//床との押し戻し
	class PushBackColliderCallBack_Foot : public CollisionCallBack
	{
		Player* m_parent;
		void OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)override;
	public:
		PushBackColliderCallBack_Foot(Player* Parent) :m_parent(Parent) {}
	}m_pushBackColliderCallBack_Foot;

/*--- アニメーション ---*/
	enum struct ANIM_TYPE
	{
		WAIT,
		MOVE,
		RUN,
		JUMP,
		ATTACK,
		GUARD,
		DODGE,
		NUM
	};
	const std::array<std::string, static_cast<int>(ANIM_TYPE::NUM)>m_animName =
	{
		"Wait",
		"Run",
		"Run",
		"",
		"Attack_",	//攻撃アニメーションは複数あるので先頭文字列だけ
		"",
		"",
	};
	const std::string& GetAnimName(const ANIM_TYPE& Type);

/*--- その他 ---*/

	//攻撃処理クラス
	PlayerAttack m_attack;
	//１フレーム前に攻撃入力したか
	bool m_oldAttackInput;

public:
	Player();
	~Player() { s_instanced = false; }
	void Init();
	void Update(UsersInput& Input, ControllerConfig& Controller, const float& Gravity);
	void Draw(Camera& Cam);
	void DrawHUD(Camera& Cam);
	std::weak_ptr<ModelObject>GetModelObj() { return m_model; }

	//imguiデバッグ機能
	void ImguiDebug();
};