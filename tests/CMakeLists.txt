include_directories(${PROJECT_SOURCE_DIR}/include) 

file(GLOB flow_headers ${PROJECT_SOURCE_DIR}/include/*.h)
file(GLOB flow_samples ${PROJECT_SOURCE_DIR}/include/samples/*.h)

source_group(flow FILES ${flow_headers})
source_group(flow\\samples FILES ${flow_samples})

file(GLOB functional_headers *.h)
file(GLOB functional_soruces *.cpp)

add_definitions(-DLWSYNC_HAS_CPP11)

add_executable(functional ${flow_headers} ${flow_samples} ${functional_headers} ${functional_soruces})

set_property(TARGET functional PROPERTY FOLDER "tests")

add_test(instantiate_empty_graph functional instantiate_empty_graph)
add_test(graph_dummy_nodes functional graph_dummy_nodes)