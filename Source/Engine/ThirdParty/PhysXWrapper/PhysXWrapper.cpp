#include "PhysXWrapper.h"

#if defined INNO_PLATFORM_WIN
#include "PxPhysicsAPI.h"
#include "cooking/PxCooking.h"
#endif

#include "../../Common/Timer.h"
#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/TaskScheduler.h"
#include "../../Component/MeshComponent.h"
#include "../../Component/ModelComponent.h"
#include "../../Services/SceneService.h"
#include "../../Services/HIDService.h"

#include "../../Engine.h"
using namespace Inno;

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

	bool createPxSphere(uint64_t index, Vec4 globalPos, float radius, bool isDynamic);
	bool createPxBox(uint64_t index, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic);
	PxConvexMesh* createPxConvexMesh(uint64_t index, PxConvexMeshCookingType::Enum convexMeshCookingType, bool directInsertion, PxU32 gaussMapLimit, std::vector<Vertex>& vertices, std::vector<Index>& indices);
	PxTriangleMesh* createBV33TriangleMesh(uint64_t index, bool skipMeshCleanup, bool skipEdgeData, bool inserted, bool cookingPerformance, bool meshSizePerfTradeoff, std::vector<Vertex>& vertices, std::vector<Index>& indices);
	PxTriangleMesh* createBV34TriangleMesh(uint64_t index, bool skipMeshCleanup, bool skipEdgeData, bool inserted, const PxU32 numTrisPerLeaf, std::vector<Vertex>& vertices, std::vector<Index>& indices);

	bool createPxMesh(uint64_t index, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic, bool isConvex, std::vector<Vertex>& vertices, std::vector<Index>& indices);

	std::unordered_map<uint64_t, PxConvexMesh*> PhysXConvexMeshes;
	std::unordered_map<uint64_t, PxTriangleMesh*> PhysXTriangleMeshes;

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
	std::function<void()> f_sceneLoadingStartedCallback;
	std::function<void()> f_TogglePhysXUpdateTask;
	Handle<ITask> m_PhysXUpdateTask;

	std::mutex m_mutex;
}

bool PhysXWrapperNS::Setup()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback,
		gDefaultErrorCallback);
	if (!gFoundation)
	{
		Log(Error, "PxCreateFoundation failed!");
		return false;
	}
	bool recordMemoryAllocations = true;

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), recordMemoryAllocations, gPvd);

	if (!gPhysics)
	{
		Log(Error, "PxCreatePhysics failed!");
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

	f_sceneLoadingStartedCallback = [&]() 
	{
		m_needSimulate = false;

		Log(Verbose, "Removing all PhysX Actors...");

		for (auto i : PhysXActors)
		{
			gScene->removeActor(*i.m_PxRigidActor);
		}

		PhysXActors.clear();

		Log(Success, "All PhysX Actors have been removed.");
	};

	g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&f_sceneLoadingStartedCallback, 1);

	m_PhysXUpdateTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("PhysXUpdateTask", ITask::Type::Recurrent, 3), [&]()
		{
			if (g_Engine->Get<SceneService>()->IsLoading())
				return;

			gScene->simulate(g_Engine->getTickTime() / 1000.0f);
			gScene->fetchResults(true);

			for (auto i : PhysXActors)
			{
				if (!i.isDynamic)
					continue;
				auto l_rigidBody = reinterpret_cast<PxRigidDynamic*>(i.m_PxRigidActor);
				if (!l_rigidBody->userData)
					continue;

				PxTransform t = i.m_PxRigidActor->getGlobalPose();
				PxVec3 p = t.p;
				PxQuat q = t.q;

				// @TODO: Add back the transform component update logic
				// auto l_collisionComponent = reinterpret_cast<CollisionComponent*>(l_rigidBody->userData);
				// auto l_transform = &l_collisionComponent->m_Transform;
				// l_transform->m_pos = Vec4(p.x, p.y, p.z, 1.0f);
				// l_transform->m_rot = Vec4(q.x, q.y, q.z, q.w);
			}
		});

	f_TogglePhysXUpdateTask = [&]() 
	{ 
		m_needSimulate = !m_needSimulate;
		if (m_needSimulate)
		{
			Log(Verbose, "PhysX Update Task has been activated.");
			m_PhysXUpdateTask->Activate();
		}
		else
		{
			Log(Verbose, "PhysX Update Task has been deactivated.");
			m_PhysXUpdateTask->Deactivate();
		}
	};

	g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_P, true }, ButtonEvent{ EventLifeTime::OneShot, &f_TogglePhysXUpdateTask });

	return true;
}

