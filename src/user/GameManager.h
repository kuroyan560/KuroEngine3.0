#pragma once
#include"Singleton.h"
#include<map>
#include<string>
#include<memory>
class Camera;
#include"DebugCamera.h"
#include"KuroFunc.h"


class GameManager : public Singleton<GameManager>
{
	friend class Singleton<GameManager>;
	GameManager();

	//デバッグカメラキー
	const std::string debugCamKey = "DebugCam";

	//デバッグ用カメラ
	DebugCamera debugCam;
	
	//カメラ配列
	std::string nowCamKey = debugCamKey;
	std::map<std::string, std::weak_ptr<Camera>>cameras;

public:
/*--- 基本ゲームループ ---*/
	void Update();

/*--- カメラ関連 ---*/
	//カメラ登録
	void RegisterCamera(const std::string& Key, const std::shared_ptr<Camera>& Cam)
	{
		KuroFunc::ErrorMessage(cameras.contains(Key), "GameManager", "RegisterCamera", "This camera key is already used.");
		cameras[Key] = Cam;
	}
	//カメラ変更
	void ChangeCamera(const std::string& Key)
	{
		KuroFunc::ErrorMessage(!cameras.contains(Key), "GameManager", "ChangeCamera", "This camera key is invalid.");
		nowCamKey = Key;
	}
	//現在選択中のカメラキーと同一か
	bool IsNowCamera(const std::string& Key) { return nowCamKey == Key; }
	//現在のカメラ取得
	std::shared_ptr<Camera>GetNowCamera() { return cameras[nowCamKey].lock(); }

/*--- その他 ---*/
	//Imguiを利用したデバッグ
	void ImGuiDebug();
};