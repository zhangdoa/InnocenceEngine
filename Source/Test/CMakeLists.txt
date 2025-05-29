set(TEST_SOURCES
    Test.cpp
    # Common test infrastructure
    Common/TestRunner.cpp
    Common/TestTimer.cpp
    Common/TestData.cpp
    # Unit tests
    UnitTests/ObjectPoolTests.cpp
    UnitTests/ArrayTests.cpp
    UnitTests/AtomicTests.cpp
    UnitTests/RingBufferTests.cpp
    # Performance tests
    PerformanceTests/StringConversionPerf.cpp
    PerformanceTests/ContainerPerf.cpp
    PerformanceTests/MemoryPerf.cpp
    # Stress tests
    StressTests/ConcurrencyStress.cpp
    StressTests/MemoryStress.cpp
)

add_executable(Test ${TEST_SOURCES})
target_link_libraries(Test Engine)
set_target_properties(Test PROPERTIES FOLDER Test)

if (INNO_PLATFORM_LINUX)
target_link_libraries(Test -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -lm -lpthread -lstdc++fs)
endif (INNO_PLATFORM_LINUX)
