# For building against an installed library:
#find_package(ZeeDraw REQUIRED)
#include_directories(${ZEEDRAW_INCLUDE_DIR})

# For building against the locally built library:
include_directories(${ZEEDRAW_SOURCE_DIR}/include)
link_directories(${ZEEDRAW_BINARY_DIR})
set(ZEEDRAW_LIBRARY zeedraw)

find_package(SDL)
if(SDL_FOUND)
	include_directories(${SDL_INCLUDE_DIR})
	link_libraries(${SDL_LIBRARY} SDLmain)
	add_executable(zdtest WIN32 MACOSX_BUNDLE zdtest.c zdtutils.c)
	target_link_libraries(zdtest ${ZEEDRAW_LIBRARY})

	add_executable(balls WIN32 MACOSX_BUNDLE balls.c zdtutils.c)
	target_link_libraries(balls ${ZEEDRAW_LIBRARY})
endif(SDL_FOUND)

# Release build: full optimization, no debug features, no debug info
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Maintainer build: No optimizations, lots of warnings, fail on warnings
set(f "-O1 -g -DDEBUG -Wall -Werror -Wwrite-strings -Wcast-align")
set(f "${f} -Wbad-function-cast -Waggregate-return")
set(f "${f} -Wstrict-prototypes -fno-builtin -Wshadow")
set(f "${f} -Wdeclaration-after-statement -Wmissing-prototypes")
set(f "${f} -Wmissing-declarations -Wdisabled-optimization")
set(CMAKE_C_FLAGS_MAINTAINER ${f})

# Debug build: Maintainer + extra debug features, don't fail on warnings
set(f "${f} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG ${f})
