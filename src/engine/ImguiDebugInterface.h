#pragma once
#include<list>

class ImguiDebugInterface
{
private:
	static std::list<ImguiDebugInterface*>DEBUGGERS;

public:
	static void DrawImguiDebugger()
	{
		for (auto& debugger : DEBUGGERS)
		{
			if (!debugger->active)continue;
			debugger->OnImguiDebug();
		}
	}

private:
	bool active = true;

protected:
	ImguiDebugInterface() { DEBUGGERS.emplace_back(this); }

public:
	void SetActive(const bool& Active) { active = Active; }
	virtual void OnImguiDebug() = 0;
};