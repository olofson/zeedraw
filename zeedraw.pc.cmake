#
# pkgconfig for ZeeDraw
#

prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}
includedir=${prefix}/include/ZeeDraw

Name: ZeeDraw
Description: ZeeDraw - a scene graph rendering engine.
Version: @VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_PATCH@
Requires: sdl >= 1.2.10
Libs: -L${libdir} -lzeedraw
Libs.private: -L${libdir} -lSDL
Cflags: -I${includedir}
