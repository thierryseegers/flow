# This is somewhat of a "dummy" target to put all the headers of the flow library in one place rather than have all projects hold references to them.

file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB samples ${CMAKE_CURRENT_SOURCE_DIR}/samples/*.h)

source_group(headers FILES ${headers})
source_group(samples FILES ${samples})

add_custom_target(flow
				  COMMAND cmake -E echo ""
				  SOURCES ${headers} ${samples})
