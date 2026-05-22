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
    # Performance tests
    PerformanceTests/StringConversionPerf.cpp
    PerformanceTests/ContainerPerf.cpp
    PerformanceTests/MemoryPerf.cpp
    # Stress tests
    StressTests/ConcurrencyStress.cpp
    StressTests/MemoryStress.cpp
    StressTests/TaskSystemStressTests.cpp
    # Concurrency tests
    ConcurrencyTests/TaskSystemTests.cpp
    # Integration tests
    IntegrationTests/AssetConversionTests.cpp
)

add_executable(TestSuite ${TEST_SOURCES})
target_link_libraries(TestSuite Engine)
set_target_properties(TestSuite PROPERTIES FOLDER TestSuite)

if (INNO_PLATFORM_LINUX)
target_link_libraries(TestSuite -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)
