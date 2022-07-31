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

void Player::MoveByInput(UsersInput& Input, ControllerConfig& Controller)
{
	//左スティック入力レート
	auto stickL = Controller.GetMoveVec(Input);
	if (stickL.IsZero())return;	//入力なし

	//移動方向
	Vec3<float>moveVec = { stickL.x,0.0f,-stickL.y };

	//カメラ位置角度のオフセットからスティックの入力方向補正
	static const Angle ANGLE_OFFSET(-90);
	moveVec = KuroMath::TransformVec3(moveVec, KuroMath::RotateMat({ 0,1,0 }, -s_camera->GetPosAngle() + ANGLE_OFFSET)).GetNormal();

	//移動
	m_move += moveVec * m_inputMoveSpeed;
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
	//ステータス初期化
	m_statusMgr.Init(PLAYER_STATUS_TAG::WAIT);

	//初期位置と向き
	static Vec3<float>INIT_POS = { 0,0,-5 };
	m_model->m_transform.SetPos(INIT_POS);
	m_model->m_transform.SetRotate(XMMatrixIdentity());

	//カメラ初期化
	s_camera->Init(m_model->m_transform);

	//待機アニメーション
	m_model->m_animator->Play("Wait", true, false);

	//接地フラグON
	m_onGround = true;

	//攻撃処理初期化
	m_attack.Init();
	m_oldAttackInput = false;
}

void Player::Update(UsersInput& Input, ControllerConfig& Controller, const float& Gravity)
{
	//移動量リセット
	m_move = { 0,0,0 };

	PlayerParameterForStatus infoForStatus;
	infoForStatus.m_markingNum = 0;
	infoForStatus.m_maxMarking = false;
	infoForStatus.m_onGround = m_onGround;
	infoForStatus.m_attackFinish = !m_attack.IsActive();
	infoForStatus.m_dodgeFinish = true;
	infoForStatus.m_rushFinish = true;
	infoForStatus.m_abilityFinish = true;

	//ステータスの更新
	m_statusMgr.Update(Input, Controller, infoForStatus);

	//ステータスのトリガーを感知して、処理を呼び出す
	if (m_statusMgr.StatusTrigger(PLAYER_STATUS_TAG::WAIT))	//待機
	{
		//待機アニメーション
		m_model->m_animator->speed = 1.0f;
		m_model->m_animator->Play("Wait", true, false);
	}
	else if (m_statusMgr.StatusTrigger(PLAYER_STATUS_TAG::MOVE))	//移動
	{
		//移動アニメーション
		m_model->m_animator->speed = 1.5f;
		m_model->m_animator->Play("Run", true, false);
	}
	else if (m_statusMgr.StatusTrigger(PLAYER_STATUS_TAG::JUMP))	//ジャンプ
	{
		//接地フラグOFF
		m_onGround = false;

		//ジャンプ
		m_fallSpeed = m_jumpPower;
	}

	//攻撃状態
	if (m_statusMgr.CompareNowStatus(PLAYER_STATUS_TAG::ATTACK))
	{
		//連続攻撃の入力
		bool attackInput = Controller.GetHandleInput(Input, HANDLE_INPUT_TAG::ATTACK);
		if (!m_oldAttackInput && attackInput)
		{
			//攻撃処理開始
			m_model->m_animator->speed = 1.0f;
			//攻撃の処理はPlayerAttack内で処理
			m_attack.Attack();
		}
		//トリガー判定用に過去の入力として記録
		m_oldAttackInput = attackInput;
	}


	//無操作状態でないとき
	if (!m_statusMgr.CompareNowStatus(PLAYER_STATUS_TAG::OUT_OF_CONTROL))
	{
		//入力による移動の処理
		MoveByInput(Input, Controller);

		//ロックオン
		if (Controller.GetCameraRock(Input))s_camera->RockOn(m_model->m_transform);

		//プレイヤー追従カメラ更新
		s_camera->Update(m_model->m_transform, Controller.GetCameraVec(Input));
	}

	//攻撃処理更新
	m_attack.Update();
	//攻撃ステータスでない（攻撃が中断された場合）
	if (!m_statusMgr.CompareNowStatus(PLAYER_STATUS_TAG::ATTACK))m_attack.Stop();

	//アニメーション更新
	m_model->m_animator->Update();

	//攻撃の勢いを移動量に加算
	auto front = m_model->m_transform.GetFront();
	m_move += front * m_attack.GetMomentum();

	//落下
	m_move.y += m_fallSpeed;
	m_fallSpeed += Gravity;

	//移動量を反映させる
	auto playerPos = m_model->m_transform.GetPos();
	playerPos += m_move;
	m_model->m_transform.SetPos(playerPos);

	//方向転換（XZ平面）
	if (m_move.x || m_move.z)
	{
		const auto up = m_model->m_transform.GetUp();
		m_model->m_transform.SetLookAtRotate(playerPos + Vec3<float>(m_move.x, 0.0f, m_move.z));
	}
}

void Player::Draw(Camera& Cam)
{
	DrawFunc3D::DrawNonShadingModel(m_model, Cam);
}

void Player::DrawHUD(Camera& Cam)
{
	s_camera->Draw(Cam);
}

#include"imguiApp.h"
void Player::ImguiDebug()
{
	ImGui::Begin("Player");

/*--- 性能調整 ---*/
	//ジャンプ力
	ImGui::InputFloat("JumpPower", &m_jumpPower);

	ImGui::Separator();

/*--- パラメータ表示 ---*/
	//落下速度
	ImGui::Text("m_fallSpeed : { %f }", m_fallSpeed);

	ImGui::Separator();
/*--- ステータス表示 ---*/
	//ステータス名取得
	static std::string s_nowStatusName = std::string(magic_enum::enum_name(m_statusMgr.GetNowStatus()));
	static std::string s_beforeStatusName = s_nowStatusName;

	//ステータス切り替えトリガー判定取得
	if (m_statusMgr.StatusTrigger())
	{
		s_beforeStatusName = s_nowStatusName;
		s_nowStatusName = std::string(magic_enum::enum_name(m_statusMgr.GetNowStatus()));
	}

	//表示
	ImGui::Text("NowStatus : { %s }", s_nowStatusName.c_str());
	ImGui::Text("BeforeStatus : { %s }", s_beforeStatusName.c_str());

	ImGui::End();

/*--- プレイやー攻撃機構 ---*/
	m_attack.ImguiDebug();
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
	Vec3<float>nowPos = m_parent->m_model->m_transform.GetPos();

	//床の高さ取得
	float floorY = Inter.y;

	//自身のコライダープリミティブ（点）を取得
	auto myPt = (CollisionPoint*)GetAttachCollider().lock()->GetColliderPrimitive().lock().get();

	//押し戻し後の座標格納先
	Vec3<float>pushBackPos = nowPos;
	pushBackPos.y += floorY - myPt->GetWorldPos().y;	//押し戻し

	//新しい座標をセット
	m_parent->m_model->m_transform.SetPos(pushBackPos);

	//接地フラグON
	m_parent->m_onGround = true;

	//落下速度リセット
	m_parent->m_fallSpeed = 0.0f;
}