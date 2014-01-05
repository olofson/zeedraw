/*
 * ZeeDraw - OpenGL interface
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

#ifndef ZD_GLI_H
#define ZD_GLI_H

/* Use vertex arrays where appropriate */
#undef ZD_USE_ARRAYS

#ifdef HAS_SDL_OPENGL_H
#	include "SDL_opengl.h"
#else
#	ifdef WIN32
#		include <windows.h>
#	endif
#	if defined(__APPLE__) && defined(__MACH__)
#		include <OpenGL/gl.h>
#		include <OpenGL/glu.h>
#	else
#		include <GL/gl.h>
#		include <GL/glu.h>
#	endif
#endif

#ifndef GL_CLAMP_TO_EDGE
#	define	GL_CLAMP_TO_EDGE	0x812F
#endif
#if 0
#ifndef GL_BLEND_EQUATION
#	define GL_BLEND_EQUATION                       0x8009
#endif
#ifndef GL_MIN
#	define GL_MIN                                  0x8007
#endif
#ifndef GL_MAX
#	define GL_MAX                                  0x8008
#endif
#ifndef GL_FUNC_ADD
#	define GL_FUNC_ADD                             0x8006
#endif
#ifndef GL_FUNC_SUBTRACT
#	define GL_FUNC_SUBTRACT                        0x800A
#endif
#ifndef GL_FUNC_REVERSE_SUBTRACT
#	define GL_FUNC_REVERSE_SUBTRACT                0x800B
#endif
#ifndef GL_BLEND_COLOR
#	define GL_BLEND_COLOR                          0x8005
#endif
#endif

#if defined(_WIN32) && !defined(APIENTRY) && \
		!defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#	include <windows.h>
#endif
#ifndef APIENTRY
#	define APIENTRY
#endif

#ifndef	GL_GENERATE_MIPMAP
#	define	GL_GENERATE_MIPMAP	0x8191
#endif
#ifndef	GL_GENERATE_MIPMAP_HINT
#	define	GL_GENERATE_MIPMAP_HINT	0x8192
#endif


