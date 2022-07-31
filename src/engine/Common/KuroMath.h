#pragma once
#include<functional>
#include"Vec.h"
#include<vector>

#include<DirectXMath.h>
using namespace DirectX;
using Matrix = XMMATRIX;

bool operator!=(const Matrix& lhs, const Matrix& rhs);
bool operator==(const Matrix& lhs, const Matrix& rhs);

#include"Angle.h"

enum EASE_CHANGE_TYPE
{
    In, Out, InOut, EASE_CHANGE_TYPE_NUM
};
enum EASING_TYPE
{
    Quad, Cubic, Quart, Quint, Sine, Exp, Circ, Elastic, Back, Bounce, EASING_TYPE_NUM
};

class KuroMath
{
private:
    static float QuadIn(float t, float totaltime, float min, float max);
    static float QuadOut(float t, float totaltime, float min, float max);
    static float QuadInOut(float t, float totaltime, float min, float max);

    static float CubicIn(float t, float totaltime, float min, float max);
    static float CubicOut(float t, float totaltime, float min, float max);
    static float CubicInOut(float t, float totaltime, float min, float max);

    static float QuartIn(float t, float totaltime, float min, float max);
    static float QuartOut(float t, float totaltime, float min, float max);
    static float QuartInOut(float t, float totaltime, float min, float max);

    static float QuintIn(float t, float totaltime, float min, float max);
    static float QuintOut(float t, float totaltime, float min, float max);
    static float QuintInOut(float t, float totaltime, float min, float max);

    static float SineIn(float t, float totaltime, float min, float max);
    static float SineOut(float t, float totaltime, float min, float max);
    static float SineInOut(float t, float totaltime, float min, float max);

    static float ExpIn(float t, float totaltime, float min, float max);
    static float ExpOut(float t, float totaltime, float min, float max);
    static float ExpInOut(float t, float totaltime, float min, float max);

    static float CircIn(float t, float totaltime, float min, float max);
    static float CircOut(float t, float totaltime, float min, float max);
    static float CircInOut(float t, float totaltime, float min, float max);

    static float ElasticIn(float t, float totaltime, float min, float max);
    static float ElasticOut(float t, float totaltime, float min, float max);
    static float ElasticInOut(float t, float totaltime, float min, float max);

    static float BackIn(float t, float totaltime, float min, float max);
    static float BackOut(float t, float totaltime, float min, float max);
    static float BackInOut(float t, float totaltime, float min, float max);

    static float BounceIn(float t, float totaltime, float min, float max);
    static float BounceOut(float t, float totaltime, float min, float max);
    static float BounceInOut(float t, float totaltime, float min, float max);

    //ƒ‰ƒ€ƒ_®‚Ì‚½‚ß‚ÌŒ^
    using EasingFunction = std::function<float(float t, float totaltime, float min, float max)>;
    static EasingFunction easing[EASE_CHANGE_TYPE_NUM][EASING_TYPE_NUM];

