#include"KuroFunc.h"
#include<windows.h>
#include <cassert>

#include<fstream>
#include<iostream>

bool operator!=(const Matrix& lhs, const Matrix& rhs)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (lhs.r[i].m128_f32[j] != rhs.r[i].m128_f32[j])return true;
		}
	}
	return false;
}

bool operator==(const Matrix& lhs, const Matrix& rhs)
{
	return !(lhs != rhs);
}

void KuroFunc::ErrorMessage(const bool& Fail, const std::string& ClassName, const std::string& FuncName, const std::string& Comment)
{
	if (Fail)
	{
		printf("ERROR - %s - %s : %s", ClassName.c_str(), FuncName.c_str(), Comment.c_str());
		assert(0);
	}
}

std::wstring KuroFunc::GetWideStrFromStr(const std::string& Str)
{
	auto num1 = MultiByteToWideChar(
		CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		Str.c_str(), -1, nullptr, 0);

	std::wstring wstr;
	wstr.resize(num1);

	auto num2 = MultiByteToWideChar(
		CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		Str.c_str(), -1, &wstr[0], num1);

	assert(num1 == num2);

	return wstr;
}

std::wstring KuroFunc::GetWideStrFromStr(const char* Str)
{
	return GetWideStrFromStr(std::string(Str));
}

std::string KuroFunc::GetExtension(const std::string& Path)
{
	int idx = static_cast<int>(Path.rfind('.'));
	return Path.substr(idx + 1, Path.length() - idx - 1);
}

void KuroFunc::GetDivideStr(const std::string& Str, std::string* Dir, std::string* FileName)
{
	int idx = static_cast<int>(Str.rfind('/'));
	std::string fileName = Str.substr(idx + 1, Str.length() - idx - 1);
	if (FileName != nullptr)
	{
		*FileName = fileName;
	}
	if (Dir != nullptr)
	{
		*Dir = Str;
		for (int i = 0; i < fileName.size(); ++i)Dir->pop_back();
	}
}


Vec2<float> KuroFunc::ConvertWorldToScreen(Vec3<float> WorldPos, const Matrix& ViewMat, const Matrix& ProjMat, const Vec2<int>& WinSize)
{
	XMVECTOR pos = XMLoadFloat3((XMFLOAT3*)&WorldPos);
	//ビュー行列適応
	pos = XMVector3Transform(pos, ViewMat);
	//プロジェクション行列適応
	pos = XMVector3TransformCoord(pos, ProjMat);

	//ビューポート行列
	Matrix viewPortMat = XMMatrixIdentity();
	viewPortMat.r[0].m128_f32[0] = (float)WinSize.x / 2.0f;
	viewPortMat.r[1].m128_f32[1] = (float)-WinSize.y / 2.0f;
	viewPortMat.r[3].m128_f32[0] = (float)WinSize.x / 2.0f;
	viewPortMat.r[3].m128_f32[1] = (float)WinSize.y / 2.0f;
	//ビューポート行列適応
	pos = XMVector3Transform(pos, viewPortMat);

	Vec2<float> result;
	result.x = pos.m128_f32[0];
	result.y = pos.m128_f32[1];
	//result.z = pos.m128_f32[2];

	return result;
}

Vec3<float> KuroFunc::ConvertScreenToWorld(Vec2<float> ScreenPos, float Z, const Matrix& ViewMat, const Matrix& ProjMat, const Vec2<int>& WinSize)
{
	Matrix invView, invPrj, invViewPort;
	invView = XMMatrixInverse(nullptr, ViewMat);
	invPrj = XMMatrixInverse(nullptr, ProjMat);

	invViewPort = XMMatrixIdentity();
	invViewPort.r[0].m128_f32[0] = (float)WinSize.x / 2.0f;
	invViewPort.r[1].m128_f32[1] = (float)-WinSize.y / 2.0f;
	invViewPort.r[3].m128_f32[0] = (float)WinSize.x / 2.0f;
	invViewPort.r[3].m128_f32[1] = (float)WinSize.y / 2.0f;
	invViewPort = XMMatrixInverse(nullptr, invViewPort);

	XMVECTOR pos;
	auto screenPosXMFLOAT3 = XMFLOAT3(ScreenPos.x, ScreenPos.y, Z);
	pos = XMLoadFloat3(&screenPosXMFLOAT3);
	/*
		※授業資料05_03.射影変換　透視投影変換を参照
		z(0.0f ~ 1.0f)
		0.0f で proj のnear面
		1.0f で far面の位置
	*/
	pos = XMVector3Transform(pos, invViewPort);
	pos = XMVector3TransformCoord(pos, invPrj);
	//Coordでw除算を行ってくれる
	/*
		w除算をすることによって遠くのものがすぼまっていき、
		画面内に収まるようにしてくれる
		Coordがないとスクリーンから見切れた、みたいになる
	*/
	pos = XMVector3Transform(pos, invView);

	Vec3<float> result;
	result.x = pos.m128_f32[0];
	result.y = pos.m128_f32[1];
	result.z = pos.m128_f32[2];
	//result.x = base.r[0].m128_f32[0];
	//result.y = base.r[0].m128_f32[1];

	return result;
}

bool KuroFunc::InScreen(Vec3<float> WorldPos, const Matrix& ViewMat, const Matrix& ProjMat, const Vec2<int>& WinSize)
{
	Vec2<float> center = KuroFunc::ConvertWorldToScreen(WorldPos, ViewMat, ProjMat, WinSize);
	if (center.x < 0)return false;
	if (WinSize.x < center.x)return false;
	if (center.y < 0)return false;
	if (WinSize.y < center.y)return false;
	return true;
}

bool KuroFunc::ExistFile(const std::string FilePass)
{
	std::ifstream ifs(FilePass);
	return ifs.is_open();
}

float KuroFunc::GetYFromXOnBezier(float x, const Vec2<float>& a, const Vec2<float>& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x;		//計算不要
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;	//t^3の係数
	const float k1 = 3 * b.x - 6 * a.x;	//t^2の係数
	const float k2 = 3 * a.x;	//tの係数

	constexpr float epsilon = 0.0005f;	//誤差許容範囲

	//tを近似で求める
	for (int i = 0; i < n; i++)
	{
		//f(t)を求める
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;
		//もし結果が０に近い（誤差の範囲内）なら打ち切る
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}
		t -= ft / 2;	//刻む
	}

	//求めたいtは既に求めているのでyを計算
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

//階乗を求める
int GetFactorial(int num)
{
	int result = 1;
	for (int i = num; 0 < i; --i)
	{
		result *= i;
	}
	return result;
}

//n個の中からk個を選ぶときの組み合わせの数
int GetSelectPatternNum(int n, int k)
{
	if (k == 0)return 1;

	//分子
	int upper = GetFactorial(n);
	//分母
	int a = GetFactorial(k);
	int b = GetFactorial(n - k);
	int lower = a * b;

	return upper / lower;
}

float KuroFunc::GetBezierFromControlPoint(float t, int ControlPointNum, float* ControlPointArray)
{
	double result = 0;
	for (int i = 0; i < ControlPointNum; ++i)
	{
		int coefficient = GetSelectPatternNum(ControlPointNum, i);	//二項係数
		double a = pow(t, i);	// t の i 乗
		double b = pow((1 - t), (ControlPointNum - i));	// (1-t) の n-i 乗
		result += coefficient * a * b * ControlPointArray[i];		//制御点の重み付けをして加算(Σ)
	}
	return static_cast<float>(result);
}

int KuroFunc::GetRand(int Min, int Max)
{
	int result = (Max - Min + 1) * rand() / RAND_MAX + Min;
	return result;
}

int KuroFunc::GetRand(int Max)
{
	return GetRand(0, Max);
}

float KuroFunc::GetRand(float Min, float Max)
{
	double result = (Max - Min) * ((double)rand() / RAND_MAX) + Min;
	return static_cast<float>(result);
}

Vec2<float> KuroFunc::GetRand(Vec2<float> Min, Vec2<float> Max)
{
	return Vec2<float>(GetRand(Min.x, Max.x), GetRand(Min.y, Max.y));
}

Vec3<float> KuroFunc::GetRand(Vec3<float> Min, Vec3<float> Max)
{
	return Vec3<float>(GetRand(Min.x, Max.x), GetRand(Min.y, Max.y), GetRand(Min.z, Max.z));
}

float KuroFunc::GetRand(float Max)
{
	return GetRand(0.0f, Max);
}

int KuroFunc::GetRandPlusMinus()
{
	int r = GetRand(0, 1);
	if (r)return 1;
	else return -1;
}

int KuroFunc::GetRandFromCenter(int CenterNum, int Range)
{
	return CenterNum + GetRand(0, Range) * GetRandPlusMinus();
}


int KuroFunc::GetSpecifiedDigitNum(int From, int Digit)
{
	int result = 0;
	for (int i = Digit; 0 <= i; i--)
	{
		result = From % 10;				//6-i桁目の数字を記録
		From /= 10;					//桁を減らす
	}
	return result;
}

int KuroFunc::GetDigit(int Num)
{
	if (Num == 0)return 1;

	int result = 0;
	while (Num != 0)
	{
		Num /= 10;
		result++;
	}
	return result;
}

int KuroFunc::GetNumSign(int Num)
{
	if (Num == 0)return 0;
	if (Num < 0)return -1;
	return 1;
}

int KuroFunc::GetNumSign(float Num)
{
	if (Num == 0.0f)return 0;
	if (Num < 0.0f)return -1;
	return 1;
}

void KuroFunc::LoadData(FILE* Fp, std::string DataName, void* Data, size_t Size, int ElementNum)
{
	if (fread(Data, Size, ElementNum, Fp) < 1)
	{
		printf("　%sの読み込みに失敗\n", DataName.c_str());
		assert(0);
	}
}

void KuroFunc::SaveData(FILE* Fp, std::string DataName, const void* Data, size_t Size, int ElementNum)
{
	if (fwrite(Data, Size, ElementNum, Fp) < 1)
	{
		printf("　%sの書き込みに失敗\n", DataName.c_str());
		assert(0);
	}
}

