add_library(ael_openssl ssl_stream_buffer_filter.cc)

target_include_directories(ael_openssl
	PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
	PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include> $<INSTALL_INTERFACE:include>)

install(TARGETS ael_openssl
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	EXPORT libael_openssl_targets)

install(FILES 
	${PROJECT_SOURCE_DIR}/include/ssl_stream_buffer_filter.h 	
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ael/openssl)