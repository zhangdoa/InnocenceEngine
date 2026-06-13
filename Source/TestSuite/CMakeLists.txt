set(TEST_SOURCES
    TestSuite.cpp
    # Common test infrastructure
    Common/TestRunner.cpp
    Common/TestTimer.cpp
    Common/TestData.cpp
    # Unit tests
    UnitTests/ObjectPoolTests.cpp
    UnitTests/ArrayTests.cpp
    UnitTests/RingBufferTests.cpp
    UnitTests/EntityRegistryTests.cpp
    UnitTests/FixedSizeStringTests.cpp
    UnitTests/MemoryTests.cpp
    UnitTests/DoubleBufferTests.cpp
    UnitTests/AllocatorTests.cpp
    UnitTests/QueueTests.cpp
    UnitTests/HashMapTests.cpp
    UnitTests/UnorderedSetTests.cpp
    UnitTests/DequeTests.cpp
    UnitTests/RenderGraphSerializerTests.cpp
    UnitTests/RenderGraphSerializerTests_ScreenSized.cpp
    UnitTests/RenderGraphTransitionTests.cpp
    UnitTests/RenderGraphSerializerTests_SSAO.cpp
    UnitTests/RenderGraphSerializerTests_TiledFrustum.cpp
    UnitTests/RenderGraphSerializerTests_PingPong.cpp
    UnitTests/RenderGraphSerializerTests_Raster.cpp
    UnitTests/RenderGraphSerializerTests_Raytracing.cpp
    # Performance tests
    PerformanceTests/StringConversionPerf.cpp
    PerformanceTests/ContainerPerf.cpp
    PerformanceTests/MemoryPerf.cpp
    # Stress tests
    StressTests/ConcurrencyStress.cpp
    StressTests/MemoryStress.cpp
    StressTests/TaskSystemStressTests.cpp
    StressTests/ContainerStress.cpp
    # Concurrency tests
    ConcurrencyTests/TaskSystemTests.cpp
    # Integration tests
    IntegrationTests/AssetConversionTests.cpp
)

add_executable(TestSuite ${TEST_SOURCES})
target_link_libraries(TestSuite Engine)
set_target_properties(TestSuite PROPERTIES FOLDER TestSuite)
target_include_directories(TestSuite PRIVATE "${INNO_GITSUBMODULE_DIRECTORIES}/json/include")

if (INNO_PLATFORM_LINUX)
target_link_libraries(TestSuite -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)
