/*
 * ZeeDraw - Scene Graph Rendering Engine
 *
 * Copyright 2013 David Olofson
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

/*
 * NOTE:
 *	* All angles (rotation and the like) are in radians.
 */

#ifndef	ZD_ZEEDRAW_H
#define	ZD_ZEEDRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------
	NULL (stolen from the GNU C Library)
---------------------------------------------------------*/
#ifndef NULL
#	if defined __GNUG__ &&					\
			(__GNUC__ > 2 ||			\
			(__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#		define NULL (__null)
#	else
#		if !defined(__cplusplus)
#			define NULL ((void*)0)
#		else
#			define NULL (0)
#		endif
#	endif
#endif


typedef double ZD_f;


/*---------------------------------------------------------
	Structures
---------------------------------------------------------*/

/* Pixel formats */
typedef enum
{
	ZD_OFF = 0,	/* No texture! (Typically all white.) */
	ZD_I,		/* I 8 (grayscale Intensity channel) */
	ZD_RGB,		/* RGB (.byte R, G, B, 0) */
	ZD_RGBA		/* RGBA (.byte R, G, B, A) */
} ZD_pixelformats;

static inline int zd_PixelSize(ZD_pixelformats format)
{
	switch(format)
	{
	  case ZD_OFF:
		return 0;
	  case ZD_I:
		return 1;
	  case ZD_RGB:
	  case ZD_RGBA:
		return 4;
	}
	return 0;
}


/* ZeeDraw "base classes" */
typedef struct ZD_state ZD_state;
typedef struct ZD_entity ZD_entity;
typedef struct ZD_texture ZD_texture;


/*---------------------------------------------------------
	Error handling
---------------------------------------------------------*/

#define ZD_ALLERRORS	\
  ZD_DEFERR(ZD_OOMEMORY,	"Out of memory")\
  ZD_DEFERR(ZD_DIVBYZERO,	"Division by zero")\
  ZD_DEFERR(ZD_NOTIMPLEMENTED,	"Feature not implemented")\
  ZD_DEFERR(ZD_NOTSUPPORTED,	"Operation not supported")\
  ZD_DEFERR(ZD_DENIED,		"Operation denied")\
  ZD_DEFERR(ZD_NOBACKEND,	"Could not find requested backend")\
  ZD_DEFERR(ZD_BACKENDOPEN,	"Could not open backend")\
  ZD_DEFERR(ZD_DRIVEROPEN,	"Could not open driver")\
  ZD_DEFERR(ZD_BADFORMAT,	"Unknown data format")\
  ZD_DEFERR(ZD_BADARGUMENTS,	"Arguments do not make sense")\
  ZD_DEFERR(ZD_BADPRIMITIVE,	"Unknown primitive kind")\
  ZD_DEFERR(ZD_WRONGTYPE,	"Wrong type of object")\
  ZD_DEFERR(ZD_NOTLOCKED,	"Object not locked")\
  ZD_DEFERR(ZD_UNLOCKED,	"Object already unlocked")\
  ZD_DEFERR(ZD_CLIPPING,	"Region requires clipping")\
  ZD_DEFERR(ZD_INVALIDPARENT,	"Entity cannot be child of specified parent")\
  ZD_DEFERR(ZD_INVALIDPARAM,	"Invalid parameter")\
  \
  ZD_DEFERR(ZD_INTERNAL,	"INTERNAL ERROR")

#define	ZD_DEFERR(x, y)	x,
typedef enum ZD_errors
{
	ZD_OK = 0,
	ZD_ALLERRORS
} ZD_errors;
#undef	ZD_DEFERR

/*
 * Get last error for 'state'. If 'state' is NULL, this returns the error code
 * from the last zd_Open() call.
 */
ZD_errors zd_LastError(ZD_state *state);
const char *zd_ErrorString(ZD_errors errorcode);


/*---------------------------------------------------------
	States
---------------------------------------------------------*/

typedef enum ZD_openflags
{
	ZD__DUMMY = 0
} ZD_openflags;

ZD_state *zd_Open(const char *renderer, ZD_openflags flags, void *context);
void zd_Close(ZD_state *state);


/*---------------------------------------------------------
	Textures
---------------------------------------------------------*/

typedef enum ZD_texflags
{
	/* Vertical clamp/wrap mode */
	ZD__VMODE =		0x0000000f,
	ZD_VUNDEF =		0x00000000,
	ZD_VCLAMP =		0x00000001,
	ZD_VWRAP =		0x00000002,

	/* Horizontal clamp/wrap mode */
	ZD__HMODE =		0x000000f0,
	ZD_HUNDEF =		0x00000000,
	ZD_HCLAMP =		0x00000010,
	ZD_HWRAP =		0x00000020,

	/* Scaling modes */
	ZD__SMODE =		0x00000f00,
	ZD_NEAREST =		0x00000000,
	ZD_BILINEAR =		0x00000100,
	ZD_BILINEAR_MIPMAP =	0x00000200,
	ZD_TRILINEAR_MIPMAP =	0x00000300,

	/*
	 * VIRTUAL and ONDEMAND textures are physically allocated and rendered
	 * only when actually used. They are typically cashed, but the exact
	 * behavior depends on backend implementation and available texture
	 * memory.
	 *   ONDEMAND textures differ from normal textures only in that the
	 * actual allocation is deferred, and pixel data uploading is done as
	 * needed via callback instead of zd_TextureWrite().
	 *   ZD_VIRTUAL is intended for large textures that are only partially
	 * used in any single scene; for example huge scrolling backgrounds.
	 */
	ZD_VIRTUAL =		0x00001000,
	ZD_ONDEMAND =		0x00002000,

	/*
	 * Do not clear allocated memory. This give undefined results (garbage)
	 * if textures are used before fully defined via zd_TextureWrite() or
	 * callbacks!
	 */
	ZD_NOCLEAR =		0x00010000,

	/* Internal state flags */
	ZD_UNDEFINED =		0x00100000
} ZD_texflags;

/* Descriptor for locked texture areas */
typedef struct ZD_pixels
{
	ZD_texture	*texture;
	unsigned char	*pixels;
	unsigned	x, y, w, h;
	unsigned	pitch;
	ZD_pixelformats	format;
} ZD_pixels;

/* Virtual texture rendering callback */
typedef ZD_errors (*ZD_texrendercb)(ZD_pixels *pixels, void *userdata);

/* Create a new texture */
ZD_texture *zd_Texture(ZD_state *state, ZD_pixelformats format,
		ZD_texflags flags, unsigned w, unsigned h);

/* Create a new on-demand rendered texture */
ZD_texture *zd_OnDemandTexture(ZD_state *state, ZD_pixelformats format,
		ZD_texflags flags, unsigned w, unsigned h,
		ZD_texrendercb callback, void *userdata);

/* Write pixels into a texture */
ZD_errors zd_TextureWrite(ZD_texture *texture,
		int x, int y, unsigned w, unsigned h,
		ZD_pixelformats format, void *pixels, unsigned stride);

/* Create a new texture from raw data */
ZD_texture *zd_TextureFromData(ZD_state *state, ZD_pixelformats format,
		ZD_texflags flags, unsigned w, unsigned h, void *pixels);

/* Set rendering callback for a virtual texture */
ZD_errors zd_TextureOnRender(ZD_texture *texture,
		ZD_texrendercb callback, void *userdata);

ZD_errors zd_LockTexture(ZD_texture *texture, ZD_pixels *pixels);
ZD_errors zd_LockTextureRegion(ZD_texture *texture, ZD_pixels *pixels,
		unsigned x, unsigned y, unsigned w, unsigned h);
ZD_errors zd_UnlockTexture(ZD_pixels *pixels);

/* Texture ownership management */
void zd_RetainTexture(ZD_texture *tx);
void zd_ReleaseTexture(ZD_texture *tx);


/*---------------------------------------------------------
	Entities
---------------------------------------------------------*/

/* Entity creation and state flags */
typedef enum ZD_entityflags
{
	/* Entity rendering modes */
	ZD__RENDERMODE =	0x0000000f,

	/*
	 * Persistent scene graph rendering mode
	 *	* Entity API calls create new, persistent entities.
	 *	* No rendering is done by entity API calls.
	 *	* Rendering is done by zd_Render().
	 */
	ZD_PERSISTENT =		0x00000000,
#if 0
	/*
	 * Immediate rendering mode
	 *	* Rendering is done directly in entity API calls.
	 *	* Non rendering API calls are not available;
	 *		zd_Group(), zd_Window()
	 */
	ZD_IMMEDIATE =		0x00000001,
#endif
	ZD_VISIBLE =		0x00000010,	/* Entity is visible */
	ZD_BUFFERED =		0x00000020,	/* Buffered rendering */
	ZD_CLEAR =		0x00000040,	/* Clear area before rendering */
	ZD_SETORIGO =		0x00000080,	/* Set new origo (window) */
	ZD_CLIP =		0x00000100,	/* Enable clipping (window) */
	ZD_ANIMATED =		0x00001000,	/* Entity is animated */
	ZD_RETHINK =		0x00002000	/* Recalculate transforms */
} ZD_entityflags;


/* Entity parameter indices */
typedef enum ZD_parameter
{
	/* Positions etc */
	ZD_X = 0,
	ZD_Y,
	ZD_Z,
	ZD_SCALE,
	ZD_ROTATION,

	/* Velocities */
	ZD_VX,
	ZD_VY,
	ZD_VZ,
	ZD_VSCALE,
	ZD_VROTATION,

	/* Color modulation */
	ZD_RED,
	ZD_GREEN,
	ZD_BLUE,
	ZD_ALPHA,

	/* Rotation/scaling matrix */
	ZD_MX0,
	ZD_MX1,
	ZD_MX2,
	ZD_MX3,

	/* View (windows and layers) */
	ZD_LEFT,
	ZD_RIGHT,
	ZD_BOTTOM,
	ZD_TOP,

	/* Width/height (windows) */
	ZD_WIDTH,
	ZD_HEIGHT,

	/* Hotspot (sprites) */
	ZD_CX,
	ZD_CY,

	/* Background color (windows and layers with ZD_CLEAR flag) */
	ZD_BGRED,
	ZD_BGGREEN,
	ZD_BGBLUE,
	ZD_BGALPHA,

	/* Number of parameter indices */
	ZD__PARAMETERS
} ZD_parameter;


/* General callback prototype for entity events */
typedef ZD_errors (*ZD_entitycb)(ZD_entity *entity, void *userdata);

/* Get state root entity */
ZD_entity *zd_Root(ZD_state *state);

/* Get first child of entity */
ZD_entity *zd_First(ZD_entity *parent);

/* Get next entity in list */
ZD_entity *zd_Next(ZD_entity *entity);

/* Entity ownership management */
void zd_RetainEntity(ZD_entity *e);
void zd_ReleaseEntity(ZD_entity *e);


/*---------------------------------------------------------
	Entity creation
---------------------------------------------------------*/

/*
 * Create a new layer entity
 *
 *	A layer is a window covering the entire display, with a coordinate
 *	where left, right, bottom and top correspond to the respective edges of
 *	the display.
 */
ZD_entity *zd_Layer(ZD_entity *parent, ZD_entityflags flags,
		ZD_f left, ZD_f right, ZD_f bottom, ZD_f top);

/*
 * Create a new window entity
 *
 *	A window is similar to a group, but adds clipping. x, y, w and h define
 *	the size and position of the window in terms of the parent coordinate
 *	system, and the window view is set up so that the local coordinate
 *	system still matches that of the parent entity.
 *
 * NOTE:
 *	Scale and rotate transforms set on this entity are applied to its
 *	children only - not the clipping window!
 */
ZD_entity *zd_Window(ZD_entity *parent, ZD_entityflags flags,
		ZD_f x, ZD_f y, ZD_f w, ZD_f h);

/*
 * Create a new group entity
 *
 *	A group is a non-rendering entity that applies transforms to its
 *	children.
 *
 * NOTE:
 *	Being non-rendering, a group does not have a position or orientation
 *	of its own! Those arguments passed here are equivalent to setting the
 *	transform using zd_Transform().
 */
ZD_entity *zd_Group(ZD_entity *parent, ZD_entityflags flags);

/*
 * Create a sprite entity
 *
 *	(cx, cy) defines the hot-spot of the texture, (0, 0) being the top-left
 *	corner of the texture and (1, 1) being the bottom right. This is used
 *	as the reference point for (x, y), scaling and rotation.
 *	   'size' and 'rotation' are actually the scale and rotation of the
 *	sprite entity. (The actual sprite size is always 1x1.)
 */
ZD_entity *zd_Sprite(ZD_entity *parent, ZD_entityflags flags,
		ZD_texture *texture, ZD_f cx, ZD_f cy);

/*
 * Create a "fill" entity
 *
 *	This entity applies a texture to the full area of the parent entity.
 *	The entity transforms are applied to the texture offset, scale and
 *	rotation, as well as any child entities.
 */
ZD_entity *zd_Fill(ZD_entity *parent, ZD_entityflags flags, ZD_texture *texture);


/*---------------------------------------------------------
	Entity control
---------------------------------------------------------*/

/* Set the position, scale and rotation of an entity */
ZD_errors zd_SetTransform(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z, ZD_f s, ZD_f r);
ZD_errors zd_SetPosition(ZD_entity *e, ZD_f x, ZD_f y);
ZD_errors zd_SetPosition3D(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z);
ZD_errors zd_SetScale(ZD_entity *e, ZD_f scale);
ZD_errors zd_SetRotation(ZD_entity *e, ZD_f rotation);

/* Change the position, scale and rotation of an entity */
ZD_errors zd_Move(ZD_entity *e, ZD_f x, ZD_f y);
ZD_errors zd_Move3D(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z);
ZD_errors zd_Scale(ZD_entity *e, ZD_f scale);
ZD_errors zd_Rotate(ZD_entity *e, ZD_f rotation);

/* Set color of entity */
ZD_errors zd_SetColor(ZD_entity *e, float r, float g, float b, float a);

/* Set background color of a layer or window entity */
ZD_errors zd_SetBGColor(ZD_entity *e, float r, float g, float b, float a);

/* Set/change texture of entity */
ZD_errors zd_SetTexture(ZD_entity *e, ZD_entityflags flags, ZD_texture *texture);

/* Change view (coordinate system) of a layer or window entity */
ZD_errors zd_SetView(ZD_entity *e, ZD_f left, ZD_f right, ZD_f bottom, ZD_f top);

/* Set/get single parameter */
ZD_errors zd_SetParameter(ZD_entity *e, ZD_parameter param, ZD_f value);
ZD_errors zd_GetParameter(ZD_entity *e, ZD_parameter param, ZD_f *value);

/* Set/get multiple parameters */
ZD_errors zd_SetParameters(ZD_entity *e,
		unsigned count, unsigned *params, ZD_f *values);
ZD_errors zd_GetParameters(ZD_entity *e,
		unsigned count, unsigned *params, ZD_f *values);


/*---------------------------------------------------------
	Entity animation
---------------------------------------------------------*/

/* Set continuos movement/rotation (units per second) */
ZD_errors zd_CMove(ZD_entity *e, ZD_f x, ZD_f y);
ZD_errors zd_CMove3D(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z);
ZD_errors zd_CScale(ZD_entity *e, ZD_f scale);
ZD_errors zd_CRotate(ZD_entity *e, ZD_f rotation);

/* Stop any animation of this entity */
ZD_errors zd_CStop(ZD_entity *e);

#if 0
/* Set rendering callback for an immediate mode entity */
ZD_errors zd_OnRender(ZD_entity *e, ZD_entitycb callback, void *userdata);

/* Set update callback for entity transform changes */
ZD_errors zd_OnTransformChange(ZD_entity *e, ZD_entitycb callback, void *userdata);
#endif


/*---------------------------------------------------------
	Primitives
---------------------------------------------------------*/

typedef enum ZD_primitives
{
	ZD_POINTS = 0,
	ZD_LINES,
	ZD_LINESTRIP,
	ZD_LINELOOP,
	ZD_TRIANGLES,
	ZD_TRIANGLESTRIP,
	ZD_TRIANGLEFAN,
	ZD_QUADS
} ZD_primitives;

ZD_entity *zd_Primitive(ZD_entity *parent, ZD_entityflags flags,
		ZD_primitives pkind, ZD_texture *texture,
		ZD_f x, ZD_f y, ZD_f size, ZD_f rotation);

/* Primitive data interface */
ZD_errors zd_Vertex2D(ZD_entity *entity, ZD_f x, ZD_f y);
ZD_errors zd_Vertex3D(ZD_entity *entity, ZD_f x, ZD_f y, ZD_f z);
ZD_errors zd_Vertices(ZD_entity *entity, unsigned dimensions, unsigned count,
		ZD_f *data);
ZD_errors zd_TexCoord(ZD_entity *entity, ZD_f x, ZD_f y);
ZD_errors zd_TexCoords(ZD_entity *entity, unsigned count, ZD_f *data);


/*---------------------------------------------------------
	Modulation and effects
---------------------------------------------------------*/

/* Advance state time by 'dt' seconds */
ZD_errors zd_Advance(ZD_state *state, ZD_f dt);


/*---------------------------------------------------------
	Rendering
---------------------------------------------------------*/

ZD_errors zd_Render(ZD_state *state);

#ifdef __cplusplus
};
#endif

#endif /*ZD_ZEEDRAW_H*/