typedef struct ZD_glinterface
{
	/*
	 * OpenGL 1.1 core functions
	 */
	/* Miscellaneous */
	GLenum	(APIENTRY *GetError)(void);
	void	(APIENTRY *GetDoublev)(GLenum pname, GLdouble *params);
	const GLubyte* (APIENTRY *GetString)(GLenum name);
	void	(APIENTRY *Disable)(GLenum cap);
	void	(APIENTRY *Enable)(GLenum cap);
	void	(APIENTRY *BlendFunc)(GLenum, GLenum);
	void	(APIENTRY *ClearColor)(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
	void	(APIENTRY *ColorMask)(GLboolean red, GLboolean green,
			GLboolean blue, GLboolean alpha);
	void	(APIENTRY *Clear)(GLbitfield mask);
	void	(APIENTRY *Flush)(void);
	void	(APIENTRY *Hint)(GLenum target, GLenum mode);
	void	(APIENTRY *DisableClientState)(GLenum cap);
	void	(APIENTRY *EnableClientState)(GLenum cap);

	/* Depth Buffer*/
	void	(APIENTRY *ClearDepth)(GLclampd depth);
	void	(APIENTRY *DepthFunc)(GLenum func);

	/* Stencil Buffer */
	void	(APIENTRY *ClearStencil)(GLint s);
	void	(APIENTRY *StencilOp)(GLenum sfail, GLenum dpfail, GLenum dppass);
	void	(APIENTRY *StencilFunc)(GLenum func, GLint ref, GLuint mask);

	/* Transformation */
	void	(APIENTRY *MatrixMode)(GLenum mode);
	void	(APIENTRY *Ortho)(GLdouble left, GLdouble right, GLdouble bottom,
			GLdouble top, GLdouble zNear, GLdouble zFar);
	void	(APIENTRY *Frustum)(GLdouble left, GLdouble right, GLdouble bottom,
			GLdouble top, GLdouble zNear, GLdouble zFar);
	void	(APIENTRY *Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);
	void	(APIENTRY *Scissor)(GLint x, GLint y, GLsizei width, GLsizei height);
	void	(APIENTRY *PushMatrix)(void);
	void	(APIENTRY *PopMatrix)(void);
	void	(APIENTRY *LoadIdentity)(void);
	void	(APIENTRY *LoadMatrixd)(const GLdouble *m);
	void	(APIENTRY *MultMatrixd)(const GLdouble *m);
	void	(APIENTRY *Rotated)(GLdouble, GLdouble, GLdouble, GLdouble);
	void	(APIENTRY *Scaled)(GLdouble, GLdouble, GLdouble);
	void	(APIENTRY *Translated)(GLdouble x, GLdouble y, GLdouble z);

	/* Textures */
	GLboolean (APIENTRY *IsTexture)(GLuint texture);
	void	(APIENTRY *GenTextures)(GLsizei n, GLuint *textures);
	void	(APIENTRY *BindTexture)(GLenum, GLuint);
	void	(APIENTRY *DeleteTextures)(GLsizei n, const GLuint *textures);
	void	(APIENTRY *TexImage2D)(GLenum target, GLint level, GLint internalformat,
			GLsizei width, GLsizei height, GLint border,
			GLenum format, GLenum type, const GLvoid *pixels);
	void	(APIENTRY *TexParameteri)(GLenum target, GLenum pname, GLint param);
	void	(APIENTRY *TexParameterf)(GLenum target, GLenum pname, GLfloat param);
	void	(APIENTRY *TexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
	void	(APIENTRY *TexSubImage2D)(GLenum target, GLint level,
			GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
			GLenum format, GLenum type, const GLvoid *pixels);

	/* Lighting */
	void	(APIENTRY *ShadeModel)(GLenum mode);

	/* Raster */
	void	(APIENTRY *PixelStorei)(GLenum pname, GLint param);
	void	(APIENTRY *ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height,
			GLenum format, GLenum type, GLvoid *pixels);

	/* Drawing */
	void	(APIENTRY *Begin)(GLenum);
	void	(APIENTRY *End)(void);
	void	(APIENTRY *Vertex2d)(GLdouble x, GLdouble y);
	void	(APIENTRY *Vertex3d)(GLdouble x, GLdouble y, GLdouble z);
	void	(APIENTRY *Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void	(APIENTRY *Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
	void	(APIENTRY *Color3d)(GLdouble red, GLdouble green, GLdouble blue);
	void	(APIENTRY *Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
	void	(APIENTRY *Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void	(APIENTRY *TexCoord1d)(GLdouble s);
	void	(APIENTRY *TexCoord2d)(GLdouble s, GLdouble t);
	void	(APIENTRY *TexCoord3d)(GLdouble s, GLdouble t, GLdouble u);
	void	(APIENTRY *TexCoord4d)(GLdouble s, GLdouble t, GLdouble u, GLdouble v);

	/* Arrays */
	void	(APIENTRY *VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void	(APIENTRY *TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void	(APIENTRY *DrawArrays)(GLenum mode, GLint first, GLsizei count);

	/*
	 * Calls below this point may be missing - CHECK FOR NULL!!!
	 */

		/* OpenGL 1.2 core functions */
	void	(APIENTRY *_BlendEquation)(GLenum);

	/* 3.0+ Texture handling */
	void	(APIENTRY *_GenerateMipmap)(GLenum target);

	/*
	 * OpenGL ntate cache
	 */
	struct
	{
		int	blend;
		int	texture_2d;
		int	scissor_test;
	} caps;
	GLenum	sfactor;
	GLenum	dfactor;
	GLuint	texture2d;

	/*
	 * OpenGL version info
	 */
	int	version;	/* (MAJOR.MINOR) * 10 */
} ZD_glinterface;

/*
 * Open/close
 */
ZD_glinterface *gli_Open(char *libname);
void gli_Close(ZD_glinterface *gli);


/*
 * OpenGL state control
 */

static inline void gli_Reset(ZD_glinterface *gli)
{
	gli->caps.blend = -1;
	gli->caps.texture_2d = -1;
	gli->caps.scissor_test = -1;
	gli->texture2d = -1;
	gli->sfactor = 0xffffffff;
	gli->dfactor = 0xffffffff;
}

static inline int *gli_GetState(ZD_glinterface *gli, GLenum cap)
{
	switch(cap)
	{
	  case GL_BLEND:
		return &gli->caps.blend;
	  case GL_TEXTURE_2D:
		return &gli->caps.texture_2d;
	  case GL_SCISSOR_TEST:
		return &gli->caps.scissor_test;
	  default:
		return NULL;
	}
}

static inline void gli_Enable(ZD_glinterface *gli, GLenum cap)
{
	int *state = gli_GetState(gli, cap);
	if(!state || (*state != 1))
	{
		gli->Enable(cap);
		if(state)
			*state = 1;
	}
}

static inline void gli_Disable(ZD_glinterface *gli, GLenum cap)
{
	int *state = gli_GetState(gli, cap);
	if(!state || (*state != 0))
	{
		gli->Disable(cap);
		if(state)
			*state = 0;
	}
}

static inline void gli_BlendFunc(ZD_glinterface *gli, GLenum sfactor, GLenum dfactor)
{
	if((sfactor == gli->sfactor) && (dfactor == gli->dfactor))
		return;
	gli->BlendFunc(sfactor, dfactor);
	gli->sfactor = sfactor;
	gli->dfactor = dfactor;
}

static inline void gli_BindTexture(ZD_glinterface *gli, GLenum target, GLuint tx)
{
	if(target != GL_TEXTURE_2D)
	{
		gli->BindTexture(target, tx);
		return;
	}
	if(tx == gli->texture2d)
		return;
	gli->BindTexture(target, tx);
	gli->texture2d = tx;
}

#endif /* ZD_GLI_H */
