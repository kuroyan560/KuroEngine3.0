#include<windows.h>
#include"KuroEngine.h"
#include"GameScene.h"
#include"Transform.h"
#include"Transform2D.h"
#include"Color.h"

#include"DrawFunc2D.h"
#include"DrawFunc2D_Shadow.h"
#include"DrawFunc2D_FillTex.h"
#include"DrawFunc2D_Mask.h"
#include"DrawFunc2D_Color.h"

#include"DrawFunc3D.h"

#include"NoiseGenerator.h"


#ifdef _DEBUG
int main()
#else
//Windowsアプリでのエントリーポイント（main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	//エンジン設定=====================
	EngineOption engineOption;
	engineOption.m_windowName = "KuroEngine3.0";
	engineOption.m_windowSize = { 1280,720 };
	engineOption.m_iconPath = nullptr;
	engineOption.m_backBuffClearColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
	engineOption.m_useHDR = false;
	engineOption.m_frameRate = 43;

	//===============================

	//エンジン起動
	KuroEngine::Instance().Initialize(engineOption);

	//シーンリスト=====================

	std::vector<BaseScene*>sceneList =
	{
		new GameScene(),
	};
	int awakeScene = 0;	//開始時のステージ番号

	//================================

	//エンジンにシーンリストを渡す
	KuroEngine::Instance().SetSceneList(sceneList, awakeScene);

	//ループ
	while (1)
	{
		KuroEngine::Instance().Update();
		KuroEngine::Instance().Draw();

		//静的クラス初期化（Dirtyフラグ系）
		Transform::DirtyReset();
		Transform2D::DirtyReset();

		DrawFunc2D::CountReset();
		DrawFunc2D_Shadow::CountReset();
		DrawFunc2D_FillTex::CountReset();
		DrawFunc2D_Mask::CountReset();
		DrawFunc2D_Color::CountReset();

		DrawFunc3D::CountReset();

		NoiseGenerator::CountReset();

		//メッセージがある？
		MSG msg{};
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);	//キー入力メッセージの処理
			DispatchMessage(&msg);	//プロシージャにメッセージを送る
		}

		//終了メッセージが来た / シーンマネージャの終了　でループを抜ける
		if (msg.message == WM_QUIT || KuroEngine::Instance().End())
		{
			break;
		}
	}
}