find_package(Doxygen REQUIRED)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
add_custom_target(documentation ALL 
				  COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
				  COMMENT "Generating API documentation with Doxygen" VERBATIM
				  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
endif()