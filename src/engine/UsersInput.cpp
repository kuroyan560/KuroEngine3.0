#include "UsersInput.h"
#include <cassert>
#include<DirectXMath.h>
#include"KuroFunc.h"

#pragma comment(lib,"xinput.lib")

#define STICK_INPUT_MAX 32768.0f

UsersInput* UsersInput::s_instance = nullptr;

bool UsersInput::StickInDeadZone(Vec2<float>& Thumb, const Vec2<float>& DeadRate)const
{
	bool x = false, y = false;
	if ((Thumb.x < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * DeadRate.x
		&& Thumb.x > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * DeadRate.x))
	{
		Thumb.x = 0.0f;
		x = true;
	}
	if((Thumb.y < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * DeadRate.y
			&& Thumb.y > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * DeadRate.y))
	{
		Thumb.y = 0.0f;
		y = true;
	}
	if (x && y)return true;
	return false;
}

void UsersInput::Initialize(const WNDCLASSEX& WinClass, const HWND& Hwnd)
{
	HRESULT hr;
	//DirectInputオブジェクトの生成
	if (FAILED(hr = DirectInput8Create(
		WinClass.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_dinput, nullptr)))assert(0);

	//キーボードデバイスの生成
	if (FAILED(hr = m_dinput->CreateDevice(GUID_SysKeyboard, &m_keyDev, NULL)))assert(0);
	if (FAILED(hr = m_keyDev->SetDataFormat(&c_dfDIKeyboard)))assert(0);	//標準形式
	if (FAILED(hr = m_keyDev->SetCooperativeLevel(		//排他制御レベルのセット
		Hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE/* | DISCL_NOWINKEY*/)))assert(0);

	//マウスデバイスの生成
	if (FAILED(hr = m_dinput->CreateDevice(GUID_SysMouse, &m_mouseDev, NULL)))assert(0);
	if (FAILED(hr = m_mouseDev->SetDataFormat(&c_dfDIMouse2)))assert(0);
	if (FAILED(hr = m_mouseDev->SetCooperativeLevel(
		Hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY)))assert(0);

	/*
		使っているフラグについて
		DISCL_FOREGROUND	画面が手前にある場合のみ入力を受け付ける
		DISCL_NONEXCLUSIVE	デバイスをこのアプリだけで専有しない
		DISCL_NOWINKEY		Windowsキーを無効にする
	*/
}

void UsersInput::Update(const HWND& Hwnd, const Vec2<float>& WinSize)
{
	//キーボード
	memcpy(m_oldkeys, m_keys, s_keyNum);
	//キーボード情報の取得開始
	HRESULT result = m_keyDev->Acquire();	//本当は一回でいいが、アプリが裏面から全面に戻る度呼び出す必要があるため、毎フレーム呼び出す。
	//全キーの入力状態を取得する
	result = m_keyDev->GetDeviceState(sizeof(m_keys), m_keys);

	//マウス
	result = m_mouseDev->Acquire();
	m_oldMouseState = m_mouseState;
	result = m_mouseDev->GetDeviceState(sizeof(m_mouseState), &m_mouseState);

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(Hwnd, &p);
	m_mousePos.x = static_cast<float>(p.x);
	m_mousePos.x = std::clamp(m_mousePos.x, 0.0f, WinSize.x);
	m_mousePos.y = static_cast<float>(p.y);
	m_mousePos.y = std::clamp(m_mousePos.y, 0.0f, WinSize.y);

	//コントローラー
	for (int i = 0; i < s_controllerNum; ++i)
	{
		m_oldXinputState[i] = m_xinputState[i];
		ZeroMemory(&m_xinputState[i], sizeof(XINPUT_STATE));

		DWORD dwResult = XInputGetState(i, &m_xinputState[i]);

		//コントローラーが接続されていない
		if (dwResult != ERROR_SUCCESS)continue;

		//コントローラーが接続されている
		if (0 < m_shakeTimer[i])
		{
			m_shakeTimer[i]--;
			XINPUT_VIBRATION vibration;
			ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

			if (m_shakeTimer[i] == 0)
			{
				vibration.wLeftMotorSpeed = static_cast<WORD>(0.0f); // use any value between 0-65535 here
				vibration.wRightMotorSpeed = static_cast<WORD>(0.0f); // use any value between 0-65535 here
			}
			else
			{
				vibration.wLeftMotorSpeed = static_cast<WORD>(65535.0f * m_shakePower[i]); // use any value between 0-65535 here
				vibration.wRightMotorSpeed = static_cast<WORD>(65535.0f * m_shakePower[i]); // use any value between 0-65535 here
			}
			XInputSetState(dwResult, &vibration);
		}
	}
}

