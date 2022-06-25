#include "KuroEngine.h"
#include<ctime>
#include"Fps.h"
#include"ImguiDebugInterface.h"

void BaseScene::Initialize()
{
	OnInitialize();
}

void BaseScene::Update()
{
	OnUpdate();
}

void BaseScene::Draw()
{
	OnDraw();
}

void BaseScene::ImguiDebug()
{
	OnImguiDebug();
}

void BaseScene::Finalize()
{
	OnFinalize();
}

void KuroEngine::Render()
{
	scenes[nowScene]->Draw();

	//シーン遷移描画
	if (nowSceneTransition != nullptr)
	{
		nowSceneTransition->Draw();
	}

	//グラフィックスマネージャのコマンドリスト全実行
	gManager.CommandsExcute(d3d12App->GetCmdList());

	//Imgui
	d3d12App->SetBackBufferRenderTarget();
	imguiApp->BeginImgui();
	scenes[nowScene]->ImguiDebug();

	ImGui::Begin("Fps");
	ImGui::Text("fps : %.5f", fps->GetNowFps());
	ImGui::End();

	ImguiDebugInterface::DrawImguiDebugger();

	imguiApp->EndImgui(d3d12App->GetCmdList());
}

KuroEngine::~KuroEngine()
{
	//XAudio2の解放
	FreeLibrary(xaudioLib);

	//シーンの削除
	for (int i = 0; i < scenes.size(); ++i)
	{
		delete scenes[i];
	}

	printf("KuroEngineシャットダウン\n");
}

void KuroEngine::Initialize(const EngineOption& Option)
{
	if (!invalid)
	{
		printf("エラー：KuroEngineは起動済です\n");
		return;
	}
	printf("KuroEngineを起動します\n");

	//XAudioの読み込み
	xaudioLib = LoadLibrary(L"XAudio2_9.dll");

	//乱数取得用シード生成
	srand(static_cast<unsigned int>(time(NULL)));

	//ウィンドウアプリ生成
	winApp = std::make_unique<WinApp>(Option.windowName, Option.windowSize, Option.iconPath);

	//D3D12アプリ生成
	d3d12App = std::make_unique<D3D12App>(winApp->GetHwnd(), Option.windowSize, Option.useHDR, Option.backBuffClearColor);

	//インプット管理アプリ生成
	usersInput = std::make_unique<UsersInput>(winApp->GetWinClass(), winApp->GetHwnd());

	//音声関連アプリ
	audioApp = std::make_unique<AudioApp>();

	//Imguiアプリ
	imguiApp = std::make_unique<ImguiApp>(d3d12App->GetDevice(), winApp->GetHwnd());

	//FPS固定機能
	fps = std::make_shared<Fps>(Option.frameRate);


	//平行投影行列定数バッファ生成
	auto parallelMatProj = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, winApp->GetExpandWinSize().x, winApp->GetExpandWinSize().y, 0.0f, 0.0f, 1.0f);
	parallelMatProjBuff = d3d12App->GenerateConstantBuffer(sizeof(XMMATRIX), 1, 
		&parallelMatProj);

	printf("KuroEngine起動成功\n");
	invalid = false;

}

void KuroEngine::SetSceneList(const std::vector<BaseScene*>& SceneList, const int& AwakeSceneNum)
{
	//シーンリスト移動
	scenes = SceneList;
	nowScene = AwakeSceneNum;
	scenes[nowScene]->Initialize();
}

void KuroEngine::Update()
{
	//FPS更新
	fps->Update();

	//音声関連アプリ更新
	audioApp->Update();

	//シーン切り替えフラグ
	bool sceneChangeFlg = false;

	if (nowSceneTransition != nullptr) //シーン遷移中
	{
		//シーン遷移クラスの更新関数は、シーン切り替えのタイミングで true を還す
		sceneChangeFlg = nowSceneTransition->Update() && (nextScene != -1);

		//シーン遷移終了
		if (nowSceneTransition->Finish())
		{
			nowSceneTransition = nullptr;
		}
	}

	//シーン切り替え
	if (sceneChangeFlg)
	{
		scenes[nowScene]->Finalize();	//切り替え前のシーン終了処理
		nowScene = nextScene;		//シーン切り替え
		scenes[nowScene]->Initialize();	//切り替え後のシーン初期化処理
		nextScene = -1;
	}

	//入力更新
	usersInput->Update(winApp->GetHwnd(), winApp->GetExpandWinSize());

	scenes[nowScene]->Update();
}

void KuroEngine::Draw()
{
	d3d12App->Render(this);
}