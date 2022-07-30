#pragma once
#include"ControllerConfig.h"
#include<array>
class UsersInput;

//プレイヤーの状態
enum struct PLAYER_STATUS_TAG
{
	WAIT,					//待機
	MOVE,					//通常移動
	ATTACK,	//通常攻撃
	JUMP,					//ジャンプ
	GUARD,					//ガード
	DODGE,					//回避
	DASH,					//ダッシュ
	MARKING,				//マーキング
	CHARGE,				//（単発ラッシュ）のチャージ中
	RUSH,					//ラッシュ
	INVOKE_ABILITY,		//アビリティ発動
	OUT_OF_CONTROL,	//無操作（操作を受け付けない）
	NUM
};

//状態遷移に必要な情報の一時格納用構造体
struct PlayerParameterForStatus
{
	int m_markingNum = 0;
	bool m_maxMarking = false;
	bool m_onGround = false;
	bool m_attackFinish = true;
	bool m_dodgeFinish = true;
	bool m_rushFinish = true;
	bool m_abilityFinish = true;
};

//プレイヤーの状態遷移管理
class PlayerStatus
{
	PLAYER_STATUS_TAG m_status = PLAYER_STATUS_TAG::OUT_OF_CONTROL;
	PLAYER_STATUS_TAG m_oldStatus = m_status;	//１フレーム前の状態

	//入力フレームの記録
	std::array<int, static_cast<int>(HANDLE_INPUT_TAG::NUM)>m_inputFrame;
	int m_leftStickInputFrame;

	//一度ボタンを離しているかのフラグ（長押しでの連発防止）
	std::array<bool, static_cast<int>(HANDLE_INPUT_TAG::NUM)>m_off;

	//各状態中の遷移
	PLAYER_STATUS_TAG WaitUpdate(const PlayerParameterForStatus& Parameters);
	PLAYER_STATUS_TAG MoveUpdate(const PlayerParameterForStatus& Parameters);
	PLAYER_STATUS_TAG JumpUpdate(const PlayerParameterForStatus& Parameters);

public:
	//初期化
	void Init(PLAYER_STATUS_TAG InitTag = PLAYER_STATUS_TAG::OUT_OF_CONTROL)
	{
		m_status = InitTag;
		m_oldStatus = m_status;

		//入力フレームの記録リセット
		m_inputFrame.fill(0);
		m_leftStickInputFrame = 0;

		//ボタン離しフラグリセット
		m_off.fill(true);
	}

	//状態遷移更新
	void Update(const UsersInput& Input, const ControllerConfig& Controller, const PlayerParameterForStatus& Parameters);

	//操作不能
	void SetOutOfControl() 
	{
		m_oldStatus = m_status;
		m_status = PLAYER_STATUS_TAG::OUT_OF_CONTROL; 
	}
	//操作不能解除
	void QuitOutOfControl(PLAYER_STATUS_TAG InitTag = PLAYER_STATUS_TAG::WAIT)
	{
		Init(InitTag);
	}

	//現在の状態が引数に渡されたものか
	bool CompareNowStatus(PLAYER_STATUS_TAG Tag) { return m_status == Tag; }
	bool CompareOldStatus(PLAYER_STATUS_TAG Tag) { return m_oldStatus == Tag; }

	//ステータスが変化した瞬間
	bool StatusTrigger()
	{
		return m_status != m_oldStatus;
	}
	//引数に渡したステータスに変化した瞬間か
	bool StatusTrigger(PLAYER_STATUS_TAG Status)
	{
		return CompareNowStatus(Status) && !CompareOldStatus(Status);
	}

	//ステータスのゲッタ
	PLAYER_STATUS_TAG& GetNowStatus() { return m_status; }
	PLAYER_STATUS_TAG& GetOldStatus() { return m_oldStatus; }

	//入力フレームのゲッタ
	const int& GetInputFrame(HANDLE_INPUT_TAG Tag)
	{
		return m_inputFrame[static_cast<int>(Tag)];
	}
};

