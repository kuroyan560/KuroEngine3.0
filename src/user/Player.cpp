#include "Player.h"
#include"Object.h"
#include"DrawFunc3D.h"
#include"UsersInput.h"
#include"Collision.h"
#include"Collider.h"
#include"Model.h"
#include"ModelAnimator.h"

bool Player::s_instanced = false;
const std::string Player::s_cameraKey = "PlayerCamera";
std::unique_ptr<PlayerCamera> Player::s_camera;
Player::CanInput Player::s_canInput;

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
		m_status = RUN;

		//入力方向
		moveVec = { stickL.x,0.0f,-stickL.y };

		//回転行列を適用
		//moveVec = KuroMath::TransformVec3(moveVec, model->transform.GetRotate()).GetNormal();
		//カメラ位置角度のオフセットからスティックの入力方向補正
		static const Angle ANGLE_OFFSET(-90);
		moveVec = KuroMath::TransformVec3(moveVec, KuroMath::RotateMat({ 0,1,0 }, -s_camera->m_posAngle + ANGLE_OFFSET)).GetNormal();

		//移動
		const float moveSpeed = 0.6f;
		auto pos = m_model->m_transform.GetPos();
		pos += moveVec * moveSpeed;
		m_model->m_transform.SetPos(pos);

		//方向転換
		const auto up = m_model->m_transform.GetUp();
		m_model->m_transform.SetLookAtRotate(pos + moveVec);
	}
	else if (attackInput)
	{
		//攻撃状態
		m_status = ATTACK;
	}
	else
	{
		//待機状態
		m_status = WAIT;
	}
}

void Player::AnimationSwitch()
{
	if (StatusTrigger(WAIT))	//待機モーション
	{
		m_model->m_animator->speed = 1.0f;
		m_model->m_animator->Play("Wait", true, false);
	}
	else if (StatusTrigger(RUN))	//走りモーション
	{
		m_model->m_animator->speed = 1.5f;
		m_model->m_animator->Play("Run", true, false);
	}
	else if (StatusTrigger(ATTACK))	//攻撃モーション
	{
		m_model->m_animator->speed = 3.0f;
		m_attack.Start();	//攻撃処理開始
	}
}

Player::Player() : m_pushBackColliderCallBack(this), m_pushBackColliderCallBack_Foot(this)
{
	assert(!s_instanced);
	s_instanced = true;
	m_model = std::make_shared<ModelObject>("resource/user/", "PrePlayer.gltf");

/*--- コライダー生成 ---*/
	//本体
	auto bodyCol_Sphere = std::make_shared<CollisionSphere>(3.0f, &m_model->m_transform);
	bodyCol_Sphere->m_offset = { 0,6,0 };
	auto bodyCol = Collider::Generate(bodyCol_Sphere);
	bodyCol->SetCallBack(&m_pushBackColliderCallBack);	//押し戻しコールバック処理をアタッチ
	bodyCol->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	m_colliders.emplace_back(bodyCol);

	//床用足元
	auto footCol_Point = std::make_shared<CollisionPoint>(Vec3<float>(), &m_model->m_transform);
	auto footCol = Collider::Generate(footCol_Point);
	footCol->SetCallBack(&m_pushBackColliderCallBack_Foot);	//床との押し戻しコールバック処理をアタッチ
	footCol->SetMyAttribute(COLLIDER_ATTRIBUTE::FOOT_POINT);
	footCol->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::FLOOR);
	m_colliders.emplace_back(footCol);

	//右手武器
	auto boneCol_R_Sphere = std::make_shared<CollisionSphere>(1.4f, &m_model->m_transform, &m_model->m_animator->GetBoneLocalMat("Hand_L"));
	boneCol_R_Sphere->m_offset = { 0,-0.5f,0.7f };
	auto boneCol_R = Collider::Generate(boneCol_R_Sphere);
	boneCol_R->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	boneCol_R->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	m_colliders.emplace_back(boneCol_R);

	//左手武器
	auto boneCol_L_Sphere = std::make_shared<CollisionSphere>(1.4f, &m_model->m_transform, &m_model->m_animator->GetBoneLocalMat("Hand_R"));
	boneCol_L_Sphere->m_offset = { 0,-0.5f,0.7f };
	auto boneCol_L = Collider::Generate(boneCol_L_Sphere);
	boneCol_L->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	boneCol_L->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	m_colliders.emplace_back(boneCol_L);

	//攻撃処理に武器コライダーをアタッチ
	m_attack.Attach(m_model->m_animator, boneCol_L, boneCol_R);
}

void Player::Init()
{
	m_status = RUN;
	m_oldStatus = RUN;

	static Vec3<float>INIT_POS = { 0,0,-5 };
	m_model->m_transform.SetPos(INIT_POS);
	m_model->m_transform.SetRotate(XMMatrixIdentity());

	s_camera->Init(m_model->m_transform);
	m_model->m_animator->Play("Wait", true, false);

	m_attack.Init();
}

void Player::Update()
{
	//移動の処理
	if (s_canInput.m_playerControl)
	{
		Move();
	}

	//プレイヤー追従カメラ更新
	if (s_canInput.m_camControl)
	{
		s_camera->Update(m_model->m_transform);
	}

	//攻撃処理更新
	m_attack.Update();
	if (m_status != ATTACK)m_attack.Stop();
	
	//アニメーション切り替え
	AnimationSwitch();

	//アニメーション更新
	m_model->m_animator->Update();

	//ステータスを記録
	m_oldStatus = m_status;
}

void Player::Draw(Camera& Cam)
{
	DrawFunc3D::DrawNonShadingModel(m_model, Cam);
}

void Player::PushBackColliderCallBack::OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)
{
	//現在の座標
	Vec3<float>nowPos = m_parent->m_model->m_transform.GetPos();

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
		float pushBackAmount = mySphere->m_radius + otherSphere->m_radius + 0.1f;
		
		//押し戻した後のコライダーの座標
		Vec3<float>pushBackColPos = otherCenter + pushBackVec * pushBackAmount;

		//押し戻し後のコライダーと現在のコライダー座標とのオフセット
		Vec3<float>colOffset = pushBackColPos - myCenter;

		//オフセットを親であるトランスフォームに適用
		pushBackPos = nowPos + colOffset;
	}
	else assert(0);	//用意していない

	//新しい座標をセット
	m_parent->m_model->m_transform.SetPos(pushBackPos);
}

void Player::PushBackColliderCallBack_Foot::OnCollision(const Vec3<float>& Inter, std::weak_ptr<Collider> OtherCollider)
{
	//現在の座標
	Vec3<float>nowPos = parent->m_model->m_transform.GetPos();

	//床の高さ取得
	float floorY = Inter.y;

	//自身のコライダープリミティブ（点）を取得
	auto myPt = (CollisionPoint*)GetAttachCollider().lock()->GetColliderPrimitive().lock().get();

	//押し戻し後の座標格納先
	Vec3<float>pushBackPos = nowPos;
	pushBackPos.y += floorY - myPt->GetWorldPos().y;	//押し戻し

	//新しい座標をセット
	parent->m_model->m_transform.SetPos(pushBackPos);
}