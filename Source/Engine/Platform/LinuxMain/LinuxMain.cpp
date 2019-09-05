#include "../ApplicationEntry/InnoApplicationEntry.h"

int main(int argc, char *argv[])
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