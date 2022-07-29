#pragma once
#include"Vec.h"
#include<stdio.h>

#include <dinput.h>
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

#include<wrl.h>
#include <Xinput.h>

enum MOUSE_BUTTON { LEFT, RIGHT, CENTER };
enum XBOX_BUTTON {
	B = XINPUT_GAMEPAD_B,
	A = XINPUT_GAMEPAD_A,
	X = XINPUT_GAMEPAD_X,
	Y = XINPUT_GAMEPAD_Y,
	START = XINPUT_GAMEPAD_START,
	BACK = XINPUT_GAMEPAD_BACK,
	LB = XINPUT_GAMEPAD_LEFT_SHOULDER,
	RB = XINPUT_GAMEPAD_RIGHT_SHOULDER,
	DPAD_UP = XINPUT_GAMEPAD_DPAD_UP,
	DPAD_DOWN = XINPUT_GAMEPAD_DPAD_DOWN,
	DPAD_LEFT = XINPUT_GAMEPAD_DPAD_LEFT,
	DPAD_RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT,
	R_STICK_PUSH = XINPUT_GAMEPAD_RIGHT_THUMB,
	L_STICK_PUSH = XINPUT_GAMEPAD_LEFT_THUMB,
	LT, RT
};
enum XBOX_STICK {
	L_UP, L_DOWN, L_LEFT, L_RIGHT,
	R_UP, R_DOWN, R_LEFT, R_RIGHT, XBOX_STICK_NUM
};

class UsersInput
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	static UsersInput* s_instance;
public:
	static UsersInput* Instance()
	{
		if (s_instance == nullptr)
		{
			printf("UsersInputのインスタンスを呼び出そうとしましたがnullptrでした\n");
			assert(0);
		}
		return s_instance;
	}

	struct MouseMove
	{
		LONG m_inputX;
		LONG m_inputY;
		LONG m_inputZ;
	};

private:
	ComPtr<IDirectInput8> m_dinput = nullptr;

	//キーボード
	ComPtr<IDirectInputDevice8> m_keyDev;
	static const int s_keyNum = 256;
	BYTE m_keys[s_keyNum] = {};
	BYTE m_oldkeys[s_keyNum] = {};

	//マウス
	ComPtr<IDirectInputDevice8>m_mouseDev;
	DIMOUSESTATE2 m_mouseState = {};
	DIMOUSESTATE2 m_oldMouseState = {};
	//マウスのゲーム空間内でのレイ
	Vec2<float> m_mousePos;

	//XINPUT(コントローラー用)
	static const int s_controllerNum = 3;
	XINPUT_STATE m_xinputState[s_controllerNum];
	XINPUT_STATE m_oldXinputState[s_controllerNum];
	float m_shakePower[s_controllerNum] = { 0.0f };
	int m_shakeTimer[s_controllerNum] = { 0 };
	//デッドゾーンに入っているか(DeadRate : デッドゾーン判定の度合い、1.0fだとデフォルト)
	bool StickInDeadZone(Vec2<float>& Thumb, const Vec2<float>& DeadRate)const;

	void Initialize(const WNDCLASSEX& WinClass,const HWND& Hwnd);

public:
	UsersInput(const WNDCLASSEX& WinClass, const HWND& Hwnd)
	{
		if (s_instance != nullptr)
		{
			printf("既にインスタンスがあります。UsersInputは１つのインスタンスしか持てません\n");
			assert(0);
		}
		s_instance = this;
		Initialize(WinClass, Hwnd);
	}
	~UsersInput() {};

	void Update(const HWND& Hwnd, const Vec2<float>& WinSize);

	//キーボード
	bool KeyOnTrigger(int KeyCode)const;
	bool KeyInput(int KeyCode)const;
	bool KeyOffTrigger(int KeyCode)const;

	//マウス
	bool MouseOnTrigger(MOUSE_BUTTON Button)const;
	bool MouseInput(MOUSE_BUTTON Button)const;
	bool MouseOffTrigger(MOUSE_BUTTON Button)const;

	const Vec2<float>& GetMousePos()const { return m_mousePos; }
	MouseMove GetMouseMove()const;
	//Ray GetMouseRay();

	//XBOXコントローラー
	bool ControllerOnTrigger(const int& ControllerIdx, XBOX_BUTTON Button)const;
	bool ControllerOnTrigger(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange = 0.3f, const Vec2<float>& DeadRate = { 1.0f,1.0f })const;
	bool ControllerInput(const int& ControllerIdx, XBOX_BUTTON Button)const;
	bool ControllerInput(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange = 0.3f, const Vec2<float>& DeadRate = { 1.0f,1.0f })const;
	bool ControllerOffTrigger(const int& ControllerIdx, XBOX_BUTTON Button)const;
	bool ControllerOffTrigger(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange = 0.3f, const Vec2<float>& DeadRate = { 1.0f,1.0f })const;

	//デッドゾーン判定の度合い(1.0fだとデフォルト採用)
	Vec2<float>GetLeftStickVec(const int& ControllerIdx, const Vec2<float>& DeadRate = { 1.0f,1.0f })const;
	Vec2<float>GetRightStickVec(const int& ControllerIdx, const Vec2<float>& DeadRate = { 1.0f,1.0f })const;
	// "Power" must fit between 0.0f and 1.0f.
	void ShakeController(const int& ControllerIdx, const float& Power, const int& Span);
};

