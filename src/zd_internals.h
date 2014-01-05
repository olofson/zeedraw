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

#ifndef	ZD_INTERNALS_H
#define	ZD_INTERNALS_H

#include "zeedraw.h"
#include <math.h>


typedef struct ZD_backend ZD_backend;


/*---------------------------------------------------------
	Error handling
---------------------------------------------------------*/

extern ZD_errors zd_lasterror;


/*---------------------------------------------------------
	States
---------------------------------------------------------*/

struct ZD_state
{
	ZD_entity	*root;
	ZD_texture	*textures;
	ZD_entity	*pool;
	ZD_backend	*backend;
	void		*bdata;
	void		*context;
	unsigned	flags;		/* ZD_openflags */
	ZD_errors	lasterror;
	unsigned	entitysize;	/* Actual size of ZD_entity (bytes) */
	unsigned	texturesize;	/* Actual size of ZD_texture (bytes)*/
	ZD_f		now;		/* Current mod/fx time */
	ZD_f		vl, vr, vb, vt;	/* View extents */
};

static inline void zd_BumpEntitySize(ZD_state *st, unsigned size)
{
	if(size > st->entitysize)
		st->entitysize = size;
}

static inline void zd_BumpTextureSize(ZD_state *st, unsigned size)
{
	if(size > st->texturesize)
		st->texturesize = size;
}


/*---------------------------------------------------------
	Entities
---------------------------------------------------------*/

/* Scene graph entity kinds */
typedef enum ZD_entitykind
{
	ZD_EROOT = 0,
	ZD_ELAYER,
	ZD_EWINDOW,
	ZD_EGROUP,
	ZD_ESPRITE,
	ZD_EPRIMITIVE,
	ZD_EFILL
} ZD_entitykind;

/* Entity header */
struct ZD_entity
{
	ZD_state	*state;
	ZD_entity	*parent;
	ZD_entity	*next;		/* Siblings */
	ZD_entity	*first, *last;	/* Children */

	/* Backend methods */
	ZD_errors (*Rethink)(ZD_entity *e);
	ZD_errors (*Render)(ZD_entity *e);
	ZD_errors (*RenderPost)(ZD_entity *e);
	void (*Destroy)(ZD_entity *e);
#if 0
	/* Application callbacks */
	ZD_entitycb	OnRender;
	void		*onrender_ud;
	ZD_entitycb	OnTransformChange;
	void		*ontransformchange_ud;
#endif
	ZD_entitykind	kind;
	int		refcount;

	/* Parameters */
	unsigned	flags;
	float		cr, cg, cb, ca;
	ZD_f		x, y, z, s, r;
	ZD_f		dx, dy, dz, ds, dr;

	/* Parameters transformed for rendering */
	ZD_f		tx, ty, tz, ts, tr;
	float		tcr, tcg, tcb, tca;

	/* Rotation + scaling matrix for coordinates and children */
	ZD_f		trmx[4];
};

/* Layer entity */
typedef struct ZD_layer
{
	ZD_entity	e;
	ZD_f		left, right, bottom, top;
	float		bgr, bgg, bgb, bga;
} ZD_layer;

/* Window entity */
typedef struct ZD_window
{
	ZD_layer	l;
	ZD_f		w, h;
} ZD_window;

/* Textured entity (internal) */
typedef struct ZD_txentity
{
	ZD_entity	e;
	ZD_texture	*texture;
} ZD_txentity;

/* Sprite entity */
typedef struct ZD_sprite
{
	ZD_txentity	txe;
	ZD_f		cx, cy;
} ZD_sprite;

typedef struct ZD_vertex
{
	ZD_f	x, y, z;	/* Vertex coordinate */
	ZD_f	tx, ty;		/* Texture coordinate */
} ZD_vertex;

/* Graphics primitive entity */
typedef struct ZD_primitive
{
	ZD_txentity	txe;
	ZD_primitives	pkind;
	unsigned	nvertices;	/* Vertices in use */
	unsigned	svertices;	/* Size of vertex array */
	ZD_vertex	*vertices;	/* Vertex array */
} ZD_primitive;

