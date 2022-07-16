#pragma once
#include<vector>
#include<string>
class Observer;
class Subject
{
	std::vector<Observer*>m_observers;
protected:
	void Notify(const std::string& EventKey);
public:
	void AddObserver(Observer* Arg) { m_observers.emplace_back(Arg); }
	void RemoveObserver(Observer* Arg) 
	{
		for (auto itr = m_observers.begin(); itr != m_observers.end(); ++itr)
		{
			if (*itr == Arg)
			{
				m_observers.erase(itr);
				break;
			}
		}
	}
};