#pragma once
//エンジン本体==========================================================
#include<string>
#include"Vec.h"
#include<memory>
#include"Color.h"

#include"D3D12App.h"
#include"WinApp.h"
#include"UsersInput.h"
#include"AudioApp.h"
#include"ImguiApp.h"
#include"GraphicsManager.h"
#include"Singleton.h"

class Fps;

//シーンの基底クラス
class BaseScene
{
protected:
	BaseScene() {}
	virtual void OnInitialize() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnDraw() = 0;
	virtual void OnImguiDebug() = 0;
	virtual void OnFinalize() = 0;
public:
	virtual ~BaseScene() {};
	void Initialize();
	void Update();
	void Draw();
	void ImguiDebug();
	void Finalize();
};

//シーン遷移（基本は継承して利用するが、１フレ切り替え遷移は継承なしで利用できる）
class SceneTransition
{
protected:
	bool m_nowTrans = false;
	virtual void OnStart() { }
	virtual bool OnUpdate() { m_nowTrans = false; return true; }
	virtual void OnDraw() {}
public:
	virtual ~SceneTransition() {}
	void Start() { m_nowTrans = true; OnStart(); }	//シーン遷移スタート
	bool Update() { return OnUpdate(); }	//シーンを切り替えるタイミングでtrueを返す
	void Draw() { OnDraw(); }
	bool Finish() { return !m_nowTrans; }	//シーン遷移が完全に終了したか
};

//アプリの初期設定
struct EngineOption
{
	//ウィンドウ関連
	std::string m_windowName;	//ウィンドウ名
	Vec2<int>m_windowSize;	//ウィンドウサイズ
	const wchar_t *m_iconPath = nullptr;	//ウィンドウアイコンファイル名

	//バックバッファ関連
	Color m_backBuffClearColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
	bool m_useHDR = false;	//HDRを使うか（HDR対応していない場合使用しない）

	//フレームレート
	int m_frameRate = 60;
};

//エンジン本体
class KuroEngine : public D3D12AppUser, public Singleton<KuroEngine>
{
public:
	friend class Singleton<KuroEngine>;

private:
	KuroEngine() {}

	//初期化されているか
	bool m_invalid = true;

	//XAudio
	HMODULE m_xaudioLib;
	//ウィンドウアプリ
	std::unique_ptr<WinApp>m_winApp;
	//D3D12アプリ
	std::unique_ptr<D3D12App>m_d3d12App;
	//入力管理
	std::unique_ptr<UsersInput>m_usersInput;
	//音声関連アプリ
	std::unique_ptr<AudioApp>m_audioApp;
	//Imguiアプリ
	std::unique_ptr<ImguiApp>m_imguiApp;

	//グラフィックスマネージャ（レンダリング統括、Zソートなどを行う）
	GraphicsManager m_gManager;
	std::shared_ptr<ConstantBuffer>m_parallelMatProjBuff;

	//シーンリスト
	std::vector<BaseScene *>m_scenes;
	int m_nowScene;	//現在のシーン番号
	int m_nextScene = -1;	//次のシーン番号
	SceneTransition *m_nowSceneTransition;	//現在セットされているシーン遷移

	//FPS固定
	std::shared_ptr<Fps>m_fps;

	//終了フラグ
	bool m_end = false;

	void Render()override;
public:
	~KuroEngine();
	void Initialize(const EngineOption &Option);
	void SetSceneList(const std::vector<BaseScene *> &SceneList, const int &AwakeSceneNum);
	void Update();
	void Draw();
	bool End() { return m_end; }

	//ゲーム終了
	void GameEnd() { m_end = true; }
	//シーンチェンジ
	void ChangeScene(const int &SceneNum, SceneTransition *SceneTransition)
	{
		m_nextScene = SceneNum;
		m_nowSceneTransition = SceneTransition;
		m_nowSceneTransition->Start();
	}

	//グラフィックスマネージャゲッタ
	GraphicsManager &Graphics() { return m_gManager; }

	//平行投影行列定数バッファ
	const std::shared_ptr<ConstantBuffer> &GetParallelMatProjBuff()
	{
		return m_parallelMatProjBuff;
	}

};

