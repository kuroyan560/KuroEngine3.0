#include "DebugCamera.h"
#include"WinApp.h"
#include"UsersInput.h"

void DebugCamera::MoveXMVector(const XMVECTOR& MoveVector)
{
	auto pos = cam->GetPos();
	auto target = cam->GetTarget();

	Vec3<float>move(MoveVector.m128_f32[0], MoveVector.m128_f32[1], MoveVector.m128_f32[2]);
	pos += move;
	target += move;

	cam->SetPos(pos);
	cam->SetTarget(target);
}

DebugCamera::DebugCamera()
{
	cam = std::make_shared<Camera>("DebugCamera");
	dist = cam->GetPos().Distance(cam->GetTarget());

	//画面サイズに対する相対的なスケールに調整
	scale.x = 1.0f / (float)WinApp::Instance()->GetExpandWinSize().x;
	scale.y = 1.0f / (float)WinApp::Instance()->GetExpandWinSize().y;
}

void DebugCamera::Init(const Vec3<float>& InitPos, const Vec3<float>& Target)
{
	cam->SetPos(InitPos);
	cam->SetTarget(Target);

	dist = InitPos.Distance(Target);
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
		float dy = mouseMove.IX * scale.y;
		float dx = mouseMove.IY * scale.x;

		angleX = -dx * XM_PI;
		angleY = -dy * XM_PI;
		moveDirty = true;
	}

	//マウス中クリックでカメラ平行移動
	if (UsersInput::Instance()->MouseInput(MOUSE_BUTTON::CENTER))
	{
		float dx = mouseMove.IX / 100.0f;
		float dy = mouseMove.IY / 100.0f;

		XMVECTOR move = { -dx,+dy,0,0 };
		move = XMVector3Transform(move, matRot);

		MoveXMVector(move);
		moveDirty = true;
	}

	//ホイール入力で距離を変更
	if (mouseMove.IZ != 0)
	{
		dist -= mouseMove.IZ / 100.0f;
		dist = std::max(dist, 1.0f);
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
		matRot = matRotNew * matRot;

		// 注視点から視点へのベクトルと、上方向ベクトル
		XMVECTOR vTargetEye = { 0.0f, 0.0f, -dist, 1.0f };
		XMVECTOR vUp = { 0.0f, 1.0f, 0.0f, 0.0f };

		// ベクトルを回転
		vTargetEye = XMVector3Transform(vTargetEye, matRot);
		vUp = XMVector3Transform(vUp, matRot);

		// 注視点からずらした位置に視点座標を決定
		Vec3<float>target = cam->GetTarget();
		cam->SetPos({ target.x + vTargetEye.m128_f32[0], target.y + vTargetEye.m128_f32[1], target.z + vTargetEye.m128_f32[2] });
		cam->SetUp({ vUp.m128_f32[0], vUp.m128_f32[1], vUp.m128_f32[2] });
	}
}