/* Parent area fill entity */
typedef struct ZD_fill
{
	ZD_txentity	txe;
	ZD_layer	*client;
} ZD_fill;

ZD_entity *zd_alloc_entity(ZD_state *st);
void zd_DestroyEntity(ZD_entity *e);

static inline ZD_entity *zd_NewEntity(ZD_entity *parent, ZD_entitykind kind)
{
	ZD_state *st = parent->state;
	ZD_entity *e = st->pool;
	if(e)
		st->pool = e->next;
	else if(!(e = zd_alloc_entity(st)))
		return NULL;
	e->kind = kind;
	e->parent = parent;
	e->Rethink = NULL;
	e->RenderPost = NULL;
	e->Destroy = NULL;
	/*
	 * NOTE:
	 *	Entities are owned by the parent entity - NOT the application!
	 */
	e->refcount = 1;
	return e;
}

static inline void zd_LinkEntity(ZD_entity *e)
{
	ZD_entity *p = e->parent;
	if(p->last)
	{
		e->next = p->last;
		p->last->next = e;
		p->last = e;
	}
	else
		p->first = p->last = e;
	e->next = NULL;
}

static inline void zd_FreeEntity(ZD_entity *e)
{
	ZD_state *st = e->state;
	e->next = st->pool;
	st->pool = e;
}

static inline void zd_EntityIncRef(ZD_entity *e)
{
	++e->refcount;
}

static inline void zd_EntityDecRef(ZD_entity *e)
{
	if(--e->refcount <= 0)
		zd_DestroyEntity(e);
}


/*---------------------------------------------------------
	2x2 matrix tools
---------------------------------------------------------*/

/* Calculate 2x2 matrix for performing the specified scale + rotation */
static inline void zd_CalculateMatrix(ZD_f rotation, ZD_f scale, ZD_f *m)
{
	ZD_f cosr = cos(rotation);
	ZD_f sinr = sin(rotation);
	m[0] = cosr * scale;
	m[1] = -sinr * scale;
	m[2] = sinr * scale;
	m[3] = cosr * scale;
}

/* Calculate inverse of 2x2 matrix */
static inline ZD_errors zd_InverseMatrix(ZD_f *m, ZD_f *mi)
{
	ZD_f d = m[0] * m[3] - m[1] * m[2];
	ZD_f m0 = m[0];
	if(!d)
		return ZD_DIVBYZERO;
	d = 1.0f / d;
	mi[0] = m[3] * d;
	mi[1] = -m[1] * d;
	mi[2] = -m[2] * d;
	mi[3] = m0 * d;
	return ZD_OK;
}

/* Multiply two 2x2 matrices */
static inline void zd_MultiplyMatrix(ZD_f *ma, ZD_f *mb, ZD_f *mr)
{
	ZD_f r0 = ma[0] * mb[0] + ma[1] * mb[2];
	ZD_f r1 = ma[0] * mb[1] + ma[1] * mb[3];
	ZD_f r2 = ma[2] * mb[0] + ma[3] * mb[2];
	ZD_f r3 = ma[2] * mb[1] + ma[3] * mb[3];
	mr[0] = r0;
	mr[1] = r1;
	mr[2] = r2;
	mr[3] = r3;
}

/* Transform (x, y) into (tx, ty) using the specified matrix and offsets */
static inline void zd_TransformPoint(ZD_f *m, ZD_f xoffs, ZD_f yoffs,
		ZD_f x, ZD_f y, ZD_f *tx, ZD_f *ty)
{
	*tx = x * m[0] + y * m[1] + xoffs;
	*ty = x * m[2] + y * m[3] + yoffs;
}

