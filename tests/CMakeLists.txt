
configure_file(CMakeLists.txt.googletest.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	RESULT_VARIABLE result
 	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
 	message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )

if(result)
	message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

file(COPY fake1.crt fake1.key DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tests_data)
add_executable(ssl ssl_test.cc helpers.cc)
target_link_libraries(ssl ael ael_openssl gtest_main ${OPENSSL_SSL_LIBRARY})
add_test(NAME ssl_test COMMAND ssl WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests_data)