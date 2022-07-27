#include "ControllerConfig.h"
#include"UsersInput.h"
#include<algorithm>

int ControllerConfig::GetAllocateButtonIdx(HANDLE_INPUT_TAG Tag)
{
	//既に割り当てられているボタンを検索
	for (int button = 0; button < static_cast<int>(CAN_ALLOCATE_BUTTON::NUM); ++button)
	{
		if (m_allocateButton[button] == Tag)
		{
			return button;
		}
	}

	//どのボタンにも割り当てられていない
	assert(0);
}

void ControllerConfig::Reset()
{
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::A)] = HANDLE_INPUT_TAG::JUMP;
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::B)] = HANDLE_INPUT_TAG::NONE;
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::Y)] = HANDLE_INPUT_TAG::GENERIC_ACTION;
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::X)] = HANDLE_INPUT_TAG::NORMAL_ATTACK;
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::LB)] = HANDLE_INPUT_TAG::RUSH;
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::RB)] = HANDLE_INPUT_TAG::MARKING;
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::LT)] = HANDLE_INPUT_TAG::GUARD_DODGE_DASH;
	m_allocateButton[static_cast<int>(CAN_ALLOCATE_BUTTON::RT)] = HANDLE_INPUT_TAG::ABILITY;
}

bool ControllerConfig::GetHandleInput(const UsersInput& Input, HANDLE_INPUT_TAG Tag)
{
	auto button = GetAllocateButton(Tag);

	if (button == CAN_ALLOCATE_BUTTON::A)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::A);
	else if (button == CAN_ALLOCATE_BUTTON::B)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::B);
	else if (button == CAN_ALLOCATE_BUTTON::Y)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::Y);
	else if (button == CAN_ALLOCATE_BUTTON::X)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::X);
	else if (button == CAN_ALLOCATE_BUTTON::LB)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::LB);
	else if (button == CAN_ALLOCATE_BUTTON::RB)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::RB);
	else if (button == CAN_ALLOCATE_BUTTON::LT)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::LT);
	else if (button == CAN_ALLOCATE_BUTTON::RT)return Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::RT);

	assert(0);
	return false;
}

Vec2<float> ControllerConfig::GetLeftStickVec(const UsersInput& Input)
{
	return Input.GetLeftStickVec(m_controllerIdx);
}

Vec2<float> ControllerConfig::GetRightStickVec(const UsersInput& Input)
{
	return Input.GetRightStickVec(m_controllerIdx);
}

Vec2<int>ControllerConfig::GetDpadInput(const UsersInput& Input)
{
	Vec2<int>result = { 0,0 };
	if (Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::DPAD_LEFT))result.x = -1;
	else if (Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::DPAD_RIGHT))result.x = 1;
	if (Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::DPAD_DOWN))result.y = -1;
	else if (Input.ControllerInput(m_controllerIdx, XBOX_BUTTON::DPAD_UP))result.y = 1;
	return result;
}


void ControllerConfig::ShakeController(UsersInput& Input, float Power, int Span)
{
	Power = std::clamp(Power, 0.0f, 1.0f);
	Input.ShakeController(m_controllerIdx, Power, Span);
}

void ControllerConfig::ReAllocateButton(HANDLE_INPUT_TAG Tag, CAN_ALLOCATE_BUTTON AllocateButton)
{
	//割り当ての入れ替え
	m_allocateButton[GetAllocateButtonIdx(Tag)] = m_allocateButton[static_cast<int>(AllocateButton)];
	m_allocateButton[static_cast<int>(AllocateButton)] = Tag;
}

#include"imguiApp.h"
void ControllerConfig::ImguiDebug(UsersInput& Input)
{
	static std::array<std::string, static_cast<int>(HANDLE_INPUT_TAG::NUM)>TAG_NAMES =
	{
		"NormalAttack",
		"Jump",
		"Guard / Dodge / DASH",
		"Marking",
		"Rush",
		"Ability",
		"GenericAction"
	};
	static std::array<std::string, static_cast<int>(CAN_ALLOCATE_BUTTON::NUM)>ALLOCATE_BUTTON_NAME =
	{
		"A","B","Y","X","LB","RB","LT","RT"
	};

	ImGui::Begin("ControllerConfig");

	//ボタン割り当て
	for (int tagIdx = 0; tagIdx < static_cast<int>(HANDLE_INPUT_TAG::NUM); ++tagIdx)
	{
		//タグ取得
		auto tag = static_cast<HANDLE_INPUT_TAG>(tagIdx);
		//割り当てているボタンのインデックス取得
		int allocateButtonIdx = GetAllocateButtonIdx(tag);

		if (ImGui::BeginCombo(TAG_NAMES[tagIdx].c_str(), ALLOCATE_BUTTON_NAME[allocateButtonIdx].c_str()))
		{
			for (int buttonIdx = 0; buttonIdx < static_cast<int>(CAN_ALLOCATE_BUTTON::NUM); ++buttonIdx)
			{
				bool isSelected = (allocateButtonIdx == buttonIdx);
				if (ImGui::Selectable(ALLOCATE_BUTTON_NAME[buttonIdx].c_str(), isSelected))
				{
					//ボタンの割り当て
					ReAllocateButton(tag, static_cast<CAN_ALLOCATE_BUTTON>(buttonIdx));
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	ImGui::Separator();

	//入力情報表示
	static auto WHITE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	static auto RED = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(250, 100), ImGuiWindowFlags_NoTitleBar);

	//コントローラー番号
	ImGui::Text("ControllerIdx : %d", m_controllerIdx);
	ImGui::Separator();

	//左スティック入力（通常移動）
	ImGui::Text("Move : ");
	ImGui::SameLine();
	auto lStickVec = GetLeftStickVec(Input);
	ImGui::TextColored(lStickVec.IsZero() ? WHITE : RED, "{ %f , %f }", lStickVec.x, lStickVec.y);

	//右スティック入力（カメラ操作）
	ImGui::Text("CameraMove : ");
	ImGui::SameLine();
	auto rStickVec = GetRightStickVec(Input);
	ImGui::TextColored(rStickVec.IsZero() ? WHITE : RED, "{ %f , %f }", rStickVec.x, rStickVec.y);

	//アビリティ選択
	ImGui::Text("AbilitySelect : ");
	ImGui::SameLine();
	auto dpadVec = GetDpadInput(Input);
	ImGui::TextColored(dpadVec.IsZero() ? WHITE : RED, "{ %d , %d }", dpadVec.x, dpadVec.y);

	//ボタンが割り当てられた操作の入力表示
	ImGui::Separator();
	for (int tagIdx = 0; tagIdx < static_cast<int>(HANDLE_INPUT_TAG::NUM); ++tagIdx)
	{
		ImGui::Text("%s : ", TAG_NAMES[tagIdx].c_str());
		ImGui::SameLine();

		//タグ取得
		auto tag = static_cast<HANDLE_INPUT_TAG>(tagIdx);
		bool input = GetHandleInput(Input, tag);
		ImGui::TextColored(input ? RED : WHITE, "{ %s }", input ? "TRUE" : "FALSE");
	}
	ImGui::EndChild();

	ImGui::Separator();
	//コントローラーを振動させるボタン
	static float s_power = 1.0f;
	static int s_span = 15;
	if (ImGui::Button("Shake"))
	{
		Input.ShakeController(m_controllerIdx, s_power, s_span);
	}
	ImGui::DragFloat("Power", &s_power, 0.05f, 0.0f, 1.0f);
	ImGui::DragInt("Span", &s_span, 1, 0, 120);

	ImGui::End();
}
