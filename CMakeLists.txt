cmake_minimum_required(VERSION 2.8)
project(ZEEDRAW)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(PACKAGE "zeedraw-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(VERSION_STRING "${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_PATCH}")

add_subdirectory(src)
add_subdirectory(test)

install(DIRECTORY include/ DESTINATION include/ZeeDraw
	FILES_MATCHING PATTERN "*.h")

configure_file(zeedraw.pc.cmake ${CMAKE_SOURCE_DIR}/zeedraw.pc @ONLY)
install(FILES ${CMAKE_SOURCE_DIR}/zeedraw.pc
	DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/")
