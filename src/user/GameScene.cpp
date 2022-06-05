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
	models[WOOD_CUBE] = std::make_shared<ModelObject>("resource/user/gltf/woodball/", "woodcube.glb");
	models[METAL_BALL] = std::make_shared<ModelObject>("resource/user/gltf/metalball/", "metalball.glb");
	models[STONE_BALL] = std::make_shared<ModelObject>("resource/user/gltf/stoneball/", "stoneball.glb");
	//sphere->model->MeshSmoothing();

	testModel = std::make_shared<ModelObject>("resource/user/", "player.glb");
	testModel->transform.SetPos({ 4,0,0 });
	//testModel->model->MeshBuildTangentAndBiNormal();
	//testModel->model->MeshSmoothing();

	//dirLig.SetDir(Vec3<Angle>(50, -30, 0));
	dirLigTop.SetDir(Vec3<float>(0, 0, -1));
	dirLigFront.SetDir(Vec3<float>(0, 0, 1));
	ligMgr.RegisterDirLight(&dirLigTop);
	dirLigTop.SetColor(Color(1.0f, 0.0f, 1.0f, 1.0f));
	ligMgr.RegisterDirLight(&dirLigFront);
	ligMgr.RegisterPointLight(&ptLig);
	ligMgr.RegisterHemiSphereLight(&hemiLig);

	const std::string yokohamaDir = "resource/user/Yokohama3/";
	yokohamaCubeMap = std::make_shared<StaticallyCubeMap>("YokohamaCubeMap");
	yokohamaCubeMap->AttachTex(CubeMap::PZ, D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "posz.jpg"));
	yokohamaCubeMap->AttachTex(CubeMap::NZ, D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "negz.jpg"));
	yokohamaCubeMap->AttachTex(CubeMap::NX, D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "posx.jpg"));
	yokohamaCubeMap->AttachTex(CubeMap::PX, D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "negx.jpg"));
	yokohamaCubeMap->AttachTex(CubeMap::NY, D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "posy.jpg"));
	yokohamaCubeMap->AttachTex(CubeMap::PY, D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "negy.jpg"));
	//yokohamaCubeMap->AttachCubeMapTex(D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "yokohama_cube.dds", true));
	yokohamaCubeMap->AttachCubeMapTex(D3D12App::Instance()->GenerateTextureBuffer(yokohamaDir + "my_yokohama_cube.dds", true));

	const std::string skyDir = "resource/user/skybox/";
	skyCubeMap = std::make_shared<StaticallyCubeMap>("SkyCubeMap");
	skyCubeMap->AttachTex(CubeMap::PZ, D3D12App::Instance()->GenerateTextureBuffer(skyDir + "posz.png"));
	skyCubeMap->AttachTex(CubeMap::NZ, D3D12App::Instance()->GenerateTextureBuffer(skyDir + "negz.png"));
	skyCubeMap->AttachTex(CubeMap::NX, D3D12App::Instance()->GenerateTextureBuffer(skyDir + "posx.png"));
	skyCubeMap->AttachTex(CubeMap::PX, D3D12App::Instance()->GenerateTextureBuffer(skyDir + "negx.png"));
	skyCubeMap->AttachTex(CubeMap::NY, D3D12App::Instance()->GenerateTextureBuffer(skyDir + "posy.png"));
	skyCubeMap->AttachTex(CubeMap::PY, D3D12App::Instance()->GenerateTextureBuffer(skyDir + "negy.png"));
	skyCubeMap->AttachCubeMapTex(D3D12App::Instance()->GenerateTextureBuffer(skyDir + "sky_cube.dds", true));

	const std::string hdriDir = "resource/user/hdri/";
	hdriCubeMap = std::make_shared<StaticallyCubeMap>("HDRICubeMap");
	hdriCubeMap->AttachTex(CubeMap::PZ, D3D12App::Instance()->GenerateTextureBuffer(hdriDir + "pz.png"));
	hdriCubeMap->AttachTex(CubeMap::NZ, D3D12App::Instance()->GenerateTextureBuffer(hdriDir + "nz.png"));
	hdriCubeMap->AttachTex(CubeMap::NX, D3D12App::Instance()->GenerateTextureBuffer(hdriDir + "px.png"));
	hdriCubeMap->AttachTex(CubeMap::PX, D3D12App::Instance()->GenerateTextureBuffer(hdriDir + "nx.png"));
	hdriCubeMap->AttachTex(CubeMap::NY, D3D12App::Instance()->GenerateTextureBuffer(hdriDir + "py.png"));
	hdriCubeMap->AttachTex(CubeMap::PY, D3D12App::Instance()->GenerateTextureBuffer(hdriDir + "ny.png"));
	hdriCubeMap->AttachCubeMapTex(D3D12App::Instance()->GenerateTextureBuffer(hdriDir + "hdri_cube.dds", true));

	dynamicCubeMap = std::make_shared<DynamicCubeMap>();
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

	//テストモデルの位置
	auto modelPos = testModel->transform.GetPos();
	//auto modelPos = ptLig.GetPos();
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
	//ptLig.SetPos(modelPos);

	//ライトのON/OFF
	if (UsersInput::Instance()->KeyOnTrigger(DIK_1))
	{
		dirLigFront.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_2))
	{
		dirLigTop.SetActive();
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_3))
	{
		hemiLig.SetActive();
	}

	if (UsersInput::Instance()->KeyOnTrigger(DIK_LEFT))
	{
		if (0 < nowModel)nowModel = (MODEL_NAME)(nowModel - 1);
		else nowModel = (MODEL_NAME)(MODEL_NUM - 1);
	}
	else if (UsersInput::Instance()->KeyOnTrigger(DIK_RIGHT))
	{
		if (nowModel < MODEL_NUM - 1)nowModel = (MODEL_NAME)(nowModel + 1);
		else nowModel = (MODEL_NAME)0;
	}

	debugCam.Move();
}


void GameScene::OnDraw()
{
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());

	dsv->Clear(D3D12App::Instance()->GetCmdList());

	//動的キューブマップに書き込み
	dynamicCubeMap->Clear();
	dynamicCubeMap->CopyCubeMap(hdriCubeMap);
	dynamicCubeMap->DrawToCubeMap(ligMgr, { testModel });

	//標準描画
	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);
	hdriCubeMap->Draw(debugCam);
	//DrawFunc3D::DrawADSShadingModel(ligMgr, testModel, debugCam);
	//DrawFunc3D::DrawPBRShadingModel(ligMgr, testModel, debugCam, yokohamaCubeMap);
	DrawFunc3D::DrawPBRShadingModel(ligMgr, testModel, debugCam, hdriCubeMap);
	DrawFunc3D::DrawPBRShadingModel(ligMgr, models[nowModel], debugCam, dynamicCubeMap);
}

void GameScene::OnImguiDebug()
{
	ImguiApp::Instance()->DebugMaterial(models[nowModel]->model->meshes[0].material, REWRITE);
}

void GameScene::OnFinalize()
{
}
