#pragma once
#include<list>

class ImguiDebugInterface
{
private:
	static std::list<ImguiDebugInterface*>s_debuggerList;

public:
	static void DrawImguiDebugger()
	{
		for (auto& debugger : s_debuggerList)
		{
			if (!debugger->m_active)continue;
			debugger->OnImguiDebug();
		}
	}

private:
	bool m_active = true;

protected:
	ImguiDebugInterface() { s_debuggerList.emplace_back(this); }

public:
	void SetActive(const bool& Active) { m_active = Active; }
	virtual void OnImguiDebug() = 0;
};