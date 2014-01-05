/*
 * ZeeDraw - OpenGL loader
 *
 * Copyright 2010-2013 David Olofson
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "zd_gli.h"
#include "SDL.h"
#include <math.h>
#include <stddef.h>


static struct
{
	const char	*name;
	size_t		fn;
} glfuncs[] = {
	/*
	 * OpenGL 1.1 core functions - if not present, we bail out!
	 */

	/* Miscellaneous */
	{"glGetError", offsetof(ZD_glinterface, GetError) },
	{"glGetDoublev", offsetof(ZD_glinterface, GetDoublev) },
	{"glGetString", offsetof(ZD_glinterface, GetString) },
	{"glDisable", offsetof(ZD_glinterface, Disable) },
	{"glEnable", offsetof(ZD_glinterface, Enable) },
	{"glBlendFunc", offsetof(ZD_glinterface, BlendFunc) },
	{"glClearColor", offsetof(ZD_glinterface, ClearColor) },
	{"glColorMask", offsetof(ZD_glinterface, ColorMask) },
	{"glClear", offsetof(ZD_glinterface, Clear) },
	{"glFlush", offsetof(ZD_glinterface, Flush) },
	{"glHint", offsetof(ZD_glinterface, Hint) },
	{"glDisableClientState", offsetof(ZD_glinterface, DisableClientState) },
	{"glEnableClientState", offsetof(ZD_glinterface, EnableClientState) },

	/* Depth Buffer */
	{"glClearDepth", offsetof(ZD_glinterface, ClearDepth) },
	{"glDepthFunc", offsetof(ZD_glinterface, DepthFunc) },

	/* Stencil Buffer */
	{"glClearStencil", offsetof(ZD_glinterface, ClearStencil) },
	{"glStencilOp", offsetof(ZD_glinterface, StencilOp) },
	{"glStencilFunc", offsetof(ZD_glinterface, StencilFunc) },

	/* Transformation */
	{"glMatrixMode", offsetof(ZD_glinterface, MatrixMode) },
	{"glOrtho", offsetof(ZD_glinterface, Ortho) },
	{"glFrustum", offsetof(ZD_glinterface, Frustum) },
	{"glViewport", offsetof(ZD_glinterface, Viewport) },
	{"glScissor", offsetof(ZD_glinterface, Scissor) },
	{"glPushMatrix", offsetof(ZD_glinterface, PushMatrix) },
	{"glPopMatrix", offsetof(ZD_glinterface, PopMatrix) },
	{"glLoadIdentity", offsetof(ZD_glinterface, LoadIdentity) },
	{"glLoadMatrixd", offsetof(ZD_glinterface, LoadMatrixd) },
	{"glMultMatrixd", offsetof(ZD_glinterface, MultMatrixd) },
	{"glRotated", offsetof(ZD_glinterface, Rotated) },
	{"glScaled", offsetof(ZD_glinterface, Scaled) },
	{"glTranslated", offsetof(ZD_glinterface, Translated) },

	/* Textures */
	{"glIsTexture", offsetof(ZD_glinterface, IsTexture) },
	{"glGenTextures", offsetof(ZD_glinterface, GenTextures) },
	{"glBindTexture", offsetof(ZD_glinterface, BindTexture) },
	{"glDeleteTextures", offsetof(ZD_glinterface, DeleteTextures) },
	{"glTexImage2D", offsetof(ZD_glinterface, TexImage2D) },
	{"glTexParameteri", offsetof(ZD_glinterface, TexParameteri) },
	{"glTexParameterf", offsetof(ZD_glinterface, TexParameterf) },
	{"glTexParameterfv", offsetof(ZD_glinterface, TexParameterfv) },
	{"glTexSubImage2D", offsetof(ZD_glinterface, TexSubImage2D) },

	/* Lighting */
	{"glShadeModel", offsetof(ZD_glinterface, ShadeModel) },
	
	/* Raster */
	{"glPixelStorei", offsetof(ZD_glinterface, PixelStorei) },
	{"glReadPixels", offsetof(ZD_glinterface, ReadPixels) },

	/* Drawing */
	{"glBegin", offsetof(ZD_glinterface, Begin) },
	{"glEnd", offsetof(ZD_glinterface, End) },
	{"glVertex2d", offsetof(ZD_glinterface, Vertex2d) },
	{"glVertex3d", offsetof(ZD_glinterface, Vertex3d) },
	{"glVertex4d", offsetof(ZD_glinterface, Vertex4d) },
	{"glNormal3d", offsetof(ZD_glinterface, Normal3d) },
	{"glColor3d", offsetof(ZD_glinterface, Color3d) },
	{"glColor4d", offsetof(ZD_glinterface, Color4d) },
	{"glColor4f", offsetof(ZD_glinterface, Color4f) },
	{"glTexCoord1d", offsetof(ZD_glinterface, TexCoord1d) },
	{"glTexCoord2d", offsetof(ZD_glinterface, TexCoord2d) },
	{"glTexCoord3d", offsetof(ZD_glinterface, TexCoord3d) },
	{"glTexCoord4d", offsetof(ZD_glinterface, TexCoord4d) },

	/* Arrays */
	{"glVertexPointer", offsetof(ZD_glinterface, VertexPointer) },
	{"glTexCoordPointer", offsetof(ZD_glinterface, TexCoordPointer) },
	{"glDrawArrays", offsetof(ZD_glinterface, DrawArrays) },

	{NULL, 0 },

	/*
	 * Calls below this point may be missing - CHECK FOR NULL!!!
	 */

	/* OpenGL 1.2 core functions */
	{"glBlendEquation", offsetof(ZD_glinterface, _BlendEquation) },

	/* 3.0+ mipmap generation */
	{"glGenerateMipmap", offsetof(ZD_glinterface, _GenerateMipmap) },
	
	{NULL, 0 }
};


