#include "PhysXWrapper.h"

#if defined INNO_PLATFORM_WIN
#include "PxPhysicsAPI.h"
#endif

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace physx;

INNO_PRIVATE_SCOPE PhysXWrapperNS
{
	void setup();

	static PxDefaultAllocator gDefaultAllocatorCallback;
	static PxDefaultErrorCallback gDefaultErrorCallback;

	PxDefaultAllocator		gAllocator;
	PxDefaultErrorCallback	gErrorCallback;

	PxFoundation*			gFoundation = nullptr;
	PxPhysics*				gPhysics = nullptr;
}

void PhysXWrapperNS::setup()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback,
		gDefaultErrorCallback);
	if (!gFoundation)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "PhysXWrapper: PxCreateFoundation failed!");
	}
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