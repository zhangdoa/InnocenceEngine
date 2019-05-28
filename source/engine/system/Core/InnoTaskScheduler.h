#pragma once
#include <memory>
#include <type_traits>
#include <future>

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
	}

	const char* GetName() override
	{
		return m_Name;
	}

private:
	Functor m_Functor;
	const char* m_Name;
};

struct InnoTaskReport
{
	float Duration;
	unsigned int ThreadID;
	const char* TaskName;
};

class InnoTaskScheduler
{
public:
	static bool Setup();
	static bool Initialize();
	static bool Update();
	static bool Terminate();

	static void WaitSync();

	static IInnoTask* AddTaskImpl(std::unique_ptr<IInnoTask>&& task);

	static InnoTaskReport GetReport();
};