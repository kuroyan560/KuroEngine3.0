#pragma once
#include"Vec.h"
#include"KuroFunc.h"
#include<vector>
class Camera;

//ロックオン / マーキング出来るポイント
class ActPoint
{
	//識別番号
	static int s_id;
	//インスタンスのポインタの静的配列
	static std::vector<ActPoint*>s_points;
public:
	static std::vector<ActPoint*>&GetActPointArray() { return s_points; }
	static void DebugDraw(Camera& Cam);

private:
	//稼働中か
	bool m_isActive;
	//ロックオン対象か
	bool m_canRockOn;
	//マーキングポイントか
	bool m_canMarking;

	//自身の座標（親がいる場合はオフセット値）
	Vec3<float>m_pos = { 0,0,0 };
	//親の座標ポインタ
	Vec3<float>* m_parent = nullptr;
public:
	const int m_id;
	ActPoint(const Vec3<float>& Pos, const bool& CanRockOn, const bool& CanMarking, Vec3<float>* Parent = nullptr, const bool& IsActive = true)
		:m_id(s_id++), m_pos(Pos), m_canRockOn(CanRockOn), m_canMarking(CanMarking), m_parent(Parent), m_isActive(IsActive)
	{
		//静的配列に登録
		s_points.emplace_back(this);
	}
	~ActPoint()
	{
		//自身を静的配列から削除
		int myId = m_id;
		std::remove_if(s_points.begin(), s_points.end(), [myId](ActPoint* p) {
			return p->m_id == myId;
			});
	}

	//セッタ
	void SetIsActive(const bool& Active) { m_isActive = Active; }

	//ゲッタ
	const bool& IsActive()const { return m_isActive; }
	const bool& IsCanRockOn()const { return m_canRockOn; }
	const bool& IsCanMarking()const { return m_canMarking; }
	Vec3<float>GetPosOn3D()const { return m_pos + (m_parent != nullptr ? *m_parent : Vec3<float>(0, 0, 0)); }
	//スクリーン座標（２D）
	Vec2<float>GetPosOn2D(const Matrix& View, const Matrix& Proj, const Vec2<float>& WinSize)const
	{
		return KuroFunc::ConvertWorldToScreen(GetPosOn3D(), View, Proj, WinSize);
	}
};

