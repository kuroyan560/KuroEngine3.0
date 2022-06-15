#pragma once
#undef min
#undef max

#ifndef VEC_H
#define VEC_H

#include<DirectXMath.h>
#include<type_traits>
#include<algorithm>

template<typename T>
struct Vec2
{
	T x = 0;
	T y = 0;

	Vec2() {};
	Vec2(T X, T Y) :x(X), y(Y) {};
	float Length()const {
		return sqrt(pow(x, 2) + pow(y, 2));
	};
	float Distance(const Vec2& To)const {
		return sqrt(pow(To.x - x, 2) + pow(To.y - y, 2));
	};
	Vec2<float> GetNormal()const {
		float len = Length();
		if (len == 0.0f)return Vec2<float>(0.0, 0.0);
		return Vec2<float>(x / len, y / len);
	};
	void Normalize() {
		float len = Length();
		x /= len;
		y /= len;
	};
	DirectX::XMFLOAT2 ConvertXMFLOAT2() {
		return DirectX::XMFLOAT2(x, y);
	};
	Vec2<int>Int()const { return Vec2<int>(x, y); }
	Vec2<float>Float()const { return Vec2<float>((float)x, (float)y); }

	float Dot(const Vec2<float>& rhs)const {
		auto me = Float();
		return me.x * rhs.x + me.y * rhs.y;
	}
	float Cross(const Vec2<float>& rhs)const {
		auto me = Float();
		return me.x * rhs.y - me.y * rhs.x;
	}
	bool IsZero()const { return x == 0 && y == 0; }

#pragma region オペレーター演算子
	Vec2 operator-() const {
		return Vec2(-x, -y);
	}
	Vec2 operator+(const Vec2& rhs) const {
		return Vec2(x + rhs.x, y + rhs.y);
	};
	Vec2 operator-(const Vec2& rhs)const {
		return Vec2(x - rhs.x, y - rhs.y);
	};
	Vec2 operator*(const Vec2& rhs)const {
		return Vec2(x * rhs.x, y * rhs.y);
	};
	Vec2 operator*(const float& rhs)const {
		return Vec2(x * rhs, y * rhs);
	};
	Vec2 operator/(const Vec2& rhs)const {
		return Vec2(x / rhs.x, y / rhs.y);
	};
	Vec2 operator/(const float& rhs)const {
		return Vec2(x / rhs, y / rhs);
	};
	Vec2 operator%(const Vec2& rhs) const {
		return Vec2(fmodf(x, rhs.x), fmodf(y, rhs.y));
	};
	void operator=(const Vec2& rhs) {
		x = rhs.x;
		y = rhs.y;
	};
	bool operator==(const Vec2& rhs)const {
		return (x == rhs.x && y == rhs.y);
	};
	bool operator!=(const Vec2& rhs)const {
		return !(*this == rhs);
	};
	void operator+=(const Vec2& rhs) {
		x += rhs.x;
		y += rhs.y;
	};
	void operator-=(const Vec2& rhs) {
		x -= rhs.x;
		y -= rhs.y;
	};
	void operator*=(const Vec2& rhs) {
		x *= rhs.x;
		y *= rhs.y;
	};
	void operator/=(const Vec2& rhs) {
		x /= rhs.x;
		y /= rhs.y;
	};
	void operator%=(const Vec2& rhs) {
		x = fmodf(x, rhs.x);
		y = fmodf(y, rhs.y);
	};

	void operator+=(const float& rhs) {
		x += rhs;
		y += rhs;
	};
	void operator-=(const float& rhs) {
		x -= rhs;
		y -= rhs;
	};
	void operator*=(const float& rhs) {
		x *= rhs;
		y *= rhs;
	};
	void operator/=(const float& rhs) {
		x /= rhs;
		y /= rhs;
	};
	void operator%=(const float& rhs) {
		x = fmodf(x, rhs);
		y = fmodf(y, rhs);
	};
#pragma endregion
};

