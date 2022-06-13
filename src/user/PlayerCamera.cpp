#include "PlayerCamera.h"
#include"Camera.h"
#include"Transform.h"
#include"UsersInput.h"

//カメラ位置高さ制限
static const float HEIGHT_MIN = 0.1f;
static const int HEIGHT_DEFAULT = 4.0f;
static const float HEIGHT_MAX = 10.0f;

//プレイヤーとの距離制限
static const float DISTANCE_MIN = 3.0f;
static const float DISTANCE_MAX = 10.0f;
static const float DISTANCE_DEFAULT = DISTANCE_MAX;

//実際のプレイヤーの位置とロックオンする位置の高さオフセット
static const float ROCK_ON_HEIGHT_OFFSET = 2.0f;

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
	lerpRate = KuroMath::Liner(LERP_RATE_MAX, LERP_RATE_MIN, rate);
	result = KuroMath::Lerp(cam->GetPos(), result, lerpRate);

	cam->SetPos(result);

	//プレイヤーをロックオン
	auto rockOnPlayerPos = Player.GetPos();
	rockOnPlayerPos.y += ROCK_ON_HEIGHT_OFFSET;
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
