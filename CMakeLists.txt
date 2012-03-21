cmake_minimum_required(VERSION 2.6)

project(flow)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(examples)
add_subdirectory(documentation)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION .)
install(DIRECTORY ${PROJECT_BINARY_DIR}/documentation/htdocs DESTINATION documentation)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME flow) 
include(CPack)