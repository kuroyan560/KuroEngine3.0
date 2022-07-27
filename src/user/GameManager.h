#pragma once
#include"Singleton.h"
#include<map>
#include<string>
#include<memory>
class Camera;
#include"DebugCamera.h"
#include"KuroFunc.h"
#include"ControllerConfig.h"
class UsersInput;

class GameManager : public Singleton<GameManager>
{
	friend class Singleton<GameManager>;
	GameManager();

	//デバッグカメラキー
	const std::string m_debugCamKey = "DebugCam";

	//デバッグ用カメラ
	DebugCamera m_debugCam;
	
	//カメラ配列
	std::string m_nowCamKey = m_debugCamKey;
	std::map<std::string, std::weak_ptr<Camera>>m_cameras;

	//コントローラー設定
	ControllerConfig m_controllerConfig;

public:
/*--- 基本ゲームループ ---*/
	void Update();

/*--- カメラ関連 ---*/
	//カメラ登録
	void RegisterCamera(const std::string& Key, const std::shared_ptr<Camera>& Cam)
	{
		assert(!m_cameras.contains(Key));
		m_cameras[Key] = Cam;
	}
	//カメラ変更
	void ChangeCamera(const std::string& Key)
	{
		assert(m_cameras.contains(Key));
		m_nowCamKey = Key;
	}
	//現在選択中のカメラキーと同一か
	bool IsNowCamera(const std::string& Key) { return m_nowCamKey == Key; }
	//現在のカメラ取得
	std::shared_ptr<Camera>GetNowCamera() { return m_cameras[m_nowCamKey].lock(); }

/*--- コントローラー設定 ---*/
	ControllerConfig& GetControllerConfig() { return m_controllerConfig; }

/*--- その他 ---*/
	//Imguiを利用したデバッグ
	void ImGuiDebug(UsersInput& Input);
};