static int get_gl_calls(ZD_glinterface *gli)
{
	int i;

	/* Get core functions */
	for(i = 0; glfuncs[i].name; ++i)
	{
		void **fns = (void **)((char *)gli + glfuncs[i].fn);
		if(!(*fns = SDL_GL_GetProcAddress(glfuncs[i].name)))
		{
			printf("ERROR: Required OpenGL call '%s' missing!\n",
					glfuncs[i].name);
			return -1;
		}
	}

	/* Try to get newer functions and extensions */
	for(++i; glfuncs[i].name; ++i)
	{
		void **fns = (void **)((char *)gli + glfuncs[i].fn);
		if(!(*fns = SDL_GL_GetProcAddress(glfuncs[i].name)))
			printf("NOTE: %s not available.\n", glfuncs[i].name);
	}

	return 0;
}


static void get_gl_version(ZD_glinterface *gli)
{
	const char *s = (const char *)gli->GetString(GL_VERSION);
	gli->version = 0;
	if(s)
	{
		int major, minor;
		if(sscanf(s, "%d.%d", &major, &minor) == 2)
			gli->version = major * 10 + minor;
	}
}


static int gl_load(ZD_glinterface *gli, const char *libname)
{
	gli_Reset(gli);
	gli->version = 0;
	if(!SDL_GL_GetProcAddress("glGetError"))
	{
		if(SDL_GL_LoadLibrary(libname) < 0)
			return -1;
	}
	if(get_gl_calls(gli) < 0)
		return -2;
	get_gl_version(gli);
	return 0;
}


ZD_glinterface *gli_Open(char *libname)
{
	ZD_glinterface *gli = (ZD_glinterface *)calloc(1, sizeof(ZD_glinterface));
	if(gl_load(gli, libname))
	{
		free(gli);
		return NULL;
	}
	return gli;
}


void gli_Close(ZD_glinterface *gli)
{
	free(gli);
}