bool PhysXWrapperNS::Initialize()
{
	return true;
}

bool PhysXWrapperNS::Update()
{
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

	Log(Success, "PhysX has been terminated.");

	return true;
}

bool PhysXWrapperNS::createPxSphere(uint64_t index, Vec4 globalPos, float radius, bool isDynamic)
{
	std::lock_guard<std::mutex> lock{ PhysXWrapperNS::m_mutex };
	PxRigidActor* l_actor;

	auto shape = gPhysics->createShape(PxSphereGeometry(radius), *gMaterial);
	PxTransform globalTm(PxVec3(globalPos.x, globalPos.y, globalPos.z));

	if (isDynamic)
	{
		auto body = gPhysics->createRigidDynamic(globalTm);
		body->userData = reinterpret_cast<void*>(index);
		body->attachShape(*shape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		l_actor = body;
	}
	else
	{
		auto body = gPhysics->createRigidStatic(globalTm);
		body->userData = reinterpret_cast<void*>(index);
		body->attachShape(*shape);
		l_actor = body;
	}

	gScene->addActor(*l_actor);
	shape->release();
	PhysXActors.emplace_back(PhysXActor{ isDynamic, l_actor });
	Log(Verbose, "PxRigidActor has been created for ", index, ".");

	return true;
}

bool PhysXWrapperNS::createPxBox(uint64_t index, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic)
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
			body->userData = reinterpret_cast<void*>(index);
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			l_actor = body;
		}
		else
		{
			auto body = gPhysics->createRigidStatic(globalTm);
			body->userData = reinterpret_cast<void*>(index);
			body->attachShape(*shape);

			l_actor = body;
		}

		gScene->addActor(*l_actor);
		shape->release();
		PhysXActors.emplace_back(PhysXActor{ isDynamic, l_actor });
		Log(Verbose, "PxRigidActor has been created for ", index, ".");
	}

	return true;
}

