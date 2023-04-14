#include "PhysXWrapper.h"

#if defined INNO_PLATFORM_WIN
#include "PxPhysicsAPI.h"
#endif

#include "../../Core/InnoLogger.h"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

using namespace physx;

#define PVD_HOST "127.0.0.1"

struct PhysXActor
{
	bool isDynamic = false;
	PxRigidActor* m_PxRigidActor = 0;
};

namespace PhysXWrapperNS
{
	bool Setup();
	bool Initialize();
	bool Update();
	bool Terminate();

	bool createPxSphere(PhysicsComponent* rhs, Vec4 globalPos, float radius, bool isDynamic);
	bool createPxBox(PhysicsComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic);
	PxConvexMesh* createPxConvexMesh(PhysicsComponent* rhs, PxConvexMeshCookingType::Enum convexMeshCookingType, bool directInsertion, PxU32 gaussMapLimit);
	PxTriangleMesh* createBV33TriangleMesh(PhysicsComponent* rhs, bool skipMeshCleanup, bool skipEdgeData, bool inserted, bool cookingPerformance, bool meshSizePerfTradeoff);
	PxTriangleMesh* createBV34TriangleMesh(PhysicsComponent* rhs, bool skipMeshCleanup, bool skipEdgeData, bool inserted, const PxU32 numTrisPerLeaf);

	bool createPxMesh(PhysicsComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic, bool isConvex);

	std::unordered_map<MeshComponent*, PxConvexMesh*> PhysXConvexMeshes;
	std::unordered_map<MeshComponent*, PxTriangleMesh*> PhysXTriangleMeshes;

	std::vector<PhysXActor> PhysXActors;

	static PxDefaultAllocator gDefaultAllocatorCallback;
	static PxDefaultErrorCallback gDefaultErrorCallback;

	PxDefaultAllocator gAllocator;
	PxDefaultErrorCallback	gErrorCallback;

	PxFoundation* gFoundation = nullptr;
	PxPhysics* gPhysics = nullptr;
	PxCooking* gCooking = nullptr;
	PxPvd* gPvd = nullptr;
	PxDefaultCpuDispatcher* gDispatcher = nullptr;
	PxScene* gScene = nullptr;
	PxMaterial* gMaterial = nullptr;

	bool m_needSimulate = false;
	std::atomic<bool> m_allowUpdate = true;
	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_pauseSimulate;

	std::mutex m_mutex;
}

bool PhysXWrapperNS::Setup()
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

	gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(PxTolerancesScale()));

	if (!gCooking)
	{
		InnoLogger::Log(LogLevel::Error, "PhysXWrapper: PxCreateCooking failed!");
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

	auto groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	gScene->addActor(*groundPlane);

	PhysXActors.reserve(65536);

	f_sceneLoadingStartCallback = [&]() {
		m_needSimulate = false;

		for (auto i : PhysXActors)
		{
			gScene->removeActor(*i.m_PxRigidActor);
		}

		PhysXActors.clear();

		InnoLogger::Log(LogLevel::Success, "PhysXWrapper: All PhysX Actors has been removed.");
	};

	g_Engine->getSceneSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	f_pauseSimulate = [&]() { m_needSimulate = !m_needSimulate; };

	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_P, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseSimulate });

	return true;
}

bool PhysXWrapperNS::Initialize()
{
	return true;
}