    static float GetSplineSection(const float& p0, const float& p1, const float& p2, const float& p3, const float& t);
public:
    static float Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float T, float TotalTime, float Min, float Max);
    static Vec2<float> Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float T, float TotalTime, Vec2<float> Min, Vec2<float> Max);
    static Vec3<float> Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float T, float TotalTime, Vec3<float> Min, Vec3<float> Max);
    static Vec4<float> Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float T, float TotalTime, Vec4<float> Min, Vec4<float> Max);
    
    static float Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float Rate, float Min, float Max);
    static Vec2<float> Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float Rate, Vec2<float> Min, Vec2<float> Max);
    static Vec3<float> Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float Rate, Vec3<float> Min, Vec3<float> Max);
    static Vec4<float> Ease(EASE_CHANGE_TYPE EaseChangeType, EASING_TYPE EasingType, float Rate, Vec4<float> Min, Vec4<float> Max);
    
    static float Lerp(float Min, float Max, float Rate);
    static Vec2<float> Lerp(Vec2<float> Min, Vec2<float> Max, float Rate);
    static Vec3<float> Lerp(Vec3<float> Min, Vec3<float> Max, float Rate);
    static Vec4<float> Lerp(Vec4<float> Min, Vec4<float> Max, float Rate);

    static float Lerp(float Min, float Max, float T, float TotalTime)
    {
        if (T < 0)T = 0;
        if (TotalTime < T)T = TotalTime;
        return Lerp(Min, Max, T / TotalTime);
    }
    static Vec2<float> Lerp(Vec2<float> Min, Vec2<float> Max, float T, float TotalTime)
    {
        if (T < 0)T = 0;
        if (TotalTime < T)T = TotalTime;
        return Lerp(Min, Max, T / TotalTime);
    }
    static Vec3<float> Lerp(Vec3<float> Min, Vec3<float> Max, float T, float TotalTime)
    {
        if (T < 0)T = 0;
        if (TotalTime < T)T = TotalTime;
        return Lerp(Min, Max, T / TotalTime);
    }
    static Vec4<float> Lerp(Vec4<float> Min, Vec4<float> Max, float T, float TotalTime)
    {
        if (T < 0)T = 0;
        if (TotalTime < T)T = TotalTime;
        return Lerp(Min, Max, T / TotalTime);
    }

    static float GetSpline(const float& T, const int& P1Idx, const std::vector<float>& Array, bool Loop = false);
    static float GetSpline(const int& Timer, const int& TotalTime, const int& P1Idx, const std::vector<float>& Array, bool Loop = false);
    static Vec2<float> GetSpline(const float& T, const int& P1Idx, const std::vector<Vec2<float>>& Array, bool Loop = false);
    static Vec2<float> GetSpline(const int& Timer, const int& TotalTime, const int& P1Idx, const std::vector<Vec2<float>>& Array, bool Loop = false);
    static Vec3<float> GetSpline(const float& T, const int& P1Idx, const std::vector<Vec3<float>>& Array, bool Loop = false);
    static Vec3<float> GetSpline(const int& Timer, const int& TotalTime, const int& P1Idx, const std::vector<Vec3<float>>& Array, bool Loop = false);
    static Vec4<float> GetSpline(const float& T, const int& P1Idx, const std::vector<Vec4<float>>& Array, bool Loop = false);
    static Vec4<float> GetSpline(const int& Timer, const int& TotalTime, const int& P1Idx, const std::vector<Vec4<float>>& Array, bool Loop = false);

    //x,y,z‚ÌŠp“x‚©‚ç‰ñ“]s—ñ‚ğ¶¬
    static Matrix RotateMat(const Vec3<Angle>& Rotate);
    //²‚ÆŠp“x‚ğw’è‚µ‚Ä‰ñ“]s—ñ‚ğ¶¬
    static Matrix RotateMat(const Vec3<float>& Axis, const float& Radian);
    //a‚ğb‚ÉŒü‚©‚¹‚é‰ñ“]s—ñ‚ğ‹‚ß‚é
    static Matrix RotateMat(const Vec3<float>& VecA, const Vec3<float>& VecB);

    //2DƒxƒNƒgƒ‹‚ğ‰ñ“]‚³‚¹‚é
    static Vec2<float>RotateVec2(const Vec2<float>& Vec, const float& Radian);

    static Vec2<float> GetCenterVec2(Vec2<float> From, Vec2<float> To);
    static Vec3<float> GetCenterVec3(Vec3<float> From, Vec3<float> To);

    //‚Q“_‚ğ’Ê‚é’¼ü‚ÌŒX‚«
    static float GetLineSlope(Vec2<float> From, Vec2<float> To);
    //1‚Â‚ÌƒxƒNƒgƒ‹‚©‚çŠp“x‚ğæ“¾
    static Angle GetAngle(Vec2<float>Vec);
    //‚Q‚Â‚ÌƒxƒNƒgƒ‹‚©‚çŠp“x‚ğæ“¾
    static Angle GetAngle(Vec2<float>From, Vec2<float>To);

    //Vec3‚És—ñ‚ğ”½‰f
    static Vec3<float>TransformVec3(const Vec3<float>& Value, const Matrix& Mat);
    //‰ñ“]²‚ğw’è‚µ‚Ä‰ñ“]
    static Vec3<float>TransformVec3(const Vec3<float>& Value, const Vec3<float>& Axis, const Angle& Angle);
};

struct EasingParameter
{
    EASE_CHANGE_TYPE m_changeType = In;
    EASING_TYPE m_easeType = Quad;

    float Calculate(float T, float TotalTime, float Min, float Max)
    {
        return KuroMath::Ease(m_changeType, m_easeType, T, TotalTime, Min, Max);
    }
    float Calculate(float Rate, float Min, float Max)
    {
        return KuroMath::Ease(m_changeType, m_easeType, Rate, Min, Max);
    }
};