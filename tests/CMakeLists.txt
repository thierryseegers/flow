include_directories(${PROJECT_SOURCE_DIR}/include) 

file(GLOB flow_headers ${PROJECT_SOURCE_DIR}/include/*.h)
file(GLOB flow_samples ${PROJECT_SOURCE_DIR}/include/samples/*.h)

source_group(flow FILES ${flow_headers})
source_group(flow\\samples FILES ${flow_samples})

file(GLOB functional_headers *.h)
file(GLOB functional_soruces *.cpp)

add_definitions(-DLWSYNC_HAS_CPP11)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -std=c++0x)
	add_definitions(-D_GLIBCXX_USE_NANOSLEEP) # Required to enable this_thread:: sleep_for and ::sleep_until on certain distros with gcc 4.6.
endif()

add_executable(functional ${flow_headers} ${flow_samples} ${functional_headers} ${functional_soruces})

if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(functional pthread)
endif()

set_property(TARGET functional PROPERTY FOLDER "tests")

add_test(instantiate_empty_graph functional instantiate_empty_graph)
add_test(graph_dummy_nodes_unconnected functional graph_dummy_nodes_unconnected)
add_test(graph_dummy_nodes_connected functional graph_dummy_nodes_connected)
add_test(just_one functional just_one)