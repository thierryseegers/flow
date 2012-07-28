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

add_test(empty_not_started functional empty nostart)
add_test(empty_started functional empty start)
add_test(dummies_unconnected_not_started functional unconnected nostart)
add_test(dummies_unconnected_started functional unconnected start)
add_test(dummies_connected_notstarted functional connected nostart)
add_test(dummies_connected_started functional connected start)
add_test(count_1 functional count 1)
add_test(count_2 functional count 2)
add_test(count_3 functional count 3)
add_test(count_10 functional count 10)
add_test(tee_1 functional tee 1)
add_test(tee_2 functional tee 2)
add_test(tee_3 functional tee 3)
add_test(tee_10 functional tee 10)
add_test(add_delay functional add_delay)