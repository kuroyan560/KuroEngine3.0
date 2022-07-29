#pragma once
#include"Vec.h"
class UsersInput;

//ボタン割り当て用タグ
enum struct HANDLE_INPUT_TAG
{
	ATTACK,			//通常攻撃
	JUMP,							//ジャンプ
	GUARD_DODGE_DASH,		// ガード / 回避 / ダッシュ
	MARKING,						//マーキング
	RUSH,							//ラッシュ
	ABILITY,							//アビリティ発動
	GENERIC_ACTION,			//汎用アクション
	NUM,	//タグ数
	NONE,
};

//割り当てられるボタン
enum struct CAN_ALLOCATE_BUTTON
{
	A, B, Y, X, LB, RB, LT, RT, NUM,
};

//ボタン割り当て設定
class ControllerConfig
{
	int m_controllerIdx = 0;
	HANDLE_INPUT_TAG m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::NUM)];
	Vec2<bool>m_camMirror = { false,false };

	int GetAllocateButtonIdx(HANDLE_INPUT_TAG Tag)const;
	CAN_ALLOCATE_BUTTON GetAllocateButton(HANDLE_INPUT_TAG Tag)const
	{
		return static_cast<CAN_ALLOCATE_BUTTON>(GetAllocateButtonIdx(Tag));
	}

public:
	ControllerConfig() { Reset(); }
	//初期設定
	void Reset();

	//入力取得
	bool GetHandleInput(const UsersInput& Input, HANDLE_INPUT_TAG Tag)const;
	bool GetHandleInput(const UsersInput& Input, int TagIdx)const
	{
		return GetHandleInput(Input, static_cast<HANDLE_INPUT_TAG>(TagIdx));
	}

	//移動入力
	Vec2<float>GetMoveVec(const UsersInput& Input)const;
	//カメラ操作入力
	Vec2<float>GetCameraVec(const UsersInput& Input)const;
	//カメラロックオン入力
	bool GetCameraRock(const UsersInput& Input)const;
	//十字ボタン（左右：x 、上下：y で -1,0,1 のいずれか）
	Vec2<int>GetDpadInput(const UsersInput& Input)const;

	//コントローラーを振動させる
	void ShakeController(UsersInput& Controller, float Power, int Span);

	//ボタンを割り当てる（元々割り当てられていたものと入れ替え）
	void ReAllocateButton(HANDLE_INPUT_TAG Tag, CAN_ALLOCATE_BUTTON AllocateButton);
	//カメラーのミラーリング設定
	void ResetCamMirrorX(bool MirrorX) { m_camMirror.x = MirrorX; }
	void ResetCamMirrorY(bool MirrorY) { m_camMirror.y = MirrorY; }

	//Imguiデバッグ機能
	void ImguiDebug(UsersInput& Input);
};
