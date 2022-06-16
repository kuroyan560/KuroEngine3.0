#include "Player.h"
#include"Object.h"
#include"DrawFunc3D.h"
#include"UsersInput.h"
#include"Collision.h"
#include"Collider.h"

bool Player::INSTANCED = false;
const std::string Player::CAMERA_KEY = "PlayerCamera";
std::unique_ptr<PlayerCamera> Player::CAMERA;
Player::CanInput Player::CAN_INPUT;

void Player::Move()
{
	//左スティック入力レート
	auto stickL = UsersInput::Instance()->GetLeftStickVec(0);

	//左スティック入力あり
	if (!stickL.IsZero())
	{
		//移動方向
		Vec3<float>moveVec = { stickL.x,0.0f,-stickL.y };

		//回転行列を適用
		//moveVec = KuroMath::TransformVec3(moveVec, model->transform.GetRotate()).GetNormal();
		//カメラ位置角度のオフセットからスティックの入力方向補正
		static const Angle ANGLE_OFFSET(-90);
		moveVec = KuroMath::TransformVec3(moveVec, KuroMath::RotateMat({ 0,1,0 }, -CAMERA->posAngle + ANGLE_OFFSET)).GetNormal();

		//移動
		const float moveSpeed = 0.3f;
		auto pos = model->transform.GetPos();
		pos += moveVec * moveSpeed;
		model->transform.SetPos(pos);

		//方向転換
		Vec3<float>front = model->transform.GetFront();
		model->transform.SetLookAtRotate(pos + moveVec);
	}
}

Player::Player()
{
	KuroFunc::ErrorMessage(INSTANCED, "Player", "Constructor", "Only one Player's instance can exsit.");
	INSTANCED = true;
	model = std::make_shared<ModelObject>("resource/user/", "player.glb");

	//コライダー生成
	const float COLLIDER_RADIUS = 3.0f;
	auto colSphere = std::make_shared<CollisionSphere>(COLLIDER_RADIUS);
	colSphere->AttachWorldTransform(&model->transform);
	collider = Collider::Generate(colSphere);
	collider->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	collider->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
}

void Player::Init()
{
	static Vec3<float>INIT_POS = { 0,0,0 };
	model->transform.SetPos(INIT_POS);
	model->transform.SetRotate(XMMatrixIdentity());

	CAMERA->Init(model->transform);
}

void Player::Update()
{
	//移動の処理
	if (CAN_INPUT.playerControl)
	{
		Move();
	}

	//プレイヤー追従カメラ更新
	if (CAN_INPUT.camControl)
	{
		CAMERA->Update(model->transform);
	}
}

void Player::Draw(Camera& Cam)
{
	DrawFunc3D::DrawNonShadingModel(model, Cam);
}
