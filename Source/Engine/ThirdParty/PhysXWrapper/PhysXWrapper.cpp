#include "PhysXWrapper.h"

#if defined INNO_PLATFORM_WIN
#include "PxPhysicsAPI.h"
#endif

#include "../../Core/InnoLogger.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

using namespace physx;

#define PVD_HOST "127.0.0.1"

struct PhysXActor
{
	bool isDynamic = false;
	PxRigidActor* m_PxRigidActor = 0;
};

namespace PhysXWrapperNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool createPxSphere(PhysicsDataComponent* rhs, Vec4 globalPos, float radius, bool isDynamic);
	bool createPxBox(PhysicsDataComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic);

	std::vector<PhysXActor> PhysXActors;

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

	bool m_needSimulate = false;
	std::atomic<bool> m_allowUpdate = true;
	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_pauseSimulate;

	std::shared_ptr<IInnoTask> m_currentTask;
	std::mutex m_mutex;
}

bool PhysXWrapperNS::setup()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback,
		gDefaultErrorCallback);
	if (!gFoundation)
	{
		InnoLogger::Log(LogLevel::Error, "PhysXWrapper: PxCreateFoundation failed!");
		return false;
	}
	bool recordMemoryAllocations = true;

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), recordMemoryAllocations, gPvd);

	if (!gPhysics)
	{
		InnoLogger::Log(LogLevel::Error, "PhysXWrapper: PxCreatePhysics failed!");
		return false;
	}

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(0);
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

	PhysXActors.reserve(65536);

	f_sceneLoadingStartCallback = [&]() {
		m_needSimulate = false;

		if (m_currentTask != nullptr)
		{
			m_currentTask->Wait();
		}

		for (auto i : PhysXActors)
		{
			gScene->removeActor(*i.m_PxRigidActor);
		}

		PhysXActors.clear();

		InnoLogger::Log(LogLevel::Success, "PhysXWrapper: All PhysX Actors has been removed.");
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	f_pauseSimulate = [&]() { m_needSimulate = !m_needSimulate; };

	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_P, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseSimulate });

	return true;
}

bool PhysXWrapperNS::initialize()
{
	return true;
}

bool PhysXWrapperNS::update()
{
	if (m_needSimulate)
	{
		if (m_allowUpdate)
		{
			m_allowUpdate = false;

			m_currentTask = g_pModuleManager->getTaskSystem()->submit("PhysXUpdateTask", 3, nullptr, [&]()
				{
					gScene->simulate(g_pModuleManager->getTickTime() / 1000.0f);
					gScene->fetchResults(true);

					for (auto i : PhysXActors)
					{
						if (i.isDynamic)
						{
							PxTransform t = i.m_PxRigidActor->getGlobalPose();
							PxVec3 p = t.p;
							PxQuat q = t.q;

							auto l_rigidBody = reinterpret_cast<PxRigidDynamic*>(i.m_PxRigidActor);

							if (l_rigidBody->userData)
							{
								auto l_PDC = reinterpret_cast<PhysicsDataComponent*>(l_rigidBody->userData);
								auto l_transformComponent = l_PDC->m_TransformComponent;
								l_transformComponent->m_localTransformVector_target.m_pos = Vec4(p.x, p.y, p.z, 1.0f);
								l_transformComponent->m_localTransformVector_target.m_rot = Vec4(q.x, q.y, q.z, q.w);
								l_transformComponent->m_localTransformVector = l_transformComponent->m_localTransformVector_target;
							}
						}
					}
					m_allowUpdate = true;
				});
		}
	}

	return true;
}

bool PhysXWrapperNS::terminate()
{
	if (m_currentTask != nullptr)
	{
		m_currentTask->Wait();
	}
	gScene->release();
	gDispatcher->release();
	gPhysics->release();
	PxPvdTransport* transport = gPvd->getTransport();
	gPvd->release();
	transport->release();

	gFoundation->release();

	InnoLogger::Log(LogLevel::Success, "PhysXWrapper: PhysX has been terminated.");

	return true;
}

bool PhysXWrapperNS::createPxSphere(PhysicsDataComponent* rhs, Vec4 globalPos, float radius, bool isDynamic)
{
	std::lock_guard<std::mutex> lock{ PhysXWrapperNS::m_mutex };
	PxRigidActor* l_actor;

	PxShape* shape = gPhysics->createShape(PxSphereGeometry(radius), *gMaterial);
	PxTransform globalTm(PxVec3(globalPos.x, globalPos.y, globalPos.z));

	if (isDynamic)
	{
		PxRigidDynamic* body = gPhysics->createRigidDynamic(globalTm);
		body->userData = rhs;
		body->attachShape(*shape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		l_actor = body;
	}
	else
	{
		PxRigidStatic* body = gPhysics->createRigidStatic(globalTm);
		body->userData = rhs;
		body->attachShape(*shape);
		l_actor = body;
	}

	rhs->m_Proxy = l_actor;
	gScene->addActor(*l_actor);
	shape->release();
	PhysXActors.emplace_back(PhysXActor{ isDynamic, l_actor });
	InnoLogger::Log(LogLevel::Verbose, "PhysXWrapper: PxRigidActor has been created for ", rhs, ".");

	return true;
}

bool PhysXWrapperNS::createPxBox(PhysicsDataComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic)
{
	std::lock_guard<std::mutex> lock{ PhysXWrapperNS::m_mutex };

	if (size.x > 0 && size.y > 0 && size.z > 0)
	{
		PxRigidActor* l_actor;

		PxShape* shape = gPhysics->createShape(PxBoxGeometry(size.x, size.y, size.z), *gMaterial);

		PxTransform globalTm(PxVec3(globalPos.x, globalPos.y, globalPos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));

		if (isDynamic)
		{
			PxRigidDynamic* body = gPhysics->createRigidDynamic(globalTm);
			body->userData = rhs;
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			l_actor = body;
		}
		else
		{
			PxRigidStatic* body = gPhysics->createRigidStatic(globalTm);
			body->userData = rhs;
			body->attachShape(*shape);

			l_actor = body;
		}

		rhs->m_Proxy = l_actor;
		gScene->addActor(*l_actor);
		shape->release();
		PhysXActors.emplace_back(PhysXActor{ isDynamic, l_actor });
		InnoLogger::Log(LogLevel::Verbose, "PhysXWrapper: PxRigidActor has been created for ", rhs, ".");
	}

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

bool PhysXWrapper::createPxSphere(PhysicsDataComponent* rhs, Vec4 globalPos, float radius, bool isDynamic)
{
	return PhysXWrapperNS::createPxSphere(rhs, globalPos, radius, isDynamic);
}

bool PhysXWrapper::createPxBox(PhysicsDataComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic)
{
	return PhysXWrapperNS::createPxBox(rhs, globalPos, rot, size, isDynamic);
}

bool PhysXWrapper::addForce(PhysicsDataComponent* rhs, Vec4 force)
{
	auto l_rigidBody = reinterpret_cast<PxRigidDynamic*>(rhs->m_Proxy);
	l_rigidBody->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE);
	return true;
}