/* Inverse transform; offsets are subtracted before instead of added after */
static inline void zd_InvTransformPoint(ZD_f *m, ZD_f xoffs, ZD_f yoffs,
		ZD_f x, ZD_f y, ZD_f *tx, ZD_f *ty)
{
	x -= xoffs;
	y -= yoffs;
	*tx = x * m[0] + y * m[1];
	*ty = x * m[2] + y * m[3];
}

/* Transform (x, y) into (tx, ty) using the precalculated transform of 'e' */
static inline void zd_TransformPointE(ZD_entity *e, ZD_f x, ZD_f y,
		ZD_f *tx, ZD_f *ty)
{
	zd_TransformPoint(e->trmx, e->tx, e->ty, x, y, tx, ty);
}

/* Transform (x, y) into (tx, ty) using the inverse transform of 'e' */
static inline void zd_InvTransformPointE(ZD_entity *e, ZD_f x, ZD_f y,
		ZD_f *tx, ZD_f *ty)
{
	ZD_f m[4];
	ZD_f si = e->ts ? 1.0f / e->ts : 1000.0f;
	zd_CalculateMatrix(-e->tr, si, m);
	zd_InvTransformPoint(m, e->tx, e->ty, x, y, tx, ty);
}


/*---------------------------------------------------------
	Textures
---------------------------------------------------------*/

typedef enum ZD_textypes
{
	ZD_TT_PHYSICAL = 0,
	ZD_TT_VIRTUAL,
	ZD_TT_SUBTEXTURE
} ZD_textypes;

typedef struct ZD_phystexture
{
	unsigned char	*pixels;
	void		*rdata;	/* Private renderer data, if any */
	ZD_errors (*Render)(ZD_state *st, ZD_texture *tx);
	void (*Unload)(ZD_state *st, ZD_entity *e);
} ZD_phystexture;

typedef struct ZD_virtualtexture
{
	int		w, h;	/* Size of tile map */
} ZD_virtualtexture;

typedef struct ZD_subtexture
{
	ZD_texture	*phys;	/* Physical texture */
	int		x, y;	/* Offset inside physical texture */
} ZD_subtexture;

struct ZD_texture
{
	ZD_state	*state;
	ZD_texture	*next;

	/* For ONDEMAND and VIRTUAL */
	ZD_texrendercb	Render;
	void		*userdata;

	ZD_textypes	type;
	int		refcount;
	unsigned	w, h;
	ZD_pixelformats	format;
	int		flags;		/* ZD_texflags */

//TODO: Filtering, wrapping, mipmapping etc

	union {
		ZD_phystexture		p;
		ZD_virtualtexture	v;
		ZD_subtexture		s;
	} t;
};


void zd_DestroyTexture(ZD_texture *tx);

static inline void zd_TextureIncRef(ZD_texture *tx)
{
	++tx->refcount;
}

static inline void zd_TextureDecRef(ZD_texture *tx)
{
	if(--tx->refcount <= 0)
		zd_DestroyTexture(tx);
}


/*---------------------------------------------------------
	Backends
---------------------------------------------------------*/

struct ZD_backend
{
	ZD_errors (*Open)(ZD_state *st);
	void (*Close)(ZD_state *st);

	/* Top level scene rendering */
	ZD_errors (*PreRender)(ZD_state *st);
	ZD_errors (*PostRender)(ZD_state *st);

	/* Entity initializers */
	ZD_errors (*InitLayer)(ZD_entity *e);
	ZD_errors (*InitWindow)(ZD_entity *e);
	ZD_errors (*InitGroup)(ZD_entity *e);
	ZD_errors (*InitSprite)(ZD_entity *e);
	ZD_errors (*InitPrimitive)(ZD_entity *e);
	ZD_errors (*InitFill)(ZD_entity *e);

	/* Texture management */
	ZD_errors (*InitTexture)(ZD_texture *tx);
	ZD_errors (*UploadTexture)(ZD_pixels *px);
	ZD_errors (*CloseTexture)(ZD_texture *tx);
};


#endif	/*ZD_INTERNALS_H*/
