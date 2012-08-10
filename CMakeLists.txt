cmake_minimum_required(VERSION 2.6)

project(flow)

set(FLOW_VERSION_MAJOR 2)
set(FLOW_VERSION_MINOR 0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

add_subdirectory(documentation)
add_subdirectory(examples)
add_subdirectory(include)
add_subdirectory(tests)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION .)
install(DIRECTORY ${PROJECT_BINARY_DIR}/documentation/htdocs DESTINATION documentation)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME flow_${FLOW_VERSION_MAJOR}_${FLOW_VERSION_MINOR}) 
include(CPack)