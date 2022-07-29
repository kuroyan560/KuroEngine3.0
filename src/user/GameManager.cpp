#include "GameManager.h"
#include"imguiApp.h"
#include"Player.h"

GameManager::GameManager()
{
	RegisterCamera(m_debugCamKey, m_debugCam);
}

void GameManager::Update()
{
	//デバッグカメラの更新
	if (!m_debugCamKey.compare(m_nowCamKey))
	{
		m_debugCam.Move();
	}
}

void GameManager::ImGuiDebug(UsersInput& Input)
{
	m_controllerConfig.ImguiDebug(Input);

	ImGui::Begin("GameManager");

/*--- デバッグ描画 ---*/
	ImGui::Checkbox("DebugDraw", &m_debugDraw);

	ImGui::Separator();

/*--- 環境 ---*/
	//重力
	ImGui::InputFloat("Gravity", &m_gravity);

	ImGui::Separator();


/*--- カメラ選択 ---*/
	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(250, 100), ImGuiWindowFlags_NoTitleBar);
	for (auto& cam : m_cameras)
	{
		//カメラキーの取得
		const auto& camKey = cam.first;	
		if (ImGui::RadioButton(camKey.c_str(), m_nowCamKey == camKey))	//選択されたか
		{
			//カメラ変更
			ChangeCamera(camKey);
		}
	}
	ImGui::EndChild();

	ImGui::End();
}
