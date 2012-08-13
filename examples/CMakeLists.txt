include_directories(${PROJECT_SOURCE_DIR}/include) 

add_definitions(-DLWSYNC_HAS_CPP11)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS -std=c++0x)
	add_definitions(-D_GLIBCXX_USE_NANOSLEEP) # Required to enable this_thread:: sleep_for and ::sleep_until on certain distros.
endif()

add_executable(hello_world hello_world.cpp)
add_executable(multiplier multiplier.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(hello_world pthread)
	target_link_libraries(multiplier pthread)
endif()

set_property(TARGET hello_world PROPERTY FOLDER "examples")
set_property(TARGET multiplier PROPERTY FOLDER "examples")