template<typename T>
struct Vec3
{
	T x = 0;
	T y = 0;
	T z = 0;

	Vec3() {};
	Vec3(T X, T Y, T Z) :x(X), y(Y), z(Z) {};
	float Length()const {
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
	};
	Vec3(Vec2<T>XY, T Z) :x(XY.x), y(XY.y), z(Z) {};
	float Distance(const Vec3& To)const {
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		return sqrt(pow(To.x - x, 2) + pow(To.y - y, 2) + pow(To.z - z, 2));
	};
	float DistanceSq(const Vec3& To)const {
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		return pow(To.x - x, 2) + pow(To.y - y, 2) + pow(To.z - z, 2);
	};
	Vec3<float> GetNormal()const {
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		float len = Length();
		if (len == 0.0f)return Vec3(0, 0, 0);
		return Vec3<float>(x / len, y / len, z / len);
	};
	void Normalize() {
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		float len = Length();
		if (len == 0.0f)
		{
			assert(0);
		}
		x /= len;
		y /= len;
		z /= len;
	};
	//中間地点取得
	Vec3<float>GetCenter(const Vec3& To)const {
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		const float distHalf = this->Distance(To) / 2.0f;
		Vec3<float> vec = To - *this;
		return *this + vec.GetNormal() * distHalf;
	}
	Vec3<int>Int()const 
	{ 
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		return Vec3<int>(x, y, z); 
	}
	Vec3<float>Float()const 
	{
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		return Vec3<float>((float)x, (float)y, (float)z); 
	}


	float Dot(const Vec3<float>& rhs)const{
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		auto me = Float();
		return me.x * rhs.x + me.y * rhs.y + me.z * rhs.z;
	}
	Vec3<float>Cross(const Vec3<float>& rhs)const {
		static_assert(std::is_arithmetic<T>::value, "template parameter T must be arithmetic");
		auto me = Float();
		return Vec3<float>(
			me.y * rhs.z - rhs.y * me.z,
			me.z * rhs.x - rhs.z * me.x,
			me.x * rhs.y - rhs.x * me.y);
	}
	bool IsZero() 
	{
		return x == 0 && y == 0 && z == 0;
	}
	void Max(const Vec3<T>&rhs)
	{
		x = std::max(x, rhs.x);
		y = std::max(y, rhs.y);
		z = std::max(z, rhs.z);
	}
	void Min(const Vec3<T>& rhs)
	{
		x = std::min(x, rhs.x);
		y = std::min(y, rhs.y);
		z = std::min(z, rhs.z);
	}

#pragma region オペレーター演算子
	Vec3 operator-()const {
		return Vec3(-x, -y, -z);
	}
	Vec3 operator+(const Vec3& rhs)const {
		return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
	};
	Vec3 operator-(const Vec3& rhs)const {
		return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
	};
	Vec3 operator*(const Vec3& rhs) const {
		return Vec3(x * rhs.x, y * rhs.y, z * rhs.z);
	};
	Vec3 operator*(const float& rhs)const {
		return Vec3(x * rhs, y * rhs, z * rhs);
	};
	Vec3 operator/(const Vec3& rhs)const {
		return Vec3(x / rhs.x, y / rhs.y, z / rhs.z);
	};
	Vec3 operator/(const float& rhs)const {
		return Vec3(x / rhs, y / rhs, z / rhs);
	};
	Vec3 operator%(const Vec3& rhs)const {
		return Vec3(fmodf(x, rhs.x), fmodf(y, rhs.y), fmodf(z, rhs.z));
	};
	void operator=(const Vec3& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
	};
	bool operator==(const Vec3& rhs)const {
		return (x == rhs.x && y == rhs.y && z == rhs.z);
	};
	bool operator!=(const Vec3& rhs)const {
		return !(*this == rhs);
	};
	void operator+=(const Vec3& rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
	};
	void operator+=(const float& rhs) {
		x += rhs;
		y += rhs;
		z += rhs;
	};
	void operator-=(const Vec3& rhs) {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
	};
	void operator-=(const float& rhs) {
		x -= rhs;
		y -= rhs;
		z -= rhs;
	};
	void operator*=(const Vec3& rhs) {
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
	};
	void operator*=(const float& rhs) {
		x *= rhs;
		y *= rhs;
		z *= rhs;
	};
	void operator/=(const Vec3& rhs) {
		x /= rhs.x;
		y /= rhs.y;
		z /= rhs.z;
	};
	void operator/=(const float& rhs) {
		x /= rhs;
		y /= rhs;
		z /= rhs;
	};
	void operator%=(const Vec3& rhs) {
		x = fmodf(x, rhs.x);
		y = fmodf(y, rhs.y);
		z = fmodf(z, rhs.z);
	};
	T& operator[](const int& Idx)
	{
		if (Idx == 0)return this->x;
		if (Idx == 1)return this->y;
		if (Idx == 2)return this->z;
		assert(0);
	}
	operator DirectX::XMFLOAT3()const
	{
		static_assert(std::is_floating_point<T>::value, "template parameter T must be floating type");
		return DirectX::XMFLOAT3(x, y, z);
	}
	operator DirectX::XMVECTOR() const
	{
		static_assert(std::is_floating_point<T>::value, "template parameter T must be floating type");
		return DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)this);
	}
	operator DirectX::XMFLOAT3*()
	{
		static_assert(std::is_floating_point<T>::value, "template parameter T must be floating type");
		return (DirectX::XMFLOAT3*)this;
	}

