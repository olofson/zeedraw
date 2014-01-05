INCLUDE (CheckIncludeFiles)

include_directories(${ZEEDRAW_SOURCE_DIR}/include)
include_directories(${ZEEDRAW_SOURCE_DIR}/src)


# Core files
set(sources
	zeedraw.c
	zd_opengl.c
	zd_gli.c
	zd_software.c
)


find_package(SDL)
if(SDL_FOUND)
	add_definitions(-DA2_HAVE_SDL)
	include_directories(${SDL_INCLUDE_DIR})
	link_libraries(${SDL_LIBRARY})
endif(SDL_FOUND)

add_library(zeedraw SHARED ${sources})
target_link_libraries(zeedraw m ${SDL_LIBRARY})


# Release build: full optimization, no debug features, no debug info
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Maintainer build: No optimizations, lots of warnings, fail on warnings
set(f "-O0 -g -Wall -Wwrite-strings -Wcast-align")
set(f "${f} -Wbad-function-cast -Waggregate-return")
set(f "${f} -Wstrict-prototypes -fno-builtin -Wshadow")
set(f "${f} -Wdeclaration-after-statement -Wmissing-prototypes")
set(f "${f} -Wmissing-declarations -Wdisabled-optimization")
set(CMAKE_C_FLAGS_MAINTAINER "${f} -Werror")

# Debug build: Maintainer + extra debug features, don't fail on warnings
set(f "${f} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG ${f})

install(TARGETS zeedraw DESTINATION lib)
