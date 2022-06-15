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

	static UsersInput* INSTANCE;
public:
	static UsersInput* Instance()
	{
		if (INSTANCE == nullptr)
		{
			printf("UsersInputのインスタンスを呼び出そうとしましたがnullptrでした\n");
			assert(0);
		}
		return INSTANCE;
	}

	struct MouseMove
	{
		LONG IX;
		LONG IY;
		LONG IZ;
	};

private:
	ComPtr<IDirectInput8> dinput = nullptr;

	//キーボード
	ComPtr<IDirectInputDevice8> keyDev;
	static const int KEY_NUM = 256;
	BYTE keys[KEY_NUM] = {};
	BYTE oldkeys[KEY_NUM] = {};

	//マウス
	ComPtr<IDirectInputDevice8>mouseDev;
	DIMOUSESTATE2 mouseState = {};
	DIMOUSESTATE2 oldMouseState = {};
	//マウスのゲーム空間内でのレイ
	Vec2<float> mousePos;

	//XINPUT(コントローラー用)
	static const int CONTROLLER_NUM = 3;
	XINPUT_STATE xinputState[CONTROLLER_NUM];
	XINPUT_STATE oldXinputState[CONTROLLER_NUM];
	float shakePower[CONTROLLER_NUM] = { 0.0f };
	int shakeTimer[CONTROLLER_NUM] = { 0 };
	//デッドゾーンに入っているか(DeadRate : デッドゾーン判定の度合い、1.0fだとデフォルト)
	bool StickInDeadZone(Vec2<float>& Thumb, const Vec2<float>& DeadRate);

	void Initialize(const WNDCLASSEX& WinClass,const HWND& Hwnd);

public:
	UsersInput(const WNDCLASSEX& WinClass, const HWND& Hwnd)
	{
		if (INSTANCE != nullptr)
		{
			printf("既にインスタンスがあります。UsersInputは１つのインスタンスしか持てません\n");
			assert(0);
		}
		INSTANCE = this;
		Initialize(WinClass, Hwnd);
	}
	~UsersInput() {};

	void Update(const HWND& Hwnd, const Vec2<float>& WinSize);

	//キーボード
	bool KeyOnTrigger(int KeyCode);
	bool KeyInput(int KeyCode);
	bool KeyOffTrigger(int KeyCode);

	//マウス
	bool MouseOnTrigger(MOUSE_BUTTON Button);
	bool MouseInput(MOUSE_BUTTON Button);
	bool MouseOffTrigger(MOUSE_BUTTON Button);

	const Vec2<float>& GetMousePos()const { return mousePos; }
	MouseMove GetMouseMove();
	//Ray GetMouseRay();

	//XBOXコントローラー
	bool ControllerOnTrigger(const int& ControllerIdx, XBOX_BUTTON Button);
	bool ControllerOnTrigger(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange = 0.3f, const Vec2<float>& DeadRate = { 1.0f,1.0f });
	bool ControllerInput(const int& ControllerIdx, XBOX_BUTTON Button);
	bool ControllerInput(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange = 0.3f, const Vec2<float>& DeadRate = { 1.0f,1.0f });
	bool ControllerOffTrigger(const int& ControllerIdx, XBOX_BUTTON Button);
	bool ControllerOffTrigger(const int& ControllerIdx, XBOX_STICK StickInput, const float& DeadRange = 0.3f, const Vec2<float>& DeadRate = { 1.0f,1.0f });

	//デッドゾーン判定の度合い(1.0fだとデフォルト採用)
	Vec2<float>GetLeftStickVec(const int& ControllerIdx, const Vec2<float>& DeadRate = { 1.0f,1.0f });
	Vec2<float>GetRightStickVec(const int& ControllerIdx, const Vec2<float>& DeadRate = { 1.0f,1.0f });
	// "Power" must fit between 0.0f and 1.0f.
	void ShakeController(const int& ControllerIdx, const float& Power, const int& Span);
};