bool PhysXWrapperNS::Update()
{
	if (m_needSimulate)
	{
		if (m_allowUpdate)
		{
			m_allowUpdate = false;

			g_Engine->getTaskSystem()->Submit("PhysXUpdateTask", 3, nullptr, [&]()
				{
					gScene->simulate(g_Engine->getTickTime() / 1000.0f);
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
								auto l_PDC = reinterpret_cast<PhysicsComponent*>(l_rigidBody->userData);
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

bool PhysXWrapperNS::Terminate()
{
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

bool PhysXWrapperNS::createPxSphere(PhysicsComponent* rhs, Vec4 globalPos, float radius, bool isDynamic)
{
	std::lock_guard<std::mutex> lock{ PhysXWrapperNS::m_mutex };
	PxRigidActor* l_actor;

	auto shape = gPhysics->createShape(PxSphereGeometry(radius), *gMaterial);
	PxTransform globalTm(PxVec3(globalPos.x, globalPos.y, globalPos.z));

	if (isDynamic)
	{
		auto body = gPhysics->createRigidDynamic(globalTm);
		body->userData = rhs;
		body->attachShape(*shape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		l_actor = body;
	}
	else
	{
		auto body = gPhysics->createRigidStatic(globalTm);
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

bool PhysXWrapperNS::createPxBox(PhysicsComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic)
{
	std::lock_guard<std::mutex> lock{ PhysXWrapperNS::m_mutex };

	if (size.x > 0 && size.y > 0 && size.z > 0)
	{
		PxRigidActor* l_actor;

		auto shape = gPhysics->createShape(PxBoxGeometry(size.x, size.y, size.z), *gMaterial);

		PxTransform globalTm(PxVec3(globalPos.x, globalPos.y, globalPos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));

		if (isDynamic)
		{
			auto body = gPhysics->createRigidDynamic(globalTm);
			body->userData = rhs;
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			l_actor = body;
		}
		else
		{
			auto body = gPhysics->createRigidStatic(globalTm);
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

PxConvexMesh* PhysXWrapperNS::createPxConvexMesh(PhysicsComponent* rhs, PxConvexMeshCookingType::Enum convexMeshCookingType, bool directInsertion, PxU32 gaussMapLimit)
{
	auto l_result = PhysXConvexMeshes.find(rhs->m_MeshMaterialPair->mesh);
	if (l_result != PhysXConvexMeshes.end())
	{
		return l_result->second;
	}

	PxCookingParams params = gCooking->getParams();

	// Use the new (default) PxConvexMeshCookingType::eQUICKHULL
	params.convexMeshCookingType = convexMeshCookingType;

	// If the gaussMapLimit is chosen higher than the number of output vertices, no gauss map is added to the convex mesh data.
	// If the gaussMapLimit is chosen lower than the number of output vertices, a gauss map is added to the convex mesh data.
	params.gaussMapLimit = gaussMapLimit;
	gCooking->setParams(params);

	// Setup the convex mesh descriptor
	PxConvexMeshDesc desc;

	// We provide points only, therefore the PxConvexFlag::eCOMPUTE_CONVEX flag must be specified
	desc.points.data = &rhs->m_MeshMaterialPair->mesh->m_Vertices[0];
	desc.points.count = (PxU32)rhs->m_MeshMaterialPair->mesh->m_Vertices.size();
	desc.points.stride = sizeof(Vertex);
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxU32 meshSize = 0;
	PxConvexMesh* convex = nullptr;

	auto startTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();

	if (directInsertion)
	{
		// Directly insert mesh into PhysX
		convex = gCooking->createConvexMesh(desc, gPhysics->getPhysicsInsertionCallback());
		PX_ASSERT(convex);
	}
	else
	{
		// Serialize the cooked mesh into a stream.
		PxDefaultMemoryOutputStream outStream;
		bool res = gCooking->cookConvexMesh(desc, outStream);
		PX_UNUSED(res);
		PX_ASSERT(res);
		meshSize = outStream.getSize();

		// Create the mesh from a stream.
		PxDefaultMemoryInputData inStream(outStream.getData(), outStream.getSize());
		convex = gPhysics->createConvexMesh(inStream);
		PX_ASSERT(convex);
	}

	// Print the elapsed time for comparison
	auto stopTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();
	auto elapsedTime = stopTime - startTime;
	InnoLogger::Log(LogLevel::Verbose, "Create convex mesh with ", desc.points.count, " triangles: ");
	directInsertion ? InnoLogger::Log(LogLevel::Verbose, "Direct mesh insertion enabled.") : InnoLogger::Log(LogLevel::Verbose, "Direct mesh insertion disabled.");
	InnoLogger::Log(LogLevel::Verbose, "Gauss map limit: %d \n", gaussMapLimit);
	InnoLogger::Log(LogLevel::Verbose, "Created hull number of vertices: ", convex->getNbVertices());
	InnoLogger::Log(LogLevel::Verbose, "Created hull number of polygons: ", convex->getNbPolygons());
	InnoLogger::Log(LogLevel::Verbose, "Elapsed time in ms: ", double(elapsedTime));
	if (!directInsertion)
	{
		InnoLogger::Log(LogLevel::Verbose, "Mesh size: ", meshSize);
	}

	PhysXConvexMeshes.emplace(rhs->m_MeshMaterialPair->mesh, convex);

	return convex;
}

// Setup common cooking params
void setupCommonCookingParams(PxCookingParams& params, bool skipMeshCleanup, bool skipEdgeData)
{
	// we suppress the triangle mesh remap table computation to gain some speed, as we will not need it
	// in this snippet
	params.suppressTriangleMeshRemapTable = true;

	// If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking. The input mesh must be valid.
	// The following conditions are true for a valid triangle mesh :
	//  1. There are no duplicate vertices(within specified vertexWeldTolerance.See PxCookingParams::meshWeldTolerance)
	//  2. There are no large triangles(within specified PxTolerancesScale.)
	// It is recommended to run a separate validation check in debug/checked builds, see below.

	if (!skipMeshCleanup)
		params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
	else
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;

	// If DISABLE_ACTIVE_EDGES_PREDOCOMPUTE is set, the cooking does not compute the active (convex) edges, and instead
	// marks all edges as active. This makes cooking faster but can slow down contact generation. This flag may change
	// the collision behavior, as all edges of the triangle mesh will now be considered active.
	if (!skipEdgeData)
		params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
	else
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
}

// Creates a triangle mesh using BVH33 midphase with different settings.
PxTriangleMesh* PhysXWrapperNS::createBV33TriangleMesh(PhysicsComponent* rhs, bool skipMeshCleanup, bool skipEdgeData, bool inserted, bool cookingPerformance, bool meshSizePerfTradeoff)
{
	auto l_result = PhysXTriangleMeshes.find(rhs->m_MeshMaterialPair->mesh);
	if (l_result != PhysXTriangleMeshes.end())
	{
		return l_result->second;
	}

	auto startTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = (PxU32)rhs->m_MeshMaterialPair->mesh->m_Vertices.size();
	meshDesc.points.data = &rhs->m_MeshMaterialPair->mesh->m_Vertices[0];
	meshDesc.points.stride = sizeof(PxVec4);
	meshDesc.triangles.count = (PxU32)rhs->m_MeshMaterialPair->mesh->m_IndexCount / 3;
	meshDesc.triangles.data = &rhs->m_MeshMaterialPair->mesh->m_Indices[0];
	meshDesc.triangles.stride = 3 * sizeof(PxU32);

	PxCookingParams params = gCooking->getParams();

	// Create BVH33 midphase
	params.midphaseDesc = PxMeshMidPhase::eBVH33;

	// Setup common cooking params
	setupCommonCookingParams(params, skipMeshCleanup, skipEdgeData);

	// The COOKING_PERFORMANCE flag for BVH33 midphase enables a fast cooking path at the expense of somewhat lower quality BVH construction.
	if (cookingPerformance)
		params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
	else
		params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eSIM_PERFORMANCE;

	// If meshSizePerfTradeoff is set to true, smaller mesh cooked mesh is produced. The mesh size/performance trade-off
	// is controlled by setting the meshSizePerformanceTradeOff from 0.0f (smaller mesh) to 1.0f (larger mesh).
	if (meshSizePerfTradeoff)
	{
		params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.0f;
	}
	else
	{
		// using the default value
		params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.55f;
	}

	gCooking->setParams(params);

#if defined(PX_CHECKED) || defined(PX_DEBUG)
	// If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking.
	// We should check the validity of provided triangles in debug/checked builds though.
	if (skipMeshCleanup)
	{
		PX_ASSERT(gCooking->validateTriangleMesh(meshDesc));
	}
#endif // DEBUG

	PxTriangleMesh* triMesh = 0;
	PxU32 meshSize = 0;

	// The cooked mesh may either be saved to a stream for later loading, or inserted directly into PxPhysics.
	if (inserted)
	{
		triMesh = gCooking->createTriangleMesh(meshDesc, gPhysics->getPhysicsInsertionCallback());
	}
	else
	{
		PxDefaultMemoryOutputStream outBuffer;
		gCooking->cookTriangleMesh(meshDesc, outBuffer);

		PxDefaultMemoryInputData stream(outBuffer.getData(), outBuffer.getSize());
		triMesh = gPhysics->createTriangleMesh(stream);

		meshSize = outBuffer.getSize();
	}

	// Print the elapsed time for comparison
	auto stopTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();
	auto elapsedTime = stopTime - startTime;
	InnoLogger::Log(LogLevel::Verbose, "\t -----------------------------------------------\n");
	InnoLogger::Log(LogLevel::Verbose, "\t Create triangle mesh with %d triangles: \n", rhs->m_MeshMaterialPair->mesh->m_IndexCount / 3);
	cookingPerformance ? InnoLogger::Log(LogLevel::Verbose, "\t\t Cooking performance on\n") : InnoLogger::Log(LogLevel::Verbose, "\t\t Cooking performance off\n");
	inserted ? InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh inserted on\n") : InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh inserted off\n");
	!skipEdgeData ? InnoLogger::Log(LogLevel::Verbose, "\t\t Precompute edge data on\n") : InnoLogger::Log(LogLevel::Verbose, "\t\t Precompute edge data off\n");
	!skipMeshCleanup ? InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh cleanup on\n") : InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh cleanup off\n");
	InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh size/performance trade-off: %f \n", double(params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff));
	InnoLogger::Log(LogLevel::Verbose, "\t Elapsed time in ms: %f \n", double(elapsedTime));
	if (!inserted)
	{
		InnoLogger::Log(LogLevel::Verbose, "\t Mesh size: %d \n", meshSize);
	}

	PhysXTriangleMeshes.emplace(rhs->m_MeshMaterialPair->mesh, triMesh);

	return triMesh;
}

// Creates a triangle mesh using BVH34 midphase with different settings.
PxTriangleMesh* PhysXWrapperNS::createBV34TriangleMesh(PhysicsComponent* rhs, bool skipMeshCleanup, bool skipEdgeData, bool inserted, const PxU32 numTrisPerLeaf)
{
	auto l_result = PhysXTriangleMeshes.find(rhs->m_MeshMaterialPair->mesh);
	if (l_result != PhysXTriangleMeshes.end())
	{
		return l_result->second;
	}

	auto startTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = (PxU32)rhs->m_MeshMaterialPair->mesh->m_Vertices.size();
	meshDesc.points.data = &rhs->m_MeshMaterialPair->mesh->m_Vertices[0];
	meshDesc.points.stride = sizeof(Vertex);
	meshDesc.triangles.count = (PxU32)rhs->m_MeshMaterialPair->mesh->m_IndexCount / 3;
	meshDesc.triangles.data = &rhs->m_MeshMaterialPair->mesh->m_Indices[0];
	meshDesc.triangles.stride = 3 * sizeof(Index);

	PxCookingParams params = gCooking->getParams();

	// Create BVH34 midphase
	params.midphaseDesc = PxMeshMidPhase::eBVH34;

	// Setup common cooking params
	setupCommonCookingParams(params, skipMeshCleanup, skipEdgeData);

	// Cooking mesh with less triangles per leaf produces larger meshes with better runtime performance
	// and worse cooking performance. Cooking time is better when more triangles per leaf are used.
	params.midphaseDesc.mBVH34Desc.numPrimsPerLeaf = numTrisPerLeaf;

	gCooking->setParams(params);

#if defined(PX_CHECKED) || defined(PX_DEBUG)
	// If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking.
	// We should check the validity of provided triangles in debug/checked builds though.
	if (skipMeshCleanup)
	{
		PX_ASSERT(gCooking->validateTriangleMesh(meshDesc));
	}
#endif // DEBUG

	PxTriangleMesh* triMesh = 0;
	PxU32 meshSize = 0;

	// The cooked mesh may either be saved to a stream for later loading, or inserted directly into PxPhysics.
	if (inserted)
	{
		triMesh = gCooking->createTriangleMesh(meshDesc, gPhysics->getPhysicsInsertionCallback());
	}
	else
	{
		PxDefaultMemoryOutputStream outBuffer;
		gCooking->cookTriangleMesh(meshDesc, outBuffer);

		PxDefaultMemoryInputData stream(outBuffer.getData(), outBuffer.getSize());
		triMesh = gPhysics->createTriangleMesh(stream);

		meshSize = outBuffer.getSize();
	}

	// Print the elapsed time for comparison
	auto stopTime = g_Engine->getTimeSystem()->getCurrentTimeFromEpoch();
	auto elapsedTime = stopTime - startTime;
	InnoLogger::Log(LogLevel::Verbose, "\t -----------------------------------------------\n");
	InnoLogger::Log(LogLevel::Verbose, "\t Create triangle mesh with %d triangles: \n", rhs->m_MeshMaterialPair->mesh->m_IndexCount / 3);
	inserted ? InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh inserted on\n") : InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh inserted off\n");
	!skipEdgeData ? InnoLogger::Log(LogLevel::Verbose, "\t\t Precompute edge data on\n") : InnoLogger::Log(LogLevel::Verbose, "\t\t Precompute edge data off\n");
	!skipMeshCleanup ? InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh cleanup on\n") : InnoLogger::Log(LogLevel::Verbose, "\t\t Mesh cleanup off\n");
	InnoLogger::Log(LogLevel::Verbose, "\t\t Num triangles per leaf: %d \n", numTrisPerLeaf);
	InnoLogger::Log(LogLevel::Verbose, "\t Elapsed time in ms: %f \n", double(elapsedTime));
	if (!inserted)
	{
		InnoLogger::Log(LogLevel::Verbose, "\t Mesh size: %d \n", meshSize);
	}

	PhysXTriangleMeshes.emplace(rhs->m_MeshMaterialPair->mesh, triMesh);

	return triMesh;
}

bool PhysXWrapperNS::createPxMesh(PhysicsComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic, bool isConvex)
{
	std::lock_guard<std::mutex> lock{ PhysXWrapperNS::m_mutex };

	if (size.x > 0 && size.y > 0 && size.z > 0)
	{
		PxRigidActor* l_actor;
		PxShape* l_shape;
		PxTriangleMeshGeometry triGeom;
		PxConvexMeshGeometry convexGeom;

		if (isConvex)
		{
			auto convex = createPxConvexMesh(rhs, PxConvexMeshCookingType::eQUICKHULL, true, 256);
			convexGeom.convexMesh = convex;
			convexGeom.scale = PxMeshScale(PxVec3(size.x, size.y, size.z));
			l_shape = gPhysics->createShape(convexGeom, *gMaterial);
		}
		else
		{
			auto triMesh = createBV34TriangleMesh(rhs, false, false, true, 15);
			triGeom.triangleMesh = triMesh;
			triGeom.scale = PxMeshScale(PxVec3(size.x, size.y, size.z));
		}

		PxTransform globalTm(PxVec3(globalPos.x, globalPos.y, globalPos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));

		if (isDynamic)
		{
			auto body = gPhysics->createRigidDynamic(globalTm);
			if (isConvex)
			{
				body->attachShape(*l_shape);
			}
			else
			{
				body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
				l_shape = PxRigidActorExt::createExclusiveShape(*body, triGeom, *gMaterial);
			}
			body->userData = rhs;
			l_actor = body;
		}
		else
		{
			auto body = gPhysics->createRigidStatic(globalTm);
			if (isConvex)
			{
				body->attachShape(*l_shape);
			}
			else
			{
				l_shape = PxRigidActorExt::createExclusiveShape(*body, triGeom, *gMaterial);
			}
			body->userData = rhs;
			l_actor = body;
		}

		rhs->m_Proxy = l_actor;
		gScene->addActor(*l_actor);
		PhysXActors.emplace_back(PhysXActor{ isDynamic, l_actor });
		InnoLogger::Log(LogLevel::Verbose, "PhysXWrapper: PxRigidActor has been created for ", rhs, ".");
	}

	return true;
}

bool PhysXWrapper::Setup()
{
	return PhysXWrapperNS::Setup();
}

bool PhysXWrapper::Initialize()
{
	return PhysXWrapperNS::Initialize();
}

bool PhysXWrapper::Update()
{
	return PhysXWrapperNS::Update();
}

bool PhysXWrapper::Terminate()
{
	return PhysXWrapperNS::Terminate();
}

bool PhysXWrapper::createPxSphere(PhysicsComponent* rhs, float radius, bool isDynamic)
{
	return PhysXWrapperNS::createPxSphere(rhs, rhs->m_TransformComponent->m_localTransformVector_target.m_pos, radius, isDynamic);
}

bool PhysXWrapper::createPxBox(PhysicsComponent* rhs, bool isDynamic)
{
	return PhysXWrapperNS::createPxBox(rhs, rhs->m_TransformComponent->m_localTransformVector_target.m_pos, rhs->m_TransformComponent->m_localTransformVector_target.m_rot, rhs->m_TransformComponent->m_localTransformVector_target.m_scale, isDynamic);
}

bool PhysXWrapper::createPxMesh(PhysicsComponent* rhs, bool isDynamic, bool isConvex)
{
	return PhysXWrapperNS::createPxMesh(rhs, rhs->m_TransformComponent->m_localTransformVector_target.m_pos, rhs->m_TransformComponent->m_localTransformVector_target.m_rot, rhs->m_TransformComponent->m_localTransformVector_target.m_scale, isDynamic, isConvex);
}

bool PhysXWrapper::addForce(PhysicsComponent* rhs, Vec4 force)
{
	auto l_rigidBody = reinterpret_cast<PxRigidDynamic*>(rhs->m_Proxy);
	l_rigidBody->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE);
	return true;
}