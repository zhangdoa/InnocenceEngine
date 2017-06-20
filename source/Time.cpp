#include "Time.h"

Time::Time() {
}
Time::~Time() {}

const double Time::getTime() {
	return 1.0;
}

const double Time::getDelta()
{

	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::duration<float, std::milli> duration;

	static clock::time_point start = clock::now();
	duration elapsed = clock::now() - start;
	return elapsed.count();

}
