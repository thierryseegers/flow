include_directories(${PROJECT_SOURCE_DIR}/include) 

file(GLOB flow_headers ${PROJECT_SOURCE_DIR}/include/*.h)
file(GLOB flow_samples ${PROJECT_SOURCE_DIR}/include/samples/*.h)

source_group(flow FILES ${flow_headers})
source_group(flow\\samples FILES ${flow_samples})

add_definitions(-DLWSYNC_HAS_CPP11)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -std=c++0x)
	add_definitions(-D_GLIBCXX_USE_NANOSLEEP) # Required to enable this_thread:: sleep_for and ::sleep_until on certain distros with gcc 4.6.
endif()

add_executable(hello_world hello_world.cpp ${flow_headers} ${flow_samples})
add_executable(multiplier multiplier.cpp ${flow_headers} ${flow_samples})

if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(hello_world pthread)
	target_link_libraries(multiplier pthread)
endif()

set_property(TARGET hello_world PROPERTY FOLDER "examples")
set_property(TARGET multiplier PROPERTY FOLDER "examples")