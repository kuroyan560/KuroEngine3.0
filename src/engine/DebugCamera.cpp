#include "DebugCamera.h"
#include"WinApp.h"
#include"UsersInput.h"

void DebugCamera::MoveXMVector(const XMVECTOR& MoveVector)
{
	auto pos = m_cam->GetPos();
	auto target = m_cam->GetTarget();

	Vec3<float>move(MoveVector.m128_f32[0], MoveVector.m128_f32[1], MoveVector.m128_f32[2]);
	pos += move;
	target += move;

	m_cam->SetPos(pos);
	m_cam->SetTarget(target);
}

DebugCamera::DebugCamera()
{
	m_cam = std::make_shared<Camera>("DebugCamera");
	m_dist = m_cam->GetPos().Distance(m_cam->GetTarget());

	//画面サイズに対する相対的なスケールに調整
	m_scale.x = 1.0f / (float)WinApp::Instance()->GetExpandWinSize().x;
	m_scale.y = 1.0f / (float)WinApp::Instance()->GetExpandWinSize().y;
}

void DebugCamera::Init(const Vec3<float>& InitPos, const Vec3<float>& Target)
{
	m_cam->SetPos(InitPos);
	m_cam->SetTarget(Target);

	m_dist = InitPos.Distance(Target);
}

void DebugCamera::Move()
{
	bool moveDirty = false;
	float angleX = 0;
	float angleY = 0;

	//マウスの入力を取得
	UsersInput::MouseMove mouseMove = UsersInput::Instance()->GetMouseMove();

	//マウス左クリックでカメラ回転
	if (UsersInput::Instance()->MouseInput(MOUSE_BUTTON::RIGHT))
	{
		float dy = mouseMove.m_inputX * m_scale.y;
		float dx = mouseMove.m_inputY * m_scale.x;

		angleX = -dx * XM_PI;
		angleY = -dy * XM_PI;
		moveDirty = true;
	}

	//マウス中クリックでカメラ平行移動
	if (UsersInput::Instance()->MouseInput(MOUSE_BUTTON::CENTER))
	{
		float dx = mouseMove.m_inputX / 100.0f;
		float dy = mouseMove.m_inputY / 100.0f;

		XMVECTOR move = { -dx,+dy,0,0 };
		move = XMVector3Transform(move, m_matRot);

		MoveXMVector(move);
		moveDirty = true;
	}

	//ホイール入力で距離を変更
	if (mouseMove.m_inputZ != 0)
	{
		m_dist -= mouseMove.m_inputZ / 100.0f;
		m_dist = std::max(m_dist, 1.0f);
		moveDirty = true;
	}

	if (moveDirty)
	{
		//追加回転分の回転行列を生成
		XMMATRIX matRotNew = XMMatrixIdentity();
		matRotNew *= XMMatrixRotationX(-angleX);
		matRotNew *= XMMatrixRotationY(-angleY);
		// ※回転行列を累積していくと、誤差でスケーリングがかかる危険がある為
		// クォータニオンを使用する方が望ましい
		m_matRot = matRotNew * m_matRot;

		// 注視点から視点へのベクトルと、上方向ベクトル
		XMVECTOR vTargetEye = { 0.0f, 0.0f, -m_dist, 1.0f };
		XMVECTOR vUp = { 0.0f, 1.0f, 0.0f, 0.0f };

		// ベクトルを回転
		vTargetEye = XMVector3Transform(vTargetEye, m_matRot);
		vUp = XMVector3Transform(vUp, m_matRot);

		// 注視点からずらした位置に視点座標を決定
		Vec3<float>target = m_cam->GetTarget();
		m_cam->SetPos({ target.x + vTargetEye.m128_f32[0], target.y + vTargetEye.m128_f32[1], target.z + vTargetEye.m128_f32[2] });
		m_cam->SetUp({ vUp.m128_f32[0], vUp.m128_f32[1], vUp.m128_f32[2] });
	}
}
