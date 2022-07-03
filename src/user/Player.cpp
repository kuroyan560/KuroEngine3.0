#include "Player.h"
#include"Object.h"
#include"DrawFunc3D.h"
#include"UsersInput.h"
#include"Collision.h"
#include"Collider.h"
#include"Model.h"
#include"ModelAnimator.h"

bool Player::INSTANCED = false;
const std::string Player::CAMERA_KEY = "PlayerCamera";
std::unique_ptr<PlayerCamera> Player::CAMERA;
Player::CanInput Player::CAN_INPUT;

void Player::Move()
{
	//左スティック入力レート
	auto stickL = UsersInput::Instance()->GetLeftStickVec(0);

	//攻撃ボタン
	auto attackInput = UsersInput::Instance()->ControllerInput(0, XBOX_BUTTON::RB);

	//左スティック入力あり
	if (!stickL.IsZero())
	{
		//走り状態
		status = RUN;

		//移動方向
		Vec3<float>moveVec = { stickL.x,0.0f,-stickL.y };

		//回転行列を適用
		//moveVec = KuroMath::TransformVec3(moveVec, model->transform.GetRotate()).GetNormal();
		//カメラ位置角度のオフセットからスティックの入力方向補正
		static const Angle ANGLE_OFFSET(-90);
		moveVec = KuroMath::TransformVec3(moveVec, KuroMath::RotateMat({ 0,1,0 }, -CAMERA->posAngle + ANGLE_OFFSET)).GetNormal();

		//移動
		const float moveSpeed = 0.6f;
		auto pos = model->transform.GetPos();
		pos += moveVec * moveSpeed;
		model->transform.SetPos(pos);

		//方向転換
		const auto up = model->transform.GetUp();
		model->transform.SetLookAtRotate(pos + moveVec);
	}
	else if (attackInput)
	{
		//攻撃状態
		status = ATTACK;
	}
	else
	{
		//待機状態
		status = WAIT;
	}
}

void Player::AnimationSwitch()
{
	if (StatusTrigger(WAIT))	//待機モーション
	{
		model->animator->speed = 1.0f;
		model->animator->Play("Wait", true, false);
	}
	else if (StatusTrigger(RUN))	//走りモーション
	{
		model->animator->speed = 1.5f;
		model->animator->Play("Run", true, false);
	}
	else if (StatusTrigger(ATTACK))	//攻撃モーション
	{
		model->animator->speed = 3.0f;
		attack.Start();	//攻撃処理開始
	}
}

Player::Player()
{
	KuroFunc::ErrorMessage(INSTANCED, "Player", "Constructor", "Only one Player's instance can exsit.");
	INSTANCED = true;
	model = std::make_shared<ModelObject>("resource/user/", "PrePlayer.gltf");

	//コライダー生成
	//メッシュ
	//for (auto& mesh : model->model->meshes)
	//{
	//	auto meshCol = std::make_shared<CollisionAABB>(mesh.GetPosMinMax(), &model->transform);
	//	colliders.emplace_back(Collider::Generate(meshCol));
	//	colliders.back()->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	//	colliders.back()->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	//}

	auto boneCol_R_Sphere = std::make_shared<CollisionSphere>(1.4f, &model->transform, &model->animator->GetBoneLocalMat("Hand_L"));
	boneCol_R_Sphere->offset = { 0,-0.5f,0.7f };
	auto boneCol_R = Collider::Generate(boneCol_R_Sphere);
	colliders.emplace_back(boneCol_R);

	auto boneCol_L_Sphere = std::make_shared<CollisionSphere>(1.4f, &model->transform, &model->animator->GetBoneLocalMat("Hand_R"));
	boneCol_L_Sphere->offset = { 0,-0.5f,0.7f };
	auto boneCol_L = Collider::Generate(boneCol_L_Sphere);
	colliders.emplace_back(boneCol_L);

	for (auto& col : colliders)
	{
		col->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
		col->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	}

	attack.Attach(model->animator, boneCol_L, boneCol_R);
}

void Player::Init()
{
	status = RUN;
	oldStatus = RUN;

	static Vec3<float>INIT_POS = { 0,0,0 };
	model->transform.SetPos(INIT_POS);
	model->transform.SetRotate(XMMatrixIdentity());

	CAMERA->Init(model->transform);
	model->animator->Play("Wait", true, false);

	attack.Init();
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

	//攻撃処理更新
	attack.Update();
	if (status != ATTACK)attack.Stop();
	
	//アニメーション切り替え
	AnimationSwitch();

	//アニメーション更新
	model->animator->Update();

	//ステータスを記録
	oldStatus = status;
}

void Player::Draw(Camera& Cam)
{
	DrawFunc3D::DrawNonShadingModel(model, Cam);
}
