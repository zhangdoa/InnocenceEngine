// InnocenceEngine 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../core/data/InnoMath.h"
#include "../core/manager/CoreManager.h"
#include "../game/InnocenceGarden.h"

auto temp1 = vec4<double>(1.0, 0.0, 0.0, 0.0);
auto temp2 = vec4<double>(0.0, 1.0, 0.0, 0.0);

std::vector<int> primeNumberPool;
int initial = 100000;
void asyncPrimeNumberCalculation()
{
	for (auto i = 2; i < initial; i++)
	{
		bool isPrime = true;
		for (auto j = 2; j < i; j++)
		{
			if (i % j == 0)
			{
				isPrime = false;
			}
		}
		if (isPrime)
		{
			primeNumberPool.emplace_back(i);
			std::cout << "The prime is " << i << ", " << std::this_thread::get_id() << " said primeNumberPool size is " << primeNumberPool.size() << std::endl;
		}
	}
}

int main()
{

	InnocenceGarden* m_innocenceGarden = new InnocenceGarden();

	CoreManager::getInstance().setGameData(m_innocenceGarden);

	//auto f = std::async(std::launch::async, asyncPrimeNumberCalculation);

	std::cout << temp1.cross(temp2).m_x<<temp1.cross(temp2).m_y<<temp1.cross(temp2).m_z<<temp1.cross(temp2).m_w << std::endl;

	CoreManager::getInstance().setup();
	CoreManager::getInstance().initialize();

	while (CoreManager::getInstance().getStatus() == objectStatus::ALIVE)
	{
		CoreManager::getInstance().update();
	}
	CoreManager::getInstance().shutdown();
	delete m_innocenceGarden;

	return EXIT_SUCCESS;
}