PxConvexMesh* PhysXWrapperNS::createPxConvexMesh(uint64_t index, PxConvexMeshCookingType::Enum convexMeshCookingType, bool directInsertion, PxU32 gaussMapLimit, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto l_result = PhysXConvexMeshes.find(index);
	if (l_result != PhysXConvexMeshes.end())
	{
		return l_result->second;
	}

	PxCookingParams params(gPhysics->getTolerancesScale());

	// Use the new (default) PxConvexMeshCookingType::eQUICKHULL
	params.convexMeshCookingType = convexMeshCookingType;

	// If the gaussMapLimit is chosen higher than the number of output vertices, no gauss map is added to the convex mesh data.
	// If the gaussMapLimit is chosen lower than the number of output vertices, a gauss map is added to the convex mesh data.
	params.gaussMapLimit = gaussMapLimit;

	// Setup the convex mesh descriptor
	PxConvexMeshDesc desc;

	// We provide points only, therefore the PxConvexFlag::eCOMPUTE_CONVEX flag must be specified
	desc.points.data = &vertices[0];
	desc.points.count = (PxU32)vertices.size();
	desc.points.stride = sizeof(Vertex);
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxU32 meshSize = 0;
	PxConvexMesh* convex = nullptr;

	auto startTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	if (directInsertion)
	{
		// Directly insert mesh into PhysX
		convex = PxCreateConvexMesh(params, desc);
		PX_ASSERT(convex);
	}
	else
	{
		// Serialize the cooked mesh into a stream.
		PxDefaultMemoryOutputStream outStream;
		bool res = PxCookConvexMesh(params, desc, outStream);
		PX_UNUSED(res);
		PX_ASSERT(res);
		meshSize = outStream.getSize();

		// Create the mesh from a stream.
		PxDefaultMemoryInputData inStream(outStream.getData(), outStream.getSize());
		convex = gPhysics->createConvexMesh(inStream);
		PX_ASSERT(convex);
	}

	// Print the elapsed time for comparison
	auto stopTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	auto elapsedTime = stopTime - startTime;
	Log(Verbose, "Create convex mesh with ", desc.points.count, " triangles: ");
	directInsertion ? Log(Verbose, "Direct mesh insertion enabled.") : Log(Verbose, "Direct mesh insertion disabled.");
	Log(Verbose, "Gauss map limit: %d \n", gaussMapLimit);
	Log(Verbose, "Created hull number of vertices: ", convex->getNbVertices());
	Log(Verbose, "Created hull number of polygons: ", convex->getNbPolygons());
	Log(Verbose, "Elapsed time in ms: ", double(elapsedTime));
	if (!directInsertion)
	{
		Log(Verbose, "Mesh size: ", meshSize);
	}

	PhysXConvexMeshes.emplace(index, convex);

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
PxTriangleMesh* PhysXWrapperNS::createBV33TriangleMesh(uint64_t index, bool skipMeshCleanup, bool skipEdgeData, bool inserted, bool cookingPerformance, bool meshSizePerfTradeoff, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto l_result = PhysXTriangleMeshes.find(index);
	if (l_result != PhysXTriangleMeshes.end())
	{
		return l_result->second;
	}

	auto startTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = (PxU32)vertices.size();
	meshDesc.points.data = &vertices[0];
	meshDesc.points.stride = sizeof(PxVec4);
	meshDesc.triangles.count = (PxU32)indices.size() / 3;
	meshDesc.triangles.data = &indices[0];
	meshDesc.triangles.stride = 3 * sizeof(PxU32);

	PxCookingParams params(gPhysics->getTolerancesScale());

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

#if defined(PX_CHECKED) || defined(PX_DEBUG)
	// If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking.
	// We should check the validity of provided triangles in debug/checked builds though.
	if (skipMeshCleanup)
	{
		PX_ASSERT(PxValidateTriangleMesh(meshDesc));
	}
#endif // DEBUG

	PxTriangleMesh* triMesh = 0;
	PxU32 meshSize = 0;

	// The cooked mesh may either be saved to a stream for later loading, or inserted directly into PxPhysics.
	if (inserted)
	{
		triMesh = PxCreateTriangleMesh(params, meshDesc);
	}
	else
	{
		PxDefaultMemoryOutputStream outBuffer;
		PxCookTriangleMesh(params, meshDesc, outBuffer);

		PxDefaultMemoryInputData stream(outBuffer.getData(), outBuffer.getSize());
		triMesh = gPhysics->createTriangleMesh(stream);

		meshSize = outBuffer.getSize();
	}

	// Print the elapsed time for comparison
	auto stopTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	auto elapsedTime = stopTime - startTime;
	Log(Verbose, "\t -----------------------------------------------\n");
	Log(Verbose, "\t Create triangle mesh with %d triangles: \n", meshDesc.triangles.count);
	cookingPerformance ? Log(Verbose, "\t\t Cooking performance on\n") : Log(Verbose, "\t\t Cooking performance off\n");
	inserted ? Log(Verbose, "\t\t Mesh inserted on\n") : Log(Verbose, "\t\t Mesh inserted off\n");
	!skipEdgeData ? Log(Verbose, "\t\t Precompute edge data on\n") : Log(Verbose, "\t\t Precompute edge data off\n");
	!skipMeshCleanup ? Log(Verbose, "\t\t Mesh cleanup on\n") : Log(Verbose, "\t\t Mesh cleanup off\n");
	Log(Verbose, "\t\t Mesh size/performance trade-off: %f \n", double(params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff));
	Log(Verbose, "\t Elapsed time in ms: %f \n", double(elapsedTime));
	if (!inserted)
	{
		Log(Verbose, "\t Mesh size: %d \n", meshSize);
	}

	PhysXTriangleMeshes.emplace(index, triMesh);

	return triMesh;
}

// Creates a triangle mesh using BVH34 midphase with different settings.
PxTriangleMesh* PhysXWrapperNS::createBV34TriangleMesh(uint64_t index, bool skipMeshCleanup, bool skipEdgeData, bool inserted, const PxU32 numTrisPerLeaf, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto l_result = PhysXTriangleMeshes.find(index);
	if (l_result != PhysXTriangleMeshes.end())
	{
		return l_result->second;
	}

	auto startTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = (PxU32)vertices.size();
	meshDesc.points.data = &vertices[0];
	meshDesc.points.stride = sizeof(Vertex);
	meshDesc.triangles.count = (PxU32)indices.size() / 3;
	meshDesc.triangles.data = &indices[0];
	meshDesc.triangles.stride = 3 * sizeof(Index);

	PxCookingParams params(gPhysics->getTolerancesScale());

	// Create BVH34 midphase
	params.midphaseDesc = PxMeshMidPhase::eBVH34;

	// Setup common cooking params
	setupCommonCookingParams(params, skipMeshCleanup, skipEdgeData);

	// Cooking mesh with less triangles per leaf produces larger meshes with better runtime performance
	// and worse cooking performance. Cooking time is better when more triangles per leaf are used.
	params.midphaseDesc.mBVH34Desc.numPrimsPerLeaf = numTrisPerLeaf;

#if defined(PX_CHECKED) || defined(PX_DEBUG)
	// If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking.
	// We should check the validity of provided triangles in debug/checked builds though.
	if (skipMeshCleanup)
	{
		PX_ASSERT(PxValidateTriangleMesh(meshDesc));
	}
#endif // DEBUG

	PxTriangleMesh* triMesh = 0;
	PxU32 meshSize = 0;

	// The cooked mesh may either be saved to a stream for later loading, or inserted directly into PxPhysics.
	if (inserted)
	{
		triMesh = PxCreateTriangleMesh(params, meshDesc);
	}
	else
	{
		PxDefaultMemoryOutputStream outBuffer;
		PxCookTriangleMesh(params, meshDesc, outBuffer);

		PxDefaultMemoryInputData stream(outBuffer.getData(), outBuffer.getSize());
		triMesh = gPhysics->createTriangleMesh(stream);

		meshSize = outBuffer.getSize();
	}

	// Print the elapsed time for comparison
	auto stopTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	auto elapsedTime = stopTime - startTime;
	Log(Verbose, "\t -----------------------------------------------\n");
	Log(Verbose, "\t Create triangle mesh with %d triangles: \n", meshDesc.triangles.count);
	inserted ? Log(Verbose, "\t\t Mesh inserted on\n") : Log(Verbose, "\t\t Mesh inserted off\n");
	!skipEdgeData ? Log(Verbose, "\t\t Precompute edge data on\n") : Log(Verbose, "\t\t Precompute edge data off\n");
	!skipMeshCleanup ? Log(Verbose, "\t\t Mesh cleanup on\n") : Log(Verbose, "\t\t Mesh cleanup off\n");
	Log(Verbose, "\t\t Num triangles per leaf: %d \n", numTrisPerLeaf);
	Log(Verbose, "\t Elapsed time in ms: %f \n", double(elapsedTime));
	if (!inserted)
	{
		Log(Verbose, "\t Mesh size: %d \n", meshSize);
	}

	PhysXTriangleMeshes.emplace(index, triMesh);

	return triMesh;
}

bool PhysXWrapperNS::createPxMesh(uint64_t index, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic, bool isConvex, std::vector<Vertex>& vertices, std::vector<Index>& indices)
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
			auto convex = createPxConvexMesh(index, PxConvexMeshCookingType::eQUICKHULL, true, 256, vertices, indices);
			convexGeom.convexMesh = convex;
			convexGeom.scale = PxMeshScale(PxVec3(size.x, size.y, size.z));
			l_shape = gPhysics->createShape(convexGeom, *gMaterial);
		}
		else
		{
			auto triMesh = createBV34TriangleMesh(index, false, false, true, 15, vertices, indices);
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
			body->userData = reinterpret_cast<void*>(index);
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
			body->userData = reinterpret_cast<void*>(index);
			l_actor = body;
		}

		gScene->addActor(*l_actor);
		PhysXActors.emplace_back(PhysXActor{ isDynamic, l_actor });
		Log(Verbose, "PxRigidActor has been created for ", index, ".");
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

bool PhysXWrapper::createPxSphere(uint64_t index, Vec4 position, float radius, bool isDynamic)
{
	return PhysXWrapperNS::createPxSphere(index, position, radius, isDynamic);
}

bool PhysXWrapper::createPxBox(uint64_t index, Vec4 position, Vec4 rotation, Vec4 scale, bool isDynamic)
{
	return PhysXWrapperNS::createPxBox(index, position, rotation, scale, isDynamic);
}

bool PhysXWrapper::createPxMesh(uint64_t index, Vec4 position, Vec4 rotation, Vec4 scale, bool isDynamic, bool isConvex, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	return PhysXWrapperNS::createPxMesh(index, position, rotation, scale, isDynamic, isConvex, vertices, indices);
}

bool PhysXWrapper::addForce(ModelComponent* rhs, Vec4 force)
{
	auto l_rigidBody = reinterpret_cast<PxRigidDynamic*>(rhs->m_SimulationProxy);
	l_rigidBody->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE);
	return true;
}