#pragma once
#include<string>
#include"Vec.h"
#include"Angle.h"
#include"KuroMath.h"
#include<memory>

namespace KuroFunc
{
	std::wstring GetWideStrFromStr(const std::string& Str);
	std::wstring GetWideStrFromStr(const char* Str);

	std::string GetExtension(const std::string& Path);

	void GetDivideStr(const std::string& Str, std::string* Dir, std::string* FileName);

	Vec2<float> ConvertWorldToScreen(Vec3<float> WorldPos,
		const Matrix& ViewMat, const Matrix& ProjMat, const Vec2<int>&WinSize);
	Vec3<float> ConvertScreenToWorld(Vec2<float> ScreenPos, float Z,
		const Matrix& ViewMat, const Matrix& ProjMat, const Vec2<int>&WinSize);

	//中心座標が画面内に映っているか
	bool InScreen(Vec3<float> WorldPos, const Matrix& ViewMat, const Matrix& ProjMat, const Vec2<int>& WinSize);

	bool ExistFile(const std::string FilePass);

	//ベジエ用
	float GetYFromXOnBezier(float x, const Vec2<float>& a, const Vec2<float>& b, uint8_t n);
	//ベジエ(制御点指定)
	float GetBezierFromControlPoint(float t, int ControlPointNum, float* ControlPointArray);

	//乱数
	int GetRand(int Min, int Max);
	int GetRand(int Max);
	float GetRand(float Min, float Max);
	Vec2<float> GetRand(Vec2<float> Min, Vec2<float> Max);
	Vec3<float> GetRand(Vec3<float> Min, Vec3<float> Max);
	float GetRand(float Max);
	Vec2<float> GetRand(Vec2<float> Max);
	Vec3<float> GetRand(Vec3<float> Max);
	//正負ランダム
	int GetRandPlusMinus();
	//基準の値から前後乱数算出
	int GetRandFromCenter(int CenterNum, int Range);


	//指定した桁の数字を取り出す
	int GetSpecifiedDigitNum(int From, int Digit);
	//桁数を算出
	int GetDigit(int Num);

	//符号取得
	int GetNumSign(int Num);
	int GetNumSign(float Num);

	//データ読み込み
	void LoadData(FILE* Fp, std::string DataName, void* Data, size_t Size, int ElementNum);
	//データ保存
	void SaveData(FILE* Fp, std::string DataName, const void* Data, size_t Size, int ElementNum);


}