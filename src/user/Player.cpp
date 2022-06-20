#include "Player.h"
#include"Object.h"
#include"DrawFunc3D.h"
#include"UsersInput.h"
#include"Collision.h"
#include"Collider.h"
#include"Model.h"

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
		const auto up = model->transform.GetUp();
		model->transform.SetRotate(up, KuroMath::GetAngle(stickL) + Angle(90));
		//model->transform.SetLookAtRotate(pos + moveVec);
	}

	/*static Angle ANGLE_X(0);
	const auto right = model->transform.GetRight();
	if (UsersInput::Instance()->ControllerInput(0, XBOX_BUTTON::RT))
	{
		ANGLE_X += Angle(1);
		model->transform.SetRotate(right, ANGLE_X);
	}
	else if (UsersInput::Instance()->ControllerInput(0, XBOX_BUTTON::RB))
	{
		ANGLE_X -= Angle(1);
		model->transform.SetRotate(right, ANGLE_X);
	}

	static Angle ANGLE_Z(0);
	const auto front = model->transform.GetFront();
	if (UsersInput::Instance()->ControllerInput(0, XBOX_BUTTON::LT))
	{
		ANGLE_Z += Angle(1);
		model->transform.SetRotate(front, ANGLE_Z);
	}
	else if (UsersInput::Instance()->ControllerInput(0, XBOX_BUTTON::LB))
	{
		ANGLE_Z -= Angle(1);
		model->transform.SetRotate(front, ANGLE_Z);
	}*/
}

Player::Player()
{
	KuroFunc::ErrorMessage(INSTANCED, "Player", "Constructor", "Only one Player's instance can exsit.");
	INSTANCED = true;
	model = std::make_shared<ModelObject>("resource/user/", "player.glb");

	//コライダー生成
	//メッシュ
	//for (auto& mesh : model->model->meshes)
	//{
	//	auto meshCol = std::make_shared<CollisionAABB>(mesh.GetPosMinMax(), &model->transform);
	//	colliders.emplace_back(Collider::Generate(meshCol));
	//	colliders.back()->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	//	colliders.back()->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	//}

	auto col = std::make_shared<CollisionAABB>(model->model->GetAllMeshPosMinMax(), &model->transform);
	colliders.emplace_back(Collider::Generate(col));
	colliders.back()->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	colliders.back()->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
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
