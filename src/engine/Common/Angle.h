#pragma once

struct Angle
{
	static const float PI() { return 3.14159265359f; }
	static const float RADIAN_PER_DEGREE() { return  PI() / 180.0f; }
	static const float ROUND() { return PI() * 2; }
	static const float ConvertToRadian(const float& Degree) { return PI() / 180.0f * Degree; }
	static const int ConvertToDegree(const float& Radian) { return static_cast<int>(Radian / (PI() / 180.0f)); }

	float m_radian = 0.0f;

	Angle() {}
	Angle(const int& Degree) { SetByDegree(Degree); }
	Angle(const float& Radian) :m_radian(Radian) {}

	void SetByDegree(const int& Degree) { m_radian = Degree * RADIAN_PER_DEGREE(); }
	float GetDegree() { return (m_radian / RADIAN_PER_DEGREE()); }
	float GetNormalize()const {
		float result = m_radian;
		if (result < 0)result += ROUND();
		if (ROUND() <= result)result -= ROUND();
		return result;
	}
	float Normalize() {
			//0~PI*2
			while (m_radian < 0)m_radian += ROUND();
			while (ROUND() <= m_radian)m_radian -= ROUND();
		return m_radian;
	}

	operator const float& ()const
	{
		return m_radian;
	}

	Angle operator-()const {
		return Angle(-m_radian);
	}

	//‘ã“ü‰‰ŽZŽq
	void operator=(const float& rhs) {
		m_radian = rhs;
	}
	void operator=(const int& rhs) {
		SetByDegree(rhs);
	}

	//Žl‘¥‰‰ŽZ(ƒ‰ƒWƒAƒ“E“x)
	void operator+=(const float& rhs) {
		m_radian += rhs;
	};
	void operator+=(const int& rhs) {
		m_radian += rhs * RADIAN_PER_DEGREE();
	}
	void operator-=(const float& rhs) {
		m_radian -= rhs;
	};
	void operator-=(const int& rhs) {
		m_radian -= rhs * RADIAN_PER_DEGREE();
	}
	float* operator&() {
		return &m_radian;
	}

	Angle operator+(Angle rhs)const {
		return Angle(m_radian + rhs.m_radian);
	}
	Angle operator-(Angle rhs)const {
		return Angle(m_radian - rhs.m_radian);
	}
	void operator+=(Angle rhs) {
		*this = *this + rhs;
	}
	void operator-=(Angle rhs) {
		*this = *this - rhs;
	}

	//”äŠr
	bool operator==(const Angle& rhs)const {
		return (m_radian == rhs.m_radian);
	};
	bool operator!=(const Angle& rhs)const {
		return !(*this == rhs);
	};
	bool operator<(Angle rhs)const {
		return m_radian < rhs.m_radian;
	}
	bool operator>(Angle rhs)const {
		return rhs.m_radian < m_radian;
	}
};