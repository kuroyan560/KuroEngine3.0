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
#include"ModelAnimator.h"

GameScene::GameScene()
{
	axisModels[0] = std::make_shared<ModelObject>("resource/user/", "Axis_NonYUp.gltf");
	axisModels[1] = std::make_shared<ModelObject>("resource/user/", "Axis.gltf");

	animModel[0] = std::make_shared<ModelObject>("resource/user/player_anim_test/", "player_anim_test.glb");
	animModel[1] = std::make_shared<ModelObject>("resource/user/", "PrePlayer.gltf");
	//axisModel = std::make_shared<ModelObject>("resource/user/", "Axis.glb");

	//dirLig.SetDir(Vec3<Angle>(50, -30, 0));
	dirLigTop.SetDir(Vec3<float>(0, 0, -1));
	dirLigFront.SetDir(Vec3<float>(0, 0, 1));
	ligMgr.RegisterDirLight(&dirLigTop);
	ligMgr.RegisterDirLight(&dirLigFront);
	ligMgr.RegisterPointLight(&ptLig);
	hemiLig.SetSkyColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	hemiLig.SetGroundColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	ligMgr.RegisterHemiSphereLight(&hemiLig);
}

void GameScene::OnInitialize()
{
	debugCam.Init({ 0,1,-3 }, { 0,1,0 });
	animModel[nowModel]->transform.SetPos({ 0,0,3 });
}

void GameScene::OnUpdate()
{
	if (UsersInput::Instance()->KeyOnTrigger(DIK_I))
	{
		debugCam.Init({ 0,1,-3 }, { 0,1,0 });
		animModel[0]->animator->Reset();
		animModel[1]->animator->Reset();
	}

	//操作モデル切り替え
	if (UsersInput::Instance()->KeyOnTrigger(DIK_O))
	{
		nowModel = 1 - nowModel;
	}

	//アニメーション（通常モデル）
	static std::array<std::string, 2>ANIM_NAME = { "Action1","Run" };
	if (UsersInput::Instance()->KeyOnTrigger(DIK_L))
	{
		animModel[nowModel]->animator->Play(ANIM_NAME[nowModel], true);
	}
	if (UsersInput::Instance()->KeyOnTrigger(DIK_S))
	{
		animModel[nowModel]->animator->stop = !animModel[nowModel]->animator->stop;
	}

	animModel[nowModel]->animator->Update();
	axisModels[nowAxis]->animator->Update();

	debugCam.Move();
}


void GameScene::OnDraw()
{
	static std::shared_ptr<DepthStencil>dsv = D3D12App::Instance()->GenerateDepthStencil(
		D3D12App::Instance()->GetBackBuffRenderTarget()->GetGraphSize());

	dsv->Clear(D3D12App::Instance()->GetCmdList());

	//動的キューブマップに書き込み
	//標準描画
	KuroEngine::Instance().Graphics().SetRenderTargets({ D3D12App::Instance()->GetBackBuffRenderTarget() }, dsv);

	//DrawFunc3D::DrawADSShadingModel(ligMgr, animModel[nowModel], debugCam);
	//DrawFunc3D::DrawADSShadingModel(ligMgr, axisModel, debugCam);
	StaticallyCubeMap::GetDefaultCubeMap()->Draw(debugCam);
	DrawFunc3D::DrawPBRShadingModel(ligMgr, animModel[nowModel], debugCam);
	DrawFunc3D::DrawPBRShadingModel(ligMgr, axisModels[nowAxis], debugCam);
}

void GameScene::OnImguiDebug()
{
	//ImguiApp::Instance()->DebugMaterial(sphere->model->meshes[0].material, REWRITE);

	static bool NOW_Y_UP_AXIS = false;
	ImGui::Begin("Debug");
	if (ImGui::Checkbox("NowAxis is Y-Up Setting.", &NOW_Y_UP_AXIS))
	{
		nowAxis = 1 - nowAxis;
	}
	ImGui::Separator();
	ImGui::Text("- Play animation -");
	if (ImGui::Button("Translation"))axisModels[nowAxis]->animator->Play("Translation", false);
	if (ImGui::Button("Rotation"))axisModels[nowAxis]->animator->Play("Rotation", false);
	if (ImGui::Button("Scaling"))axisModels[nowAxis]->animator->Play("Scaling", false);
	ImGui::End();
}

void GameScene::OnFinalize()
{
}
