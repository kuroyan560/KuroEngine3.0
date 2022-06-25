#include "PlayerCamera.h"
#include"Camera.h"
#include"Transform.h"
#include"UsersInput.h"

/*
//カメラ位置高さ制限
static float HEIGHT_MIN = 0.1f;
static float HEIGHT_DEFAULT = 4.0f;
static float HEIGHT_MAX = 10.0f;

//プレイヤーとの距離制限
static float DISTANCE_MIN = 3.0f;
static float DISTANCE_MAX = 10.0f;
static float DISTANCE_DEFAULT = DISTANCE_MAX;

//実際のプレイヤーの位置とロックオンする位置の高さオフセット
static float ROCK_ON_HEIGHT_OFFSET = 2.0f;
*/

//カメラ位置高さ制限
static float HEIGHT_MIN = 3.63f;
static float HEIGHT_DEFAULT = 11.397f;
static float HEIGHT_MAX = 17.495f;

//プレイヤーとの距離制限
static float DISTANCE_MIN = 4.040f;
static float DISTANCE_MAX = 15.791f;
static float DISTANCE_DEFAULT = DISTANCE_MAX;

//実際のプレイヤーの位置とロックオンする位置のオフセット
static float TARGET_DIST_OFFSET = 7.363f;
static float TARGET_HEIGHT_OFFSET = 10.549f;

void PlayerCamera::CalculatePos(const Transform& Player)
{
	static const float LERP_RATE_MIN = 0.12f;
	static const float LERP_RATE_MAX = 0.19f;

	Vec3<float>angleVec = { cos(posAngle),0.0f,sin(posAngle) };

	//プレイヤーとカメラの距離
	auto result = Player.GetPos() + angleVec * dist;
	result.y += height;

	float rate = (dist - DISTANCE_MIN) / (DISTANCE_MAX - DISTANCE_MIN);
	float lerpRate = 0.0f;
	lerpRate = KuroMath::Lerp(LERP_RATE_MAX, LERP_RATE_MIN, rate);
	result = KuroMath::Lerp(cam->GetPos(), result, lerpRate);

	cam->SetPos(result);

	//プレイヤーをロックオン
	auto rockOnPlayerPos = Player.GetPos();
	auto forward = cam->GetForward();
	rockOnPlayerPos += forward * TARGET_DIST_OFFSET;
	rockOnPlayerPos.y += TARGET_HEIGHT_OFFSET;
	cam->SetTarget(rockOnPlayerPos);
}

PlayerCamera::PlayerCamera()
{
	cam = std::make_shared<Camera>("PlayerCamera");
}

void PlayerCamera::Init(const Transform& Player)
{
	posAngle = Angle(-90);	//プレイヤー後方
	height = HEIGHT_DEFAULT;
	dist = DISTANCE_DEFAULT;

	CalculatePos(Player);
}

void PlayerCamera::Update(const Transform& Player)
{
	//角度の変化量
	const Angle angleAmount = Angle(2 * (mirrorX ? 1 : -1));

	//上下入力の変化量
	const float verticalAmount = 0.2f * (mirrorY ? 1 : -1);

	//右スティック入力
	Vec2<float> inputVec = UsersInput::Instance()->GetRightStickVec(0);

	//右スティック入力あり
	if (!inputVec.IsZero())
	{
		//左右首振り
		posAngle += angleAmount * inputVec.x;

		//右スティック上下
		//高さが下限にあるとき上下スティック入力が距離入力になる
		if (dist < DISTANCE_MAX)
		{
			dist += verticalAmount * inputVec.y;
		}
		//高さの入力になる(縦首振り)
		else
		{
			height += verticalAmount * inputVec.y;
		}

		//超過した分の高さ入力を距離入力に変換
		if (height < HEIGHT_MIN)
		{
			dist += height - HEIGHT_MIN;
		}
		//超過した分の距離入力を高さ入力に変換
		if (DISTANCE_MAX < dist)
		{
			height += dist - DISTANCE_MAX;
		}
	}

	//クランプ
	dist = std::clamp(dist, DISTANCE_MIN, DISTANCE_MAX);
	height = std::clamp(height, HEIGHT_MIN, HEIGHT_MAX);
	posAngle.Normalize();

	//カメラ位置計算
	CalculatePos(Player);


}

#include"imguiApp.h"
void PlayerCamera::OnImguiDebug()
{
	ImGui::Begin("PlayerCamera");

	ImGui::Text("Height");
	ImGui::SliderFloat("h_min", &HEIGHT_MIN, 0.1f, HEIGHT_DEFAULT);
	ImGui::SliderFloat("h_default", &HEIGHT_DEFAULT, HEIGHT_MIN, HEIGHT_MAX);
	ImGui::SliderFloat("h_max", &HEIGHT_MAX, HEIGHT_DEFAULT, 20.0f);
	ImGui::Separator();

	ImGui::Text("Distance");
	ImGui::SliderFloat("d_min", &DISTANCE_MIN, 0.1f, DISTANCE_MAX);
	if (ImGui::SliderFloat("d_max", &DISTANCE_MAX, DISTANCE_MIN, 20.0f))DISTANCE_DEFAULT = DISTANCE_MAX;
	ImGui::Separator();

	ImGui::Text("TargetOffset");
	ImGui::SliderFloat("t_distance", &TARGET_DIST_OFFSET, 0.0f, 20.0f);
	ImGui::SliderFloat("t_height", &TARGET_HEIGHT_OFFSET, 0.0f, 20.0f);

	ImGui::End();
}
