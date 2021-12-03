#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <functional>
#include <queue>


class ThreadPool
{
public:
	typedef std::function<void()> Action;

	ThreadPool(int threadCount);
	~ThreadPool();
	int getThreadCount() const;
	bool getWorkStatus() const;
	void pushRoutine(Action action);
	void finish();

private:
	struct RoutineInfo
	{
		bool* pWorking;
		Action* pAction;
		LPCRITICAL_SECTION pCriticalSection;
	};

	int threadCount;

	bool working;

	std::vector<HANDLE> threads;

	std::vector<Action> routines;
	HANDLE queueThread;
	CRITICAL_SECTION CriticalSection;
	CRITICAL_SECTION criticalSections1;

	std::queue<Action> routineQueue;

	static DWORD WINAPI threadRoutine(LPVOID pParameter);

	static DWORD WINAPI popRoutines(LPVOID pParameter);
};

