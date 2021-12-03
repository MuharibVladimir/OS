#include "pch.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(const int threadCount)
{
	if (threadCount < 1)
	{
		throw std::invalid_argument("Number of threads must be greater than 0");
	}

	this->threadCount = threadCount;
	InitializeCriticalSection(&CriticalSection);
	InitializeCriticalSection(&criticalSections1);

	threads.resize(threadCount);
	routines.resize(threadCount);
	working = true;
	for (auto i = 0; i < threadCount; i++)
	{
		const auto pRoutineInfo = new RoutineInfo{ &working, &routines[i], &criticalSections1 };
		threads[i] = CreateThread(nullptr, 0, threadRoutine, pRoutineInfo, 0, nullptr);
	}
	queueThread = CreateThread(nullptr, 0, popRoutines, this, 0, nullptr);
}

ThreadPool::~ThreadPool()
{
}

int ThreadPool::getThreadCount() const
{
	return threadCount;
}

bool ThreadPool::getWorkStatus() const
{
	return working;
}

void ThreadPool::pushRoutine(Action action)
{
	routineQueue.push(action);
}

void ThreadPool::finish()
{
	bool noTasks;
	do
	{
		noTasks = routineQueue.empty();
		if (noTasks)
		{
			for (int i = 0; i < threadCount; i++)
			{
				noTasks = noTasks && routines[i] == nullptr;
			}
		}

	} while (!noTasks);

	working = false;
}

DWORD ThreadPool::threadRoutine(LPVOID pParameter)
{
	RoutineInfo* pInfo = reinterpret_cast<RoutineInfo*>(pParameter);
	while (*(pInfo->pWorking))
	{
		EnterCriticalSection(pInfo->pCriticalSection);
		if (*(pInfo->pAction) != nullptr)
		{
			(*(pInfo->pAction))();
			*(pInfo->pAction) = nullptr;
		}
		LeaveCriticalSection(pInfo->pCriticalSection);
	}
	delete(pInfo);
	return 0;
}

DWORD ThreadPool::popRoutines(LPVOID pParameter)
{
	ThreadPool* pThreadPool = reinterpret_cast<ThreadPool*>(pParameter);
	while (pThreadPool->working)
	{
		for (int i = 0; i < pThreadPool->threadCount; i++)
		{
			if (TryEnterCriticalSection(&pThreadPool->CriticalSection))
			{
				if (pThreadPool->routines[i] == nullptr)
				{
					if (!pThreadPool->routineQueue.empty())
					{
						pThreadPool->routines[i] = pThreadPool->routineQueue.front();
						pThreadPool->routineQueue.pop();
					}
				}
				LeaveCriticalSection(&pThreadPool->CriticalSection);
			}
		}
	}
	return 0;
}