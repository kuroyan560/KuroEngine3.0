#include "Player.h"
#include"Object.h"
#include"DrawFunc3D.h"
#include"Collision.h"
#include"Collider.h"
#include"Model.h"
#include"ModelAnimator.h"
#include"UsersInput.h"
#include"ControllerConfig.h"
#include<magic_enum.h>

bool Player::s_instanced = false;
const std::string Player::s_cameraKey = "PlayerCamera";
std::unique_ptr<PlayerCamera> Player::s_camera;
Player::CanInput Player::s_canInput;

void Player::Move(UsersInput& Input, ControllerConfig& Controller)
{
	//左スティック入力あり
	if (m_statusMgr.CompareNowStatus(PLAYER_STATUS_TAG::MOVE))
	{
		//移動方向
		Vec3<float>moveVec = { 0,0,0 };

		//左スティック入力レート
		auto stickL = Controller.GetMoveVec(Input);

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
}

void Player::AnimationSwitch()
{
	if (m_statusMgr.StatusTrigger(PLAYER_STATUS_TAG::WAIT))	//待機モーション
	{
		m_model->m_animator->speed = 1.0f;
		m_model->m_animator->Play("Wait", true, false);
	}
	else if (m_statusMgr.StatusTrigger(PLAYER_STATUS_TAG::MOVE))	//移動モーション
	{
		m_model->m_animator->speed = 1.5f;
		m_model->m_animator->Play("Run", true, false);
	}
	else if (m_statusMgr.StatusTrigger(PLAYER_STATUS_TAG::ATTACK))	//攻撃モーション
	{
		m_model->m_animator->speed = 1.0f;
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

	//通常攻撃用コライダー
	auto nrmAttackCol_Sphere = std::make_shared<CollisionSphere>(3.0f, &m_model->m_transform);
	nrmAttackCol_Sphere->m_offset = { 0,6.0f,6.0f };
	auto nrmAttackCol = Collider::Generate(nrmAttackCol_Sphere);
	nrmAttackCol->SetMyAttribute(COLLIDER_ATTRIBUTE::PLAYER);
	nrmAttackCol->SetHitCheckAttribute(COLLIDER_ATTRIBUTE::ENEMY);
	m_colliders.emplace_back(nrmAttackCol);

	//攻撃処理に武器コライダーをアタッチ
	m_attack.Attach(m_model->m_animator, nrmAttackCol);
}

void Player::Init()
{
	m_statusMgr.Init(PLAYER_STATUS_TAG::WAIT);

	static Vec3<float>INIT_POS = { 0,0,-5 };
	m_model->m_transform.SetPos(INIT_POS);
	m_model->m_transform.SetRotate(XMMatrixIdentity());

	s_camera->Init(m_model->m_transform);
	m_model->m_animator->Play("Wait", true, false);

	m_attack.Init();
}

void Player::Update(UsersInput& Input, ControllerConfig& Controller)
{
	PlayerParameterForStatus infoForStatus;
	infoForStatus.m_markingNum = 0;
	infoForStatus.m_maxMarking = false;
	infoForStatus.m_onGround = true;
	infoForStatus.m_attackFinish = m_attack.IsActive();
	infoForStatus.m_dodgeFinish = true;
	infoForStatus.m_rushFinish = true;
	infoForStatus.m_abilityFinish = true;

	//ステータスの更新
	m_statusMgr.Update(Input, Controller, infoForStatus);

	//移動の処理
	if (s_canInput.m_playerControl)
	{
		Move(Input, Controller);
	}

	//プレイヤー追従カメラ更新
	if (s_canInput.m_camControl)
	{
		s_camera->Update(m_model->m_transform, Controller.GetCameraVec(Input));
	}

	//攻撃処理更新
	m_attack.Update();
	if (!m_statusMgr.CompareNowStatus(PLAYER_STATUS_TAG::ATTACK))m_attack.Stop();
	
	//アニメーション切り替え
	AnimationSwitch();

	//アニメーション更新
	m_model->m_animator->Update();
}

void Player::Draw(Camera& Cam)
{
	DrawFunc3D::DrawNonShadingModel(m_model, Cam);
}

#include"imguiApp.h"
void Player::ImguiDebug()
{
	static std::string s_nowStatusName = std::string(magic_enum::enum_name(m_statusMgr.GetNowStatus()));
	static std::string s_beforeStatusName = s_nowStatusName;

	if (m_statusMgr.StatusTrigger())
	{
		s_beforeStatusName = s_nowStatusName;
		s_nowStatusName = std::string(magic_enum::enum_name(m_statusMgr.GetNowStatus()));
	}

	ImGui::Begin("Player");
	ImGui::Text("NowStatus : { %s }", s_nowStatusName.c_str());
	ImGui::Text("BeforeStatus : { %s }", s_beforeStatusName.c_str());
	ImGui::End();
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