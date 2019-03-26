#include "PhysXWrapper.h"

#if defined INNO_PLATFORM_WIN
#include "PxPhysicsAPI.h"
#endif

INNO_PRIVATE_SCOPE PhysXWrapperNS
{
	void setup();
}

void PhysXWrapperNS::setup()
{
}

void PhysXWrapper::setup()
{
	PhysXWrapperNS::setup();
}

void PhysXWrapper::initialize()
{
}

void PhysXWrapper::update()
{
}

void PhysXWrapper::terminate()
{
}