#pragma endregion
};

template<typename T>
struct Vec4
{
	T x = 0;
	T y = 0;
	T z = 0;
	T w = 0;

	Vec4() {}
	Vec4(T X, T Y, T Z, T W) :x(X), y(Y), z(Z), w(W) {}

	DirectX::XMFLOAT4 ConvertXMFLOAT4() {
		return DirectX::XMFLOAT3(x, y, z, w);
	}
	Vec4<int>Int() { return Vec4<int>(x, y, z, w); }
	Vec4<float>Float() { return Vec4<float>((float)x, (float)y, (float)z, (float)w); }

#pragma region オペレーター演算子
	Vec4 operator-() {
		return Vec4(-x, -y, -z, -w);
	}
	Vec4 operator+(const Vec4& rhs) {
		return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}
	Vec4 operator-(const Vec4& rhs) {
		return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
	}
	Vec4 operator*(const Vec4& rhs) {
		return Vec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
	}
	Vec4 operator*(const float& rhs) {
		return Vec4(x * rhs, y * rhs, z * rhs, w * rhs);
	}
	Vec4 operator/(const Vec4& rhs) {
		return Vec4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
	}
	Vec4 operator%(const Vec4& rhs) {
		return Vec4(fmodf(x, rhs.x), fmodf(y, rhs.y), fmodf(z, rhs.z), fmodf(w, rhs.w));
	}
	void operator=(const Vec4& rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
	}
	bool operator==(const Vec4& rhs) {
		return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
	}
	bool operator!=(const Vec4& rhs)const {
		return !(*this == rhs);
	};
	void operator+=(const Vec4& rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
	}
	void operator-=(const Vec4& rhs) {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
	}
	void operator*=(const Vec4& rhs) {
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
		w *= rhs.w;
	}
	void operator/=(const Vec4& rhs) {
		x /= rhs.x;
		y /= rhs.y;
		z /= rhs.z;
		w /= rhs.w;
	}
	void operator%=(const Vec4& rhs) {
		x = fmodf(x, rhs.x);
		y = fmodf(y, rhs.y);
		z = fmodf(z, rhs.z);
		w = fmodf(w, rhs.w);
	}
	T& operator[](int idx) {
		return *(&x + idx);
	}
#pragma endregion
};

#endif