#include "../ApplicationEntry/ApplicationEntry.h"

int32_t main(int32_t argc, char* argv[])
{
	if (!ApplicationEntry::Setup(nullptr, nullptr, argv[1]))
	{
		return 0;
	}
	if (!ApplicationEntry::Initialize())
	{
		return 0;
	}
	ApplicationEntry::Run();
	ApplicationEntry::Terminate();

	return 0;
}