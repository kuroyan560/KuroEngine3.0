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
	//移動方向
	Vec3<float>moveVec = { 0,0,0 };

	//左スティック入力レート
	auto stickL = UsersInput::Instance()->GetLeftStickVec(0);

	//攻撃ボタン
	auto attackInput = UsersInput::Instance()->ControllerInput(0, XBOX_BUTTON::RB);

	//左スティック入力あり
	if (!stickL.IsZero())
	{
		//走り状態
		status = RUN;

		//入力方向
		moveVec = { stickL.x,0.0f,-stickL.y };

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

Player::Player() : pushBackColliderCallBack(this)
{
	KuroFunc::ErrorMessage(INSTANCED, "Player", "Constructor", "Only one Player's instance can exsit.");
	INSTANCED = true;
	model = std::make_shared<ModelObject>("resource/user/", "PrePlayer.gltf");

/*--- コライダー生成 ---*/
	//本体
	auto bodyCol_Sphere = std::make_shared<CollisionSphere>(3.0f, &model->transform);
	bodyCol_Sphere->offset = { 0,6,0 };
	auto bodyCol = Collider::Generate(bodyCol_Sphere);
	bodyCol->SetCallBack(&pushBackColliderCallBack);	//押し戻しコールバック処理をアタッチ
	colliders.emplace_back(bodyCol);

	//右手武器
	auto boneCol_R_Sphere = std::make_shared<CollisionSphere>(1.4f, &model->transform, &model->animator->GetBoneLocalMat("Hand_L"));
	boneCol_R_Sphere->offset = { 0,-0.5f,0.7f };
	auto boneCol_R = Collider::Generate(boneCol_R_Sphere);
	boneCol_R->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	colliders.emplace_back(boneCol_R);

	//左手武器
	auto boneCol_L_Sphere = std::make_shared<CollisionSphere>(1.4f, &model->transform, &model->animator->GetBoneLocalMat("Hand_R"));
	boneCol_L_Sphere->offset = { 0,-0.5f,0.7f };
	auto boneCol_L = Collider::Generate(boneCol_L_Sphere);
	boneCol_L->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	colliders.emplace_back(boneCol_L);

	//全ての自身のコライダーにプレイヤー属性を与える
	for (auto& col : colliders)
	{
		col->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	}

	//攻撃処理に武器コライダーをアタッチ
	attack.Attach(model->animator, boneCol_L, boneCol_R);
}

void Player::Init()
{
	status = RUN;
	oldStatus = RUN;

	static Vec3<float>INIT_POS = { 0,0,-5 };
	model->transform.SetPos(INIT_POS);
	model->transform.SetRotate(XMMatrixIdentity());

	CAMERA->Init(model->transform);
	model->animator->Play("Wait", true, false);

	attack.Init();
}

void Player::Update()
{
	//１フレーム前の座標をコールバック処理に記録
	pushBackColliderCallBack.prePos = model->transform.GetPos();

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

void Player::PushBackColliderCallBack::OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)
{
	//現在の座標
	Vec3<float>nowPos = parent->model->transform.GetPos();

	//押し戻し後の座標格納先
	Vec3<float>pushBackPos = nowPos;

	//自身のコライダープリミティブ（球）を取得
	auto mySphere = (CollisionSphere*)GetAttachCollider().lock()->GetColliderPrimitive().lock().get();

	//相手のコライダープリミティブを取得
	auto otherPrimitive = OtherCollider.lock()->GetColliderPrimitive().lock();
	if (otherPrimitive->GetShape() == CollisionPrimitive::SPHERE)
	{
		//相手の衝突判定用球取得
		CollisionSphere* otherSphere = (CollisionSphere*)otherPrimitive.get();

		//それぞれの中心座標
		Vec3<float>myCenter = mySphere->GetCenter();
		Vec3<float>otherCenter = otherSphere->GetCenter();

		//押し戻し方向
		Vec3<float>pushBackVec = (myCenter - otherCenter).GetNormal();
		//Vec3<float>pushBackVec = (prePos - nowPos).GetNormal();

		//押し戻し量
		float pushBackAmount = mySphere->radius + otherSphere->radius + 0.1f;
		
		//押し戻した後のコライダーの座標
		Vec3<float>pushBackColPos = otherCenter + pushBackVec * pushBackAmount;

		//押し戻し後のコライダーと現在のコライダー座標とのオフセット
		Vec3<float>colOffset = pushBackColPos - myCenter;

		//オフセットを親であるトランスフォームに適用
		pushBackPos = nowPos + colOffset;
	}
	else assert(0);	//用意していない

	//新しい座標をセット
	parent->model->transform.SetPos(pushBackPos);
}
