#pragma once
#include <memory>
#include <type_traits>
#include <future>
#include "../Common/InnoContainer.h"

class IInnoTask
{
public:
	IInnoTask() = default;
	virtual ~IInnoTask(void) = default;
	IInnoTask(const IInnoTask& rhs) = delete;
	IInnoTask& operator=(const IInnoTask& rhs) = delete;
	IInnoTask(IInnoTask&& other) = default;
	IInnoTask& operator=(IInnoTask&& other) = default;

	virtual void Execute() = 0;
	virtual const char* GetName() = 0;
	virtual void Wait() = 0;
};

template <typename Functor>
class InnoTask : public IInnoTask
{
public:
	InnoTask(Functor&& functor, const char* name)
		:m_Functor{ std::move(functor) }, m_Name{ name }
	{
	}

	~InnoTask() override = default;
	InnoTask(const InnoTask& rhs) = delete;
	InnoTask& operator=(const InnoTask& rhs) = delete;
	InnoTask(InnoTask&& other) = default;
	InnoTask& operator=(InnoTask&& other) = default;

	void Execute() override
	{
		m_Functor();
		m_IsFinished = true;
	}

	const char* GetName() override
	{
		return m_Name;
	}

	void Wait() override
	{
		while (!m_IsFinished);
	}

private:
	Functor m_Functor;
	const char* m_Name;
	std::atomic_bool m_IsFinished = false;
};

struct InnoTaskReport
{
	unsigned long long m_StartTime;
	unsigned long long m_FinishTime;
	unsigned int m_ThreadID;
	const char* m_TaskName;
};

class InnoTaskScheduler
{
public:
	static bool Setup();
	static bool Initialize();
	static bool Update();
	static bool Terminate();

	static void WaitSync();

	static IInnoTask* AddTaskImpl(std::unique_ptr<IInnoTask>&& task, int threadID);
	static size_t GetTotalThreadsNumber();

	static const RingBuffer<InnoTaskReport, true>& GetTaskReport(int threadID);
};