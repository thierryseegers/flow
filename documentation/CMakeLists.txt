find_package(Doxygen REQUIRED)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
add_custom_target(documentation ALL ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
				  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
				  COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()