bool UsersInput::KeyOnTrigger(int KeyCode)const
{
	return (!m_oldkeys[KeyCode] && m_keys[KeyCode]);
}

bool UsersInput::KeyInput(int KeyCode)const
{
	return m_keys[KeyCode];
}

bool UsersInput::KeyOffTrigger(int KeyCode)const
{
	return (m_oldkeys[KeyCode] && !m_keys[KeyCode]);
}

bool UsersInput::MouseOnTrigger(MOUSE_BUTTON Button)const
{
	return (!m_oldMouseState.rgbButtons[Button] && m_mouseState.rgbButtons[Button]);
}

bool UsersInput::MouseInput(MOUSE_BUTTON Button)const
{
	return m_mouseState.rgbButtons[Button];
}

bool UsersInput::MouseOffTrigger(MOUSE_BUTTON Button)const
{
	return (m_oldMouseState.rgbButtons[Button] && !m_mouseState.rgbButtons[Button]);
}

UsersInput::MouseMove UsersInput::GetMouseMove()const
{
	MouseMove tmp;
	tmp.m_inputX = m_mouseState.lX;
	tmp.m_inputY = m_mouseState.lY;
	tmp.m_inputZ = m_mouseState.lZ;
	return tmp;
}

//#include"Object.h"
//#include"WinApp.h"
//Ray UsersInput::GetMouseRay()
//{
//	Ray mouseRay;
//	Vec3<float> start = MyFunc::ConvertScreenToWorld(mousePos, 0.0f,
//	Camera::GetNowCam()->MatView(),
//		Camera::GetNowCam()->MatProjection(),
//		App::GetWinApp().GetWinSize());
//	mouseRay.start = { start.x,start.y,start.z };
//
//	Vec3<float> to = MyFunc::ConvertScreenToWorld(mousePos, 1.0f,
//		Camera::GetNowCam()->MatView(),
//		Camera::GetNowCam()->MatProjection(),
//		App::GetWinApp().GetWinSize());
//	Vec3<float> vec(to - start);
//	mouseRay.dir = vec.GetNormal();
//
//	return mouseRay;
//}

bool UsersInput::ControllerOnTrigger(const int& ControllerIdx, XBOX_BUTTON Button)const
{
	//トリガー
	if (Button == LT)
	{
		return m_oldXinputState[ControllerIdx].Gamepad.bLeftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD
			&& ControllerInput(ControllerIdx, Button);
	}
	else if (Button == RT)
	{
		return m_oldXinputState[ControllerIdx].Gamepad.bRightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD
			&& ControllerInput(ControllerIdx, Button);
	}
	else
	{
		return !(m_oldXinputState[ControllerIdx].Gamepad.wButtons & Button)
			&& ControllerInput(ControllerIdx, Button);
	}
	assert(0);
	return false;
}

bool UsersInput::ControllerOnTrigger(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange, const Vec2<float>& DeadRate)const
{
	Vec2<float>oldVec;
	Vec2<float>vec;
	bool isLeftStick = StickInput <= L_RIGHT;
	if (isLeftStick)
	{
		oldVec = Vec2<float>(m_oldXinputState[ControllerIdx].Gamepad.sThumbLX, m_oldXinputState[ControllerIdx].Gamepad.sThumbLY);
		vec = Vec2<float>(m_xinputState[ControllerIdx].Gamepad.sThumbLX, m_xinputState[ControllerIdx].Gamepad.sThumbLY);
	}
	else
	{
		oldVec = Vec2<float>(m_oldXinputState[ControllerIdx].Gamepad.sThumbRX, m_oldXinputState[ControllerIdx].Gamepad.sThumbRY);
		vec = Vec2<float>(m_xinputState[ControllerIdx].Gamepad.sThumbRX, m_xinputState[ControllerIdx].Gamepad.sThumbRY);
	}

	if (!StickInDeadZone(oldVec, DeadRate))return false;
	if (StickInDeadZone(vec, DeadRate))return false;

	bool result = false;

	if (StickInput % 4 == L_UP)
	{
		result = !(DeadRange < (oldVec.y / STICK_INPUT_MAX))
			&& DeadRange < (vec.y / STICK_INPUT_MAX);
	}
	else if (StickInput % 4 == L_DOWN)
	{
		result = !(oldVec.y / STICK_INPUT_MAX < -DeadRange)
			&& vec.y / STICK_INPUT_MAX < -DeadRange;
	}
	else if (StickInput % 4 == L_RIGHT)
	{
		result = !(DeadRange < (oldVec.x / STICK_INPUT_MAX))
			&& DeadRange < (vec.x / STICK_INPUT_MAX);
	}
	else if (StickInput % 4 == L_LEFT)
	{
		result =  !(oldVec.x / STICK_INPUT_MAX < -DeadRange)
			&& vec.x / STICK_INPUT_MAX < -DeadRange;
	}
	else
	{
		assert(0);
	}

	return result;
}

bool UsersInput::ControllerInput(const int& ControllerIdx, XBOX_BUTTON Button)const
{
	if (Button == LT)
	{
		return XINPUT_GAMEPAD_TRIGGER_THRESHOLD < m_xinputState[ControllerIdx].Gamepad.bLeftTrigger;
	}
	else if (Button == RT)
	{
		return XINPUT_GAMEPAD_TRIGGER_THRESHOLD < m_xinputState[ControllerIdx].Gamepad.bRightTrigger;
	}
	else
	{
		return m_xinputState[ControllerIdx].Gamepad.wButtons & Button;
	}
	assert(0);
	return false;
}

bool UsersInput::ControllerInput(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange, const Vec2<float>& DeadRate)const
{
	Vec2<float>vec;
	bool isLeftStick = StickInput <= L_RIGHT;
	if (isLeftStick)
	{
		vec = Vec2<float>(m_xinputState[ControllerIdx].Gamepad.sThumbLX, m_xinputState[ControllerIdx].Gamepad.sThumbLY);
	}
	else
	{
		vec = Vec2<float>(m_xinputState[ControllerIdx].Gamepad.sThumbRX, m_xinputState[ControllerIdx].Gamepad.sThumbRY);
	}

	if (StickInDeadZone(vec, DeadRate))return false;

	if (StickInput % 4 == L_UP)
	{
		return DeadRange < (vec.y / STICK_INPUT_MAX);
	}
	else if (StickInput % 4 == L_DOWN)
	{
		return  vec.y / STICK_INPUT_MAX < -DeadRange;
	}
	else if (StickInput % 4 == L_RIGHT)
	{
		return DeadRange < (vec.x / STICK_INPUT_MAX);
	}
	else if (StickInput % 4 == L_LEFT)
	{
		return  vec.x / STICK_INPUT_MAX < -DeadRange;
	}

	assert(0);
	return false;
}

bool UsersInput::ControllerOffTrigger(const int& ControllerIdx, XBOX_BUTTON Button)const
{
	//トリガー
	if (Button == LT)
	{
		return XINPUT_GAMEPAD_TRIGGER_THRESHOLD < m_oldXinputState[ControllerIdx].Gamepad.bLeftTrigger
			&& !ControllerInput(ControllerIdx, Button);
	}
	else if (Button == RT)
	{
		return XINPUT_GAMEPAD_TRIGGER_THRESHOLD < m_oldXinputState[ControllerIdx].Gamepad.bRightTrigger
			&& !ControllerInput(ControllerIdx, Button);
	}
	//ボタン
	else
	{
		return (m_oldXinputState[ControllerIdx].Gamepad.wButtons & Button)
			&& !ControllerInput(ControllerIdx, Button);
	}
	assert(0);
	return false;
}

bool UsersInput::ControllerOffTrigger(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange, const Vec2<float>& DeadRate)const
{
	Vec2<float>oldVec;
	Vec2<float>vec;
	bool isLeftStick = StickInput <= L_RIGHT;
	if (isLeftStick)
	{
		oldVec = Vec2<float>(m_oldXinputState[ControllerIdx].Gamepad.sThumbLX, m_oldXinputState[ControllerIdx].Gamepad.sThumbLY);
		vec = Vec2<float>(m_xinputState[ControllerIdx].Gamepad.sThumbLX, m_xinputState[ControllerIdx].Gamepad.sThumbLY);
	}
	else
	{
		oldVec = Vec2<float>(m_oldXinputState[ControllerIdx].Gamepad.sThumbRX, m_oldXinputState[ControllerIdx].Gamepad.sThumbRY);
		vec = Vec2<float>(m_xinputState[ControllerIdx].Gamepad.sThumbRX, m_xinputState[ControllerIdx].Gamepad.sThumbRY);
	}

	if (!StickInDeadZone(oldVec, DeadRate))return false;
	if (StickInDeadZone(vec, DeadRate))return false;

	bool result = false;
	if (StickInput % 4 == L_UP)
	{
		result = DeadRange < (oldVec.y / STICK_INPUT_MAX)
			&& !(DeadRange < (vec.y / STICK_INPUT_MAX));
	}
	else if (StickInput % 4 == L_DOWN)
	{
		result = oldVec.y / STICK_INPUT_MAX < -DeadRange
			&& !(vec.y / STICK_INPUT_MAX < -DeadRange);
	}
	else if (StickInput % 4 == L_RIGHT)
	{
		result = DeadRange < (oldVec.x / STICK_INPUT_MAX)
			&& !(DeadRange < (vec.x / STICK_INPUT_MAX));
	}
	else if (StickInput % 4 == L_LEFT)
	{
		result = oldVec.x / STICK_INPUT_MAX < -DeadRange
			&& !(vec.x / STICK_INPUT_MAX < -DeadRange);
	}
	else
	{
		assert(0);
	}
	if (result)
	{
	}
	return result;
}

Vec2<float> UsersInput::GetLeftStickVec(const int& ControllerIdx, const Vec2<float>& DeadRate)const
{
	Vec2<float>result(static_cast<float>(m_xinputState[ControllerIdx].Gamepad.sThumbLX), static_cast<float>(-m_xinputState[ControllerIdx].Gamepad.sThumbLY));
	StickInDeadZone(result, DeadRate);
	return result.GetNormal();
}

Vec2<float> UsersInput::GetRightStickVec(const int& ControllerIdx, const Vec2<float>& DeadRate)const
{
	Vec2<float>result(static_cast<float>(m_xinputState[ControllerIdx].Gamepad.sThumbRX), static_cast<float>(-m_xinputState[ControllerIdx].Gamepad.sThumbRY));
	StickInDeadZone(result, DeadRate);
	return result.GetNormal();
}

void UsersInput::ShakeController(const int& ControllerIdx, const float& Power, const int& Span)
{
	if (!(0 < Power && Power <= 1.0f))assert(0);
	m_shakePower[ControllerIdx] = Power;
	m_shakeTimer[ControllerIdx] = Span;
}
