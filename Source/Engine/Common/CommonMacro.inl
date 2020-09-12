#define GetComponentManager( className ) \
reinterpret_cast<I##className##Manager*>(g_pModuleManager->getComponentManager(className::GetTypeID()))

#define SpawnComponent( className, parentEntity, objectSource, objectUsage ) \
reinterpret_cast<className*>(GetComponentManager(className)->Spawn(parentEntity, objectSource, objectUsage))

#define GetComponent( className, parentEntity ) \
reinterpret_cast<className*>(GetComponentManager(className)->Find(parentEntity))

#define DestroyComponent( className, component ) \
GetComponentManager(className)->Destroy(component)