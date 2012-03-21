find_package(Boost COMPONENTS date_time thread)

include_directories(${Boost_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/include) 

link_directories(${Boost_LIBRARY_DIRS})

file(GLOB flow_headers ${PROJECT_SOURCE_DIR}/include/*.h)
file(GLOB flow_samples ${PROJECT_SOURCE_DIR}/include/samples/*.h)

source_group(flow FILES ${flow_headers})
source_group(flow\\samples FILES ${flow_samples})

add_executable(hello_world hello_world.cpp ${flow_headers} ${flow_samples})
add_executable(multiplier multiplier.cpp ${flow_headers} ${flow_samples})

set_property(TARGET hello_world PROPERTY FOLDER "examples")
set_property(TARGET multiplier PROPERTY FOLDER "examples")