#include "PhysXWrapper.h"

#if defined INNO_PLATFORM_WIN
#include "PxPhysicsAPI.h"
#endif

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace physx;

#define PVD_HOST "127.0.0.1"

INNO_PRIVATE_SCOPE PhysXWrapperNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool createPxActor(void* component, vec4 globalPos, vec4 size);

	std::vector<PxRigidActor*> PxRigidActors;

	static PxDefaultAllocator gDefaultAllocatorCallback;
	static PxDefaultErrorCallback gDefaultErrorCallback;

	PxDefaultAllocator gAllocator;
	PxDefaultErrorCallback	gErrorCallback;

	PxFoundation* gFoundation = nullptr;
	PxPhysics* gPhysics = nullptr;
	PxPvd* gPvd = nullptr;
	PxDefaultCpuDispatcher* gDispatcher = nullptr;
	PxScene* gScene = nullptr;
	PxMaterial* gMaterial = nullptr;
}

bool PhysXWrapperNS::setup()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback,
		gDefaultErrorCallback);
	if (!gFoundation)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "PhysXWrapper: PxCreateFoundation failed!");
		return false;
	}
	bool recordMemoryAllocations = true;

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), recordMemoryAllocations, gPvd);

	if (!gPhysics)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "PhysXWrapper: PxCreatePhysics failed!");
		return false;
	}

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	gScene->addActor(*groundPlane);

	PxRigidActors.reserve(16384);
	PxRigidActors.emplace_back(groundPlane);
	return true;
}

bool PhysXWrapperNS::initialize()
{
	return true;
}

bool PhysXWrapperNS::update()
{
	gScene->simulate(1.0f / 60.0f);
	gScene->fetchResults(true);
	for (auto i : PxRigidActors)
	{
		PxTransform t = i->getGlobalPose();

		PxMat44 m = PxMat44(t);
	}
	return true;
}

bool PhysXWrapperNS::terminate()
{
	gScene->release();
	gDispatcher->release();
	gPhysics->release();
	PxPvdTransport* transport = gPvd->getTransport();
	gPvd->release();
	transport->release();

	gFoundation->release();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysXWrapper: PhysX has been terminated.");

	return true;
}

bool PhysXWrapperNS::createPxActor(void* component, vec4 globalPos, vec4 size)
{
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(size.x, size.y, size.z), *gMaterial);
	PxTransform globalTm(PxVec3(globalPos.x, globalPos.y, globalPos.z));
	PxRigidDynamic* body = gPhysics->createRigidDynamic(globalTm);
	body->userData = component;
	body->attachShape(*shape);
	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	gScene->addActor(*body);
	shape->release();
	PxRigidActors.emplace_back(body);
	return true;
}

bool PhysXWrapper::setup()
{
	return PhysXWrapperNS::setup();
}

bool PhysXWrapper::initialize()
{
	return PhysXWrapperNS::initialize();
}

bool PhysXWrapper::update()
{
	return PhysXWrapperNS::update();
}

bool PhysXWrapper::terminate()
{
	return PhysXWrapperNS::terminate();
}

bool PhysXWrapper::createPxActor(void* component, vec4 globalPos, vec4 size)
{
	return PhysXWrapperNS::createPxActor(component, globalPos, size);
}