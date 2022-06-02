#include "GameScene.h"
#include"KuroEngine.h"
#include"Importer.h"
#include"DrawFunc3D.h"
#include"DrawFunc2D.h"
#include"Camera.h"
#include"Model.h"
#include"Object.h"
#include"GaussianBlur.h"
#include"CubeMap.h"

GameScene::GameScene()
{
	testModel = std::make_shared<ModelObject>("resource/user/", "player.glb");
	testModel->model->MeshSmoothing();

	//dirLig.SetDir(Vec3<Angle>(50, -30, 0));
	ligMgr.RegisterDirLight(&dirLig);
	ligMgr.RegisterPointLight(&ptLig);
	ligMgr.RegisterHemiSphereLight(&hemiLig);

	const std::string skyBoxDir = "resource/user/skybox/";
	cubeMap = std::make_shared<CubeMap>("TestCubeMap");
	cubeMap->AttachTex(CubeMap::PZ, D3D12App::Instance()->GenerateTextureBuffer(skyBoxDir + "pz.png"));
	cubeMap->AttachTex(CubeMap::NZ, D3D12App::Instance()->GenerateTextureBuffer(skyBoxDir + "nz.png"));
	cubeMap->AttachTex(CubeMap::PX, D3D12App::Instance()->GenerateTextureBuffer(skyBoxDir + "px.png"));
	cubeMap->AttachTex(CubeMap::NX, D3D12App::Instance()->GenerateTextureBuffer(skyBoxDir + "nx.png"));
	cubeMap->AttachTex(CubeMap::PY, D3D12App::Instance()->GenerateTextureBuffer(skyBoxDir + "py.png"));
	cubeMap->AttachTex(CubeMap::NY, D3D12App::Instance()->GenerateTextureBuffer(skyBoxDir + "ny.png"));

}

void GameScene::OnInitialize()
{
	debugCam.Init({ 0,1,-3 }, { 0,1,0 });
}

void GameScene::OnUpdate()
{
	if (UsersInput::Instance()->KeyOnTrigger(DIK_I))
	{
		debugCam.Init({ 0,1,-3 }, { 0,1,0 });
	}

	//ポイントライト位置
	static const float UINT = 0.1f;
	auto ptLigPos = ptLig.GetPos();
	if (UsersInput::Instance()->KeyInput(DIK_RIGHT))
	{
		ptLigPos.x += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_LEFT))
	{
		ptLigPos.x -= UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_UP))
	{
		ptLigPos.z += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_DOWN))
	{
		ptLigPos.z -= UINT;
	}
	ptLig.SetPos(ptLigPos);

	//テストモデルの位置
	auto modelPos = testModel->transform.GetPos();
	if (UsersInput::Instance()->KeyInput(DIK_E))
	{
		modelPos.y += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_Q))
	{
		modelPos.y -= UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_D))
	{
		modelPos.x += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_A))
	{
		modelPos.x -= UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_W))
	{
		modelPos.z += UINT;
	}
	if (UsersInput::Instance()->KeyInput(DIK_S))
	{
		modelPos.z -= UINT;
	}
	testModel->transform.SetPos(modelPos);

	//ライトのON/OFF
	if (UsersInput::Instance()->KeyOnTrigger(DIK_1))
	{
		dirLig.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_2))
	{
		ptLig.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_3))
	{
		hemiLig.SetActive();
	}

	debugCam.Move();
}


void GameScene::OnDraw()
{
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());

	dsv->Clear(D3D12App::Instance()->GetCmdList());

	//標準描画
	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);
	cubeMap->Draw(debugCam);
	DrawFunc3D::DrawADSShadingModel(ligMgr, testModel, debugCam);
}

void GameScene::OnImguiDebug()
{
}

void GameScene::OnFinalize()
{
}
