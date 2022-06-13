#include "GameManager.h"
#include"imguiApp.h"
#include"Player.h"

GameManager::GameManager()
{
	RegisterCamera(debugCamKey, debugCam);
}

void GameManager::Update()
{
	//デバッグカメラの更新
	if (!debugCamKey.compare(nowCamKey))
	{
		debugCam.Move();
	}
}

void GameManager::ImGuiDebug()
{
	ImGui::Begin("GameManager - Settings");

	//カメラ選択
	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(250, 100), ImGuiWindowFlags_NoTitleBar);
	for (auto& cam : cameras)
	{
		//カメラキーの取得
		const auto& camKey = cam.first;	
		if (ImGui::RadioButton(camKey.c_str(), nowCamKey == camKey))	//選択されたか
		{
			//カメラ変更
			ChangeCamera(camKey);
		}
	}
	ImGui::EndChild();

	ImGui::End();
}
