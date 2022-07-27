#include "PlayerCamera.h"
#include"Camera.h"
#include"Transform.h"

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

	Vec3<float>angleVec = { cos(m_posAngle),0.0f,sin(m_posAngle) };

	//プレイヤーとカメラの距離
	auto result = Player.GetPos() + angleVec * m_dist;
	result.y += m_height;

	float rate = (m_dist - DISTANCE_MIN) / (DISTANCE_MAX - DISTANCE_MIN);
	float lerpRate = 0.0f;
	lerpRate = KuroMath::Lerp(LERP_RATE_MAX, LERP_RATE_MIN, rate);
	result = KuroMath::Lerp(m_cam->GetPos(), result, lerpRate);

	m_cam->SetPos(result);

	//プレイヤーをロックオン
	auto rockOnPlayerPos = Player.GetPos();
	auto forward = m_cam->GetForward();
	rockOnPlayerPos += forward * TARGET_DIST_OFFSET;
	rockOnPlayerPos.y += TARGET_HEIGHT_OFFSET;
	m_cam->SetTarget(rockOnPlayerPos);
}

PlayerCamera::PlayerCamera()
{
	m_cam = std::make_shared<Camera>("PlayerCamera");
}

void PlayerCamera::Init(const Transform& Player)
{
	m_posAngle = Angle(-90);	//プレイヤー後方
	m_height = HEIGHT_DEFAULT;
	m_dist = DISTANCE_DEFAULT;

	CalculatePos(Player);
}

void PlayerCamera::Update(const Transform& Player, const Vec2<float>& InputVec)
{
	//角度の変化量
	const Angle angleAmount = Angle(2 * (m_mirrorX ? 1 : -1));

	//上下入力の変化量
	const float verticalAmount = 0.2f * (m_mirrorY ? 1 : -1);

	//右スティック入力
	Vec2<float> inputVec = InputVec;

	//右スティック入力あり
	if (!inputVec.IsZero())
	{
		//左右首振り
		m_posAngle += angleAmount * inputVec.x;

		//右スティック上下
		//高さが下限にあるとき上下スティック入力が距離入力になる
		if (m_dist < DISTANCE_MAX)
		{
			m_dist += verticalAmount * inputVec.y;
		}
		//高さの入力になる(縦首振り)
		else
		{
			m_height += verticalAmount * inputVec.y;
		}

		//超過した分の高さ入力を距離入力に変換
		if (m_height < HEIGHT_MIN)
		{
			m_dist += m_height - HEIGHT_MIN;
		}
		//超過した分の距離入力を高さ入力に変換
		if (DISTANCE_MAX < m_dist)
		{
			m_height += m_dist - DISTANCE_MAX;
		}
	}

	//クランプ
	m_dist = std::clamp(m_dist, DISTANCE_MIN, DISTANCE_MAX);
	m_height = std::clamp(m_height, HEIGHT_MIN, HEIGHT_MAX);
	m_posAngle.Normalize();

	//カメラ位置計算
	CalculatePos(Player);


}

#include"imguiApp.h"
void PlayerCamera::OnImguiDebug()
{
	return;
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
