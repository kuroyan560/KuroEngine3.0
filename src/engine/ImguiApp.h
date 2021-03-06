#pragma once
#include<wrl.h>
#include<d3d12.h>
#include"imgui.h"
#include"imgui_impl_win32.h"
#include"imgui_impl_dx12.h"
#include<stdio.h>
#include<array>
#include<string>

static const enum IMGUI_DEBUG_MODE
{
	REFERENCE,	//参照（見るだけ）
	REWRITE,	//書き換え
	IMGUI_DEBUG_MODE_NUM
};

#include<memory>
class Material;

class ImguiApp
{
private:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	static ImguiApp* INSTANCE;

public:
	static ImguiApp* Instance()
	{
		if (INSTANCE == nullptr)
		{
			printf("ImguiAppのインスタンスを呼び出そうとしましたがnullptrでした\n");
			assert(0);
		}
		return INSTANCE;
	}

private:
	ComPtr<ID3D12DescriptorHeap>heap;

	const std::array<std::string, IMGUI_DEBUG_MODE_NUM>modeName =
	{
		"Reference",
		"Rewrite"
	};
	const float indent = 32.0f;

public:
	ImguiApp(const ComPtr<ID3D12Device>& Device, const HWND& Hwnd);
	~ImguiApp();
	void BeginImgui();
	void EndImgui(const ComPtr<ID3D12GraphicsCommandList>& CmdList);

	//マテリアル情報調整
	void DebugMaterial(std::weak_ptr<Material>Material, const IMGUI_DEBUG_MODE& Mode);
};

