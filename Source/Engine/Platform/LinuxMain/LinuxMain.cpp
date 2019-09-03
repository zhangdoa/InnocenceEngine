#include "../../Application/InnoApplication.h"

int main(int argc, char *argv[])
{
	if (!InnoApplication::Setup(nullptr, nullptr, argv[1]))
	{
		return 0;
	}
	if (!InnoApplication::Initialize())
	{
		return 0;
	}
	InnoApplication::Run();
	InnoApplication::Terminate();

	return 0;
}
