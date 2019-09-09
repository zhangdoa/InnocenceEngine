#include "../ApplicationEntry/InnoApplicationEntry.h"

int32_t main(int32_t argc, char *argv[])
{
	if (!InnoApplicationEntry::Setup(nullptr, nullptr, argv[1]))
	{
		return 0;
	}
	if (!InnoApplicationEntry::Initialize())
	{
		return 0;
	}
	InnoApplicationEntry::Run();
	InnoApplicationEntry::Terminate();

	return 0;
}