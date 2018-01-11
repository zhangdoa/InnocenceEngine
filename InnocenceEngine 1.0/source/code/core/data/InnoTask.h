#pragma once
class InnoTask
{
public:
	InnoTask();
	~InnoTask();

private:
	std::function<void()> m_functor;
};

