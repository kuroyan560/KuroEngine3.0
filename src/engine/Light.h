#pragma once
#include"Vec.h"
#include"Color.h"
#include"Angle.h"
#include"KuroFunc.h"
#include"KuroMath.h"

class LightManager;

//ライトデータコンテナ
namespace Light
{
	class Direction
	{
		friend class LightManager;
		static const Vec3<float>DEFAULT_DIR;
		bool dirty = true;	//LightManagerからの監視用

		class ConstData
		{
		public:
			unsigned int active = 1;
			Color color;
			Vec3<float>dir = DEFAULT_DIR;
		}constData;

	public:
		void SetColor(const Color& Color)
		{
			if (constData.color == Color)return;
			constData.color = Color;
			dirty = true;
		}
		void SetDir(const Vec3<float>& Direction)
		{
			if (constData.dir == Direction)return;
			constData.dir = Direction;
			dirty = true;
		}
		void SetDir(const Vec3<Angle>& Rotate, const Vec3<float>& Dir = DEFAULT_DIR)
		{
			const auto dir = KuroMath::TransformVec3(Dir, KuroMath::RotateMat(Rotate));
			SetDir(dir);
		}
		void SetActive(const bool& Active)
		{
			unsigned int val = Active ? 1 : 0;
			if (constData.active == val)return;
			constData.active = val;
			dirty = true;
		}
		void SetActive()
		{
			SetActive(1 - constData.active);
		}
	};

	class Point
	{
		friend class LightManager;
		bool dirty = true;	//LightManagerからの監視用

		class ConstData
		{
		public:
			unsigned int active = 1;
			Color color;
			Vec3<float>pos = { 0.0f,1.0f,0.0f };
			float influenceRange = 20.0f;
			int pad[3];
		}constData;

	public:
		const Color& GetColor() { return constData.color; }
		const Vec3<float>& GetPos() { return constData.pos; }
		const float& GetInfluenceRange() { return constData.influenceRange; }
		void SetColor(const Color& Color)
		{
			if (constData.color == Color)return;
			constData.color = Color;
			dirty = true;
		}
		void SetPos(const Vec3<float>& Pos)
		{
			if (constData.pos == Pos)return;
			constData.pos = Pos;
			dirty = true;
		}
		void SetInfluenceRange(const float& Range)
		{
			if (constData.influenceRange == Range)return;
			constData.influenceRange = Range;
			dirty = true;
		}
		void SetActive(const bool& Active)
		{
			unsigned int val = Active ? 1 : 0;
			if (constData.active == val)return;
			constData.active = val;
			dirty = true;
		}
		void SetActive()
		{
			SetActive(1 - constData.active);
		}
	};

	class Spot
	{
		friend class LightManager;
		bool dirty = true;	//LightManagerからの監視用

		class ConstData
		{
		public:
			unsigned int active = 1;
			Color color;
			Vec3<float>pos = { 0.0f,1.0f,0.0f };
			float influenceRange = 20.0f;
			Vec3<float>target = { 0.0f,0.0f,0.0f };
			Angle angle = 30.0f;
			int pad[3];
		}constData;

	public:
		void SetColor(const Color& Color)
		{
			if (constData.color == Color)return;
			constData.color = Color;
			dirty = true;
		}
		void SetPos(const Vec3<float>& Pos)
		{
			if (constData.pos == Pos)return;
			constData.pos = Pos;
			dirty = true;
		}
		void SetInfluenceRange(const float& Range)
		{
			if (constData.influenceRange == Range)return;
			constData.influenceRange = Range;
			dirty = true;
		}
		void SetTarget(const Vec3<float>& Target)
		{
			if (constData.target == Target)return;
			constData.target = Target;
			dirty = true;
		}
		void SetAngle(const Angle& Angle)
		{
			if (constData.angle == Angle)return;
			constData.angle = Angle;
			dirty = true;
		}
		void SetActive(const bool& Active)
		{
			unsigned int val = Active ? 1 : 0;
			if (constData.active == val)return;
			constData.active = val;
			dirty = true;
		}
		void SetActive()
		{
			SetActive(1 - constData.active);
		}
	};

	class HemiSphere
	{
		friend class LightManager;
		bool dirty = true;	//LightManagerからの監視用

		class ConstData
		{
		public:
			unsigned int active = 1;
			Color skyColor = Color(0.4f, 0.6f, 0.8f, 1.0f);
			Color groundColor = Color(0.15f, 0.7f, 0.95f, 1.0f);
			Vec3<float>groundNormal = { 0.0f,1.0f,0.0f };
		}constData;

	public:
		void SetSkyColor(const Color& SkyColor)
		{
			if (constData.skyColor == SkyColor)return;
			constData.skyColor = SkyColor;
			dirty = true;
		}
		void SetGroundColor(const Color& GroundColor)
		{
			if (constData.groundColor == GroundColor)return;
			constData.groundColor = GroundColor;
			dirty = true;
		}
		void SetGroundNormal(const Vec3<float>& GroundNormal)
		{
			if (constData.groundNormal == GroundNormal)return;
			constData.groundNormal = GroundNormal;
			dirty = true;
		}
		void SetActive(const bool& Active)
		{
			unsigned int val = Active ? 1 : 0;
			if (constData.active == val)return;
			constData.active = val;
			dirty = true;
		}
		void SetActive()
		{
			SetActive(1 - constData.active);
		}
	};

	enum TYPE
	{
		DIRECTION, POINT, SPOT, HEMISPHERE, TYPE_NUM
	};
}