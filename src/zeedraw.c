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

#include "zd_internals.h"
#include "zd_opengl.h"
#include "zd_software.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* Create a root entity (no parent!) */
static ZD_entity *zd_create_root(ZD_state *st)
{
	ZD_entity *e = zd_alloc_entity(st);
	if(!e)
		return NULL;
	/* NOTE: We rely on zd_alloc_entity() to return a zero-filled block! */
	e->kind = ZD_EROOT;
	e->flags = ZD_RETHINK | ZD_VISIBLE;
	e->refcount = 1;
	e->s = 1.0f;
	e->cr = 1.0f;
	e->cg = 1.0f;
	e->cb = 1.0f;
	e->ca = 1.0f;
	if(st->backend->InitGroup)
		if((st->lasterror = st->backend->InitGroup(e)))
			return NULL;
	return e;
}


/* Destroy entity 'e', which must not be linked into any list! */
static void zd_destroy_entity(ZD_entity *e)
{
	while(e->first)
	{
		ZD_entity *ne = e->first->next;
		zd_destroy_entity(e->first);
		e->first = ne;
	}
	if(e->Destroy)
		e->Destroy(e);
	switch(e->kind)
	{
	  case ZD_EROOT:
	  case ZD_ELAYER:
	  case ZD_EWINDOW:
	  case ZD_EGROUP:
		break;
	  case ZD_ESPRITE:
	  case ZD_EPRIMITIVE:
	  case ZD_EFILL:
	  {
		ZD_txentity *txe = (ZD_txentity *)e;
		if(txe->texture)
			zd_TextureDecRef(txe->texture);
		break;
	  }
	}
	zd_FreeEntity(e);
}


/*---------------------------------------------------------
-----------------------------------------------------------
	States
-----------------------------------------------------------
---------------------------------------------------------*/

ZD_state *zd_Open(const char *renderer, ZD_openflags flags, void *context)
{
	ZD_state *st;
	zd_lasterror = ZD_OK;
	if(!(st = (ZD_state *)calloc(1, sizeof(ZD_state))))
	{
		zd_lasterror = ZD_OOMEMORY;
		return NULL;
	}
	st->flags = flags;
	st->context = context;
	zd_BumpEntitySize(st, sizeof(ZD_layer));
	zd_BumpEntitySize(st, sizeof(ZD_window));
	zd_BumpEntitySize(st, sizeof(ZD_sprite));
	zd_BumpEntitySize(st, sizeof(ZD_fill));
	zd_BumpTextureSize(st, sizeof(ZD_texture));
	if(!renderer || !strcmp(renderer, "opengl"))
		st->backend = &zd_opengl_backend;
	else if(!strcmp(renderer, "software"))
		st->backend = &zd_software_backend;
	if(!st->backend)
	{
		free(st);
		zd_lasterror = ZD_NOBACKEND;
		return NULL;
	}
	if((zd_lasterror = st->backend->Open(st)))
	{
		free(st);
		zd_lasterror = ZD_BACKENDOPEN;
		return NULL;
	}
	st->root = zd_create_root(st);
	return st;
}


void zd_Close(ZD_state *state)
{
	zd_destroy_entity(state->root);
	while(state->pool)
	{
		ZD_entity *e = state->pool;
		state->pool = e->next;
		free(e);
	}
	state->root = NULL;
	state->backend->Close(state);
	free(state);
}


/*---------------------------------------------------------
-----------------------------------------------------------
	Textures
-----------------------------------------------------------
---------------------------------------------------------*/

static inline void zd_PixelsFromTexture(ZD_pixels *pixels, ZD_texture *texture)
{
	pixels->texture = texture;
	pixels->pixels = texture->t.p.pixels;
	pixels->x = 0;
	pixels->y = 0;
	pixels->w = texture->w;
	pixels->h = texture->h;
	pixels->pitch = texture->w * zd_PixelSize(texture->format);
	pixels->format = texture->format;
}


static ZD_errors zd_default_texonrender(ZD_pixels *pixels, void *userdata)
{
	int y;
	unsigned char *px = (unsigned char *)(pixels->pixels);
	int pxsize = zd_PixelSize(pixels->format);
	if(!pxsize)
		return ZD_OK;
	for(y = 0; y < pixels->h; ++y)
		memset(px + y * pixels->pitch, -1, pixels->w * pxsize);
	return ZD_OK;
}


ZD_errors zd_TextureOnRender(ZD_texture *texture,
		ZD_texrendercb callback, void *userdata)
{
	if(!(texture->flags & (ZD_VIRTUAL | ZD_ONDEMAND)))
		return ZD_NOTSUPPORTED;
	if(callback)
	{
		texture->Render = callback;
		texture->userdata = userdata;
	}
	else
	{
		texture->Render = zd_default_texonrender;
		texture->userdata = NULL;
	}
	return ZD_OK;
}


static void zd_FreeTexturePixels(ZD_texture *texture)
{
	if(texture->t.p.pixels)
		free(texture->t.p.pixels);
}


static ZD_errors zd_AllocTexturePixels(ZD_texture *texture)
{
	int pxsize = zd_PixelSize(texture->format);
	zd_FreeTexturePixels(texture);
	texture->flags |= ZD_UNDEFINED;
/*TODO: Padding for row alignment! */
	texture->t.p.pixels = malloc(pxsize * texture->w * texture->h);
	if(!texture->t.p.pixels)
		return ZD_OOMEMORY;
	return ZD_OK;
}


static ZD_texture *zd_AllocTexture(ZD_state *st)
{
	ZD_texture *tx = (ZD_texture *)calloc(1, st->texturesize);
	if(!tx)
	{
		st->lasterror = ZD_OOMEMORY;
		return NULL;
	}
	tx->state = st;
	return tx;
}

/* Create a new texture */
ZD_texture *zd_Texture(ZD_state *state, ZD_pixelformats format,
		ZD_texflags flags, unsigned w, unsigned h)
{
	ZD_backend *be = state->backend;
	ZD_errors res;
	ZD_texture *tx;
	if(flags & ZD_VIRTUAL)
	{
		zd_lasterror = ZD_NOTIMPLEMENTED;
		return NULL;
	}
	if(flags & ZD_ONDEMAND)
	{
		zd_lasterror = ZD_NOTSUPPORTED;
		return NULL;
	}
	switch(format)
	{
	  case ZD_OFF:
		break;
	  case ZD_I:
		zd_lasterror = ZD_NOTIMPLEMENTED;
		return NULL;
	  case ZD_RGB:
		break;
	  case ZD_RGBA:
		break;
	  default:
		zd_lasterror = ZD_BADFORMAT;
		return NULL;
	}

	if(!(tx = zd_AllocTexture(state)))
		return NULL;

	tx->state = state;
	tx->flags = flags;

	tx->format = format;
	tx->w = w;
	tx->h = h;

	tx->type = ZD_TT_PHYSICAL;
	if((res = zd_AllocTexturePixels(tx)))
	{
		free(tx);
		zd_lasterror = res;
		return NULL;
	}

	tx->next = state->textures;
	state->textures = tx;
	/*
	 * Since textures are typically allocated and initialized (which
	 * involes locking/unlocking, directly on indirectly) before assigned
	 * to any entities, we automatically give ownership to the application.
	 * Otherwise, applications would have to zd_RetainTexture() to prevent
	 * textures from being destroyed before assigned!
	 */
	tx->refcount = 1;

	if(be->InitTexture && (res = be->InitTexture(tx)))
	{
		zd_FreeTexturePixels(tx);
		free(tx);
		zd_lasterror = res;
		return NULL;
	}

	return tx;
}


ZD_texture *zd_OnDemandTexture(ZD_state *state, ZD_pixelformats format,
		ZD_texflags flags, unsigned w, unsigned h,
		ZD_texrendercb callback, void *userdata)
{
	ZD_texture *tx = zd_Texture(state, format, flags | ZD_NOCLEAR, w, h);
	if(!tx)
		return NULL;
	tx->flags |= ZD_ONDEMAND;
	tx->Render = callback;
	tx->userdata = userdata;
	return tx;
}


void zd_DestroyTexture(ZD_texture *tx)
{
	ZD_backend *be = tx->state->backend;
	if(be->CloseTexture)
		be->CloseTexture(tx);
	zd_FreeTexturePixels(tx);
	free(tx);
}


void zd_RetainTexture(ZD_texture *tx)
{
	zd_TextureIncRef(tx);
}


void zd_ReleaseTexture(ZD_texture *tx)
{
	zd_TextureDecRef(tx);
}


/*---------------------------------------------------------
	Texture pixel access
---------------------------------------------------------*/

ZD_texture *zd_TextureFromData(ZD_state *state, ZD_pixelformats format,
		ZD_texflags flags, unsigned w, unsigned h, void *pixels)
{
	ZD_errors res;
	ZD_pixels px;
	ZD_texture *tx = zd_Texture(state, format, flags | ZD_NOCLEAR, w, h);
	if(!tx)
		return NULL;
	if((res = zd_LockTexture(tx, &px)))
		return NULL;
/*TODO: Padding for row alignment! */
	memcpy(px.pixels, pixels, w * h * zd_PixelSize(format));
	zd_UnlockTexture(&px);
	return tx;
}


/*
 * NOTE:
 *	Currently, we keep textures buffered unless they're set up to be
 *	rendered on demand (callbacks), so we don't need to do download
 *	anything when locking.
 */
ZD_errors zd_LockTexture(ZD_texture *texture, ZD_pixels *pixels)
{
	zd_PixelsFromTexture(pixels, texture);
	if(!(texture->flags & ZD_NOCLEAR) && (texture->flags & ZD_UNDEFINED))
	{
		zd_default_texonrender(pixels, NULL);
		texture->flags &= ~ZD_UNDEFINED;
	}
	zd_TextureIncRef(texture);
	return ZD_OK;
}


ZD_errors zd_LockTextureRegion(ZD_texture *texture, ZD_pixels *pixels,
		unsigned x, unsigned y, unsigned w, unsigned h)
{
	if(x > texture->w || y > texture->h ||
			(x + w) > texture->w || (y + h) > texture->h)
		return ZD_CLIPPING;
	zd_PixelsFromTexture(pixels, texture);
	if(!(texture->flags & ZD_NOCLEAR) && (texture->flags & ZD_UNDEFINED))
	{
		zd_default_texonrender(pixels, NULL);
		texture->flags &= ~ZD_UNDEFINED;
	}
	pixels->pixels += x * zd_PixelSize(pixels->format) + y * pixels->pitch;
	pixels->x = x;
	pixels->y = y;
	pixels->w = w;
	pixels->h = h;
	return ZD_OK;
}


ZD_errors zd_UnlockTexture(ZD_pixels *pixels)
{
	ZD_errors res = ZD_OK;
	ZD_backend *be;
	ZD_texture *tx = pixels->texture;
	if(!tx)
		return ZD_UNLOCKED;
	be = tx->state->backend;
	if(be->UploadTexture)
		res = be->UploadTexture(pixels);
	zd_TextureDecRef(tx);
	pixels->texture = NULL;
	return res;
}


/*---------------------------------------------------------
-----------------------------------------------------------
	Entities
-----------------------------------------------------------
---------------------------------------------------------*/

ZD_entity *zd_Root(ZD_state *state)
{
	return state->root;
}

ZD_entity *zd_First(ZD_entity *parent)
{
	return parent->first;
}

ZD_entity *zd_Next(ZD_entity *entity)
{
	return entity->next;
}


ZD_entity *zd_alloc_entity(ZD_state *st)
{
	ZD_entity *e = (ZD_entity *)calloc(1, st->entitysize);
	if(!e)
	{
		st->lasterror = ZD_OOMEMORY;
		return NULL;
	}
	e->state = st;
	return e;
}


/*---------------------------------------------------------
	Layer entity
---------------------------------------------------------*/

ZD_entity *zd_Layer(ZD_entity *parent, ZD_entityflags flags,
		ZD_f left, ZD_f right, ZD_f bottom, ZD_f top)
{
	ZD_entity *e;
	ZD_layer *le;
	ZD_state *st = parent->state;
	if(parent->kind != ZD_EROOT)
	{
		/*
		 * This would be technically possible, but makes little sense,
		 * and could complicate backends for no good reason.
		 */
		st->lasterror = ZD_INVALIDPARENT;
		return NULL;
	}
	e = zd_NewEntity(parent, ZD_ELAYER);
	le = (ZD_layer *)e;
	if(!e)
		return NULL;
	e->flags = flags | ZD_RETHINK | ZD_VISIBLE;
	e->x = e->y = e->z = 0.0f;
	e->s = 1.0f;
	e->r = 0.0f;
	e->cr = e->cg = e->cb = e->ca = 1.0f;
	le->left = left;
	le->right = right;
	le->bottom = bottom;
	le->top = top;
	le->bgr = le->bgg = le->bgb = 0.0f;
	le->bga = 1.0f;
	if(st->backend->InitLayer)
		if((st->lasterror = st->backend->InitLayer(e)))
		{
			zd_FreeEntity(e);
			return NULL;
		}
	zd_LinkEntity(e);
	return e;
}


/*---------------------------------------------------------
	Window entity
---------------------------------------------------------*/

ZD_entity *zd_Window(ZD_entity *parent, ZD_entityflags flags,
		ZD_f x, ZD_f y, ZD_f w, ZD_f h)
{
	ZD_state *st = parent->state;
	ZD_entity *e = zd_NewEntity(parent, ZD_EWINDOW);
	ZD_window *we = (ZD_window *)e;
	if(!e)
		return NULL;
	e->flags = flags | ZD_RETHINK | ZD_VISIBLE;
	if(flags & ZD_SETORIGO)
	{
		e->x = x;
		e->y = y;
	}
	else
	{
		e->x = 0.0f;
		e->y = 0.0f;
	}
	e->z = 0.0f;
	we->w = w;
	we->h = h;
	we->l.left = x;
	we->l.right = x + w;
	we->l.bottom = y;
	we->l.top = y + h;
	we->l.bgr = we->l.bgg = we->l.bgb = 0.0f;
	we->l.bga = 1.0f;
	e->s = 1.0f;
	e->r = 0.0f;
	e->cr = e->cg = e->cb = e->ca = 1.0f;
	if(st->backend->InitWindow)
		if((st->lasterror = st->backend->InitWindow(e)))
		{
			zd_FreeEntity(e);
			return NULL;
		}
	zd_LinkEntity(e);
	return e;
}


/*---------------------------------------------------------
	Group entity
---------------------------------------------------------*/

ZD_entity *zd_Group(ZD_entity *parent, ZD_entityflags flags)
{
	ZD_state *st = parent->state;
	ZD_entity *e = zd_NewEntity(parent, ZD_EGROUP);
	if(!e)
		return NULL;
	e->flags = flags | ZD_RETHINK | ZD_VISIBLE;
	e->x = e->y = e->z = 0.0f;
	e->s = 1.0f;
	e->r = 0.0f;
	e->cr = e->cg = e->cb = e->ca = 1.0f;
	if(st->backend->InitGroup)
		if((st->lasterror = st->backend->InitGroup(e)))
		{
			zd_FreeEntity(e);
			return NULL;
		}
	zd_LinkEntity(e);
	return e;
}


/*---------------------------------------------------------
	Sprite entity
---------------------------------------------------------*/

ZD_entity *zd_Sprite(ZD_entity *parent, ZD_entityflags flags,
		ZD_texture *texture, ZD_f cx, ZD_f cy)
{
	ZD_state *st = parent->state;
	ZD_entity *e = zd_NewEntity(parent, ZD_ESPRITE);
	ZD_sprite *se = (ZD_sprite *)e;
	if(!e)
		return NULL;
	e->flags = flags | ZD_RETHINK | ZD_VISIBLE;
	se->txe.texture = texture;
	if(texture)
		zd_TextureIncRef(texture);
	se->cx = cx;
	se->cy = cy;
	e->x = e->y = e->z = 0.0f;
	e->s = 1.0f;
	e->r = 0.0f;
	e->cr = e->cg = e->cb = e->ca = 1.0f;
	if(st->backend->InitSprite)
		if((st->lasterror = st->backend->InitSprite(e)))
		{
			zd_FreeEntity(e);
			return NULL;
		}
	zd_LinkEntity(e);
	return e;
}


/*---------------------------------------------------------
	Fill entity
---------------------------------------------------------*/

ZD_entity *zd_Fill(ZD_entity *parent, ZD_entityflags flags, ZD_texture *texture)
{
	ZD_entity *le;
	ZD_state *st = parent->state;
	ZD_entity *e = zd_NewEntity(parent, ZD_EFILL);
	ZD_fill *fe = (ZD_fill *)e;
	if(!e)
		return NULL;
	e->flags = flags | ZD_RETHINK | ZD_VISIBLE;
	fe->txe.texture = texture;
	if(texture)
		zd_TextureIncRef(texture);
	e->x = e->y = e->z = 0.0f;
	e->s = 1.0f;
	e->r = 0.0f;
	e->cr = e->cg = e->cb = e->ca = 1.0f;
	fe->client = NULL;
	for(le = e->parent; le; le = le->parent)
	{
		switch(le->kind)
		{
		  case ZD_ELAYER:
		  case ZD_EWINDOW:
			fe->client = (ZD_layer *)le;
			break;
		  default:
			continue;
		}
		break;
	}
	if(!fe->client)
	{
		st->lasterror = ZD_INVALIDPARENT;
		zd_FreeEntity(e);
		return NULL;
	}
	if(st->backend->InitFill)
		if((st->lasterror = st->backend->InitFill(e)))
		{
			zd_FreeEntity(e);
			return NULL;
		}
	zd_LinkEntity(e);
	return e;
}


/*---------------------------------------------------------
	Entity control
---------------------------------------------------------*/

ZD_errors zd_SetTransform(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z, ZD_f s, ZD_f r)
{
	e->x = x;
	e->y = y;
	e->z = z;
	e->s = s;
	e->r = r;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_SetPosition(ZD_entity *e, ZD_f x, ZD_f y)
{
	e->x = x;
	e->y = y;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_SetPosition3D(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z)
{
	e->x = x;
	e->y = y;
	e->z = z;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_SetScale(ZD_entity *e, ZD_f scale)
{
	e->s = scale;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_SetRotation(ZD_entity *e, ZD_f rotation)
{
	e->r = rotation;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}


ZD_errors zd_Move(ZD_entity *e, ZD_f x, ZD_f y)
{
	e->x += x;
	e->y += y;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_Move3D(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z)
{
	e->x += x;
	e->y += y;
	e->z += z;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_Scale(ZD_entity *e, ZD_f scale)
{
	e->s *= scale;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_Rotate(ZD_entity *e, ZD_f rotation)
{
	e->r += rotation;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}


static inline void zd_update_animstate(ZD_entity *e)
{
	if(e->dx || e->dy || e->dz || e->ds || e->dr)
		e->flags |= ZD_ANIMATED;
	else
		e->flags &= ~ZD_ANIMATED;
}

ZD_errors zd_CMove(ZD_entity *e, ZD_f x, ZD_f y)
{
	e->dx = x;
	e->dy = y;
	zd_update_animstate(e);
	return ZD_OK;
}

ZD_errors zd_CMove3D(ZD_entity *e, ZD_f x, ZD_f y, ZD_f z)
{
	e->dx = x;
	e->dy = y;
	e->dz = z;
	zd_update_animstate(e);
	return ZD_OK;
}

ZD_errors zd_CScale(ZD_entity *e, ZD_f scale)
{
	e->ds = scale;
	zd_update_animstate(e);
	return ZD_OK;
}

ZD_errors zd_CRotate(ZD_entity *e, ZD_f rotation)
{
	e->dr = rotation;
	zd_update_animstate(e);
	return ZD_OK;
}


ZD_errors zd_CStop(ZD_entity *e)
{
	e->dx = 0.0f;
	e->dy = 0.0f;
	e->dz = 0.0f;
	e->ds = 0.0f;
	e->dr = 0.0f;
	e->flags &= ~ZD_ANIMATED;
	return ZD_OK;
}


ZD_errors zd_SetColor(ZD_entity *e, float r, float g, float b, float a)
{
	e->cr = r;
	e->cg = g;
	e->cb = b;
	e->ca = a;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}


ZD_errors zd_SetBGColor(ZD_entity *e, float r, float g, float b, float a)
{
	ZD_layer *l = (ZD_layer *)e;
	switch(e->kind)
	{
	  case ZD_ELAYER:
	  case ZD_EWINDOW:
		break;
	  default:
		return ZD_WRONGTYPE;
	}
	l->bgr = r;
	l->bgg = g;
	l->bgb = b;
	l->bga = a;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}


ZD_errors zd_SetTexture(ZD_entity *e, ZD_entityflags flags, ZD_texture *texture)
{
	switch(e->kind)
	{
	  case ZD_EROOT:
	  case ZD_ELAYER:
	  case ZD_EWINDOW:
	  case ZD_EGROUP:
		return ZD_NOTSUPPORTED;
	  case ZD_ESPRITE:
	  case ZD_EPRIMITIVE:
	  case ZD_EFILL:
	  {
		ZD_txentity *txe = (ZD_txentity *)e;
		if(txe->texture)
			zd_TextureDecRef(txe->texture);
		txe->texture = texture;
		if(txe->texture)
			zd_TextureIncRef(txe->texture);
		return ZD_OK;
	  }
	}
	return ZD_INTERNAL + 1;	/* Illegal kind! Bad pointer...? */
}


ZD_errors zd_SetView(ZD_entity *e, ZD_f left, ZD_f right, ZD_f bottom, ZD_f top)
{
	ZD_layer *l = (ZD_layer *)e;
	switch(e->kind)
	{
	  case ZD_ELAYER:
	  case ZD_EWINDOW:
		break;
	  default:
		return ZD_WRONGTYPE;
	}
	if((left == right) || (bottom == top))
		return ZD_BADARGUMENTS;
	l->left = left;
	l->right = right;
	l->bottom = bottom;
	l->top = top;
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

#if 0
ZD_errors zd_OnRender(ZD_entity *entity, ZD_entitycb callback, void *userdata)
{
	entity->OnRender = callback;
	entity->onrender_ud = userdata;
	return ZD_OK;
}

ZD_errors zd_OnTransformChange(ZD_entity *entity, ZD_entitycb callback, void *userdata)
{
	entity->OnTransformChange = callback;
	entity->ontransformchange_ud = userdata;
	return ZD_OK;
}
#endif


ZD_errors zd_SetParameter(ZD_entity *e, ZD_parameter param, ZD_f value)
{
	switch(param)
	{
	  case ZD_X:		e->x = value; break;
	  case ZD_Y:		e->y = value; break;
	  case ZD_Z:		e->z = value; break;
	  case ZD_SCALE:	e->s = value; break;
	  case ZD_ROTATION:	e->r = value; break;
	  case ZD_VX:		e->dx = value; break;
	  case ZD_VY:		e->dy = value; break;
	  case ZD_VZ:		e->dz = value; break;
	  case ZD_VSCALE:	e->ds = value; break;
	  case ZD_VROTATION:	e->dr = value; break;
	  case ZD_RED:		e->cr = value; break;
	  case ZD_GREEN:	e->cg = value; break;
	  case ZD_BLUE:		e->cb = value; break;
	  case ZD_ALPHA:	e->ca = value; break;
	  case ZD_MX0:		e->trmx[0] = value; break;
	  case ZD_MX1:		e->trmx[1] = value; break;
	  case ZD_MX2:		e->trmx[2] = value; break;
	  case ZD_MX3:		e->trmx[3] = value; break;
	  default:		break;
	}
	switch(e->kind)
	{
	  case ZD_EWINDOW:
	  {
		ZD_window *we = (ZD_window *)e;
		switch(param)
		{
		  case ZD_WIDTH:	we->w = value; break;
		  case ZD_HEIGHT:	we->h = value; break;
		  default:		break;
		}
		/* Fall through if no hit! */
	  }
	  case ZD_ELAYER:
	  {
		ZD_layer *le = (ZD_layer *)e;
		switch(param)
		{
		  case ZD_LEFT:		le->left = value; return ZD_OK;
		  case ZD_RIGHT:	le->right = value; return ZD_OK;
		  case ZD_BOTTOM:	le->bottom = value; return ZD_OK;
		  case ZD_TOP:		le->top = value; return ZD_OK;
		  case ZD_BGRED:	le->bgr = value; return ZD_OK;
		  case ZD_BGGREEN:	le->bgg = value; return ZD_OK;
		  case ZD_BGBLUE:	le->bgb = value; return ZD_OK;
		  case ZD_BGALPHA:	le->bga = value; return ZD_OK;
		  default:		return ZD_INVALIDPARAM;
		}
	  }
	  case ZD_ESPRITE:
	  {
		ZD_sprite *se = (ZD_sprite *)e;
		switch(param)
		{
		  case ZD_CX:		se->cx = value; return ZD_OK;
		  case ZD_CY:		se->cy = value; return ZD_OK;
		  default:		return ZD_INVALIDPARAM;
		}
	  }
	  default:
		return ZD_INVALIDPARAM;
	}
	e->flags |= ZD_RETHINK;
	return ZD_OK;
}

ZD_errors zd_GetParameter(ZD_entity *e, ZD_parameter param, ZD_f *value)
{
	switch(param)
	{
	  case ZD_X:		*value = e->x; return ZD_OK;
	  case ZD_Y:		*value = e->y; return ZD_OK;
	  case ZD_Z:		*value = e->z; return ZD_OK;
	  case ZD_SCALE:	*value = e->s; return ZD_OK;
	  case ZD_ROTATION:	*value = e->r; return ZD_OK;
	  case ZD_VX:		*value = e->dx; return ZD_OK;
	  case ZD_VY:		*value = e->dy; return ZD_OK;
	  case ZD_VZ:		*value = e->dz; return ZD_OK;
	  case ZD_VSCALE:	*value = e->ds; return ZD_OK;
	  case ZD_VROTATION:	*value = e->dr; return ZD_OK;
	  case ZD_RED:		*value = e->cr; return ZD_OK;
	  case ZD_GREEN:	*value = e->cg; return ZD_OK;
	  case ZD_BLUE:		*value = e->cb; return ZD_OK;
	  case ZD_ALPHA:	*value = e->ca; return ZD_OK;
	  case ZD_MX0:		*value = e->trmx[0]; return ZD_OK;
	  case ZD_MX1:		*value = e->trmx[1]; return ZD_OK;
	  case ZD_MX2:		*value = e->trmx[2]; return ZD_OK;
	  case ZD_MX3:		*value = e->trmx[3]; return ZD_OK;
	  default:		break;
	}
	switch(e->kind)
	{
	  case ZD_EWINDOW:
	  {
		ZD_window *we = (ZD_window *)e;
		switch(param)
		{
		  case ZD_WIDTH:	*value = we->w; return ZD_OK;
		  case ZD_HEIGHT:	*value = we->h; return ZD_OK;
		  default:		break;
		}
		/* Fall through if no hit! */
	  }
	  case ZD_ELAYER:
	  {
		ZD_layer *le = (ZD_layer *)e;
		switch(param)
		{
		  case ZD_LEFT:		*value = le->left; return ZD_OK;
		  case ZD_RIGHT:	*value = le->right; return ZD_OK;
		  case ZD_BOTTOM:	*value = le->bottom; return ZD_OK;
		  case ZD_TOP:		*value = le->top; return ZD_OK;
		  case ZD_BGRED:	*value = le->bgr; return ZD_OK;
		  case ZD_BGGREEN:	*value = le->bgg; return ZD_OK;
		  case ZD_BGBLUE:	*value = le->bgb; return ZD_OK;
		  case ZD_BGALPHA:	*value = le->bga; return ZD_OK;
		  default:		return ZD_INVALIDPARAM;
		}
	  }
	  case ZD_ESPRITE:
	  {
		ZD_sprite *se = (ZD_sprite *)e;
		switch(param)
		{
		  case ZD_CX:		*value = se->cx; return ZD_OK;
		  case ZD_CY:		*value = se->cy; return ZD_OK;
		  default:		return ZD_INVALIDPARAM;
		}
	  }
	  default:
		return ZD_INVALIDPARAM;
	}
}

/*TODO: Optimize these! */
ZD_errors zd_SetParameters(ZD_entity *e,
		unsigned count, unsigned *params, ZD_f *values)
{
	unsigned i;
	for(i = 0; i < count; ++i)
	{
		ZD_errors res = zd_SetParameter(e, params[i], values[i]);
		if(res)
			return res;
	}
	return ZD_OK;
}

ZD_errors zd_GetParameters(ZD_entity *e,
		unsigned count, unsigned *params, ZD_f *values)
{
	unsigned i;
	for(i = 0; i < count; ++i)
	{
		ZD_errors res = zd_GetParameter(e, params[i], values + i);
		if(res)
			return res;
	}
	return ZD_OK;
}


/*---------------------------------------------------------
	Entity management
---------------------------------------------------------*/

void zd_DestroyEntity(ZD_entity *entity)
{
	ZD_entity *e;
	ZD_entity *p = entity->parent;
	if(!p)
	{
		fprintf(stderr, "ZeeDraw: Tried to destroy the root entity!\n");
		return;
	}
	e = p->first;
	while(e)
	{
		if(e->next == entity)
		{
			if(p->last == entity)
				p->last = e;
			e->next = e->next->next;
			zd_destroy_entity(entity);
			return;	/* Done! --> */
		}
		e = e->next;
	}
	fprintf(stderr, "ZeeDraw: Entity not found under its parent!?\n");
}


void zd_RetainEntity(ZD_entity *e)
{
	zd_EntityIncRef(e);
}


void zd_ReleaseEntity(ZD_entity *e)
{
	zd_EntityDecRef(e);
}


/*---------------------------------------------------------
-----------------------------------------------------------
	Primitives
-----------------------------------------------------------
---------------------------------------------------------*/

ZD_entity *zd_Primitive(ZD_entity *parent, ZD_entityflags flags,
		ZD_primitives pkind, ZD_texture *texture,
		ZD_f x, ZD_f y, ZD_f size, ZD_f rotation)
{
	ZD_state *st = parent->state;
	ZD_entity *e = zd_NewEntity(parent, ZD_ESPRITE);
	ZD_primitive *pe = (ZD_primitive *)e;
	if(!e)
		return NULL;
	e->flags = flags | ZD_RETHINK | ZD_VISIBLE;
	pe->pkind = pkind;
	pe->txe.texture = texture;
	if(texture)
		zd_TextureIncRef(texture);
	pe->nvertices = 0;
	pe->vertices = NULL;
	e->x = x;
	e->y = y;
	e->z = 0.0f;
	e->s = size;
	e->r = rotation;
	e->cr = 1.0f;
	e->cg = 1.0f;
	e->cb = 1.0f;
	e->ca = 1.0f;
	if(st->backend->InitPrimitive)
		if((st->lasterror = st->backend->InitPrimitive(e)))
		{
			zd_FreeEntity(e);
			return NULL;
		}
	zd_LinkEntity(e);
	return e;
}


ZD_errors zd_Vertex2D(ZD_entity *entity, ZD_f x, ZD_f y);
ZD_errors zd_Vertex3D(ZD_entity *entity, ZD_f x, ZD_f y, ZD_f z);
ZD_errors zd_Vertices(ZD_entity *entity, unsigned dimensions, unsigned count,
		ZD_f *data);
ZD_errors zd_TexCoord(ZD_entity *entity, ZD_f x, ZD_f y);
ZD_errors zd_TexCoords(ZD_entity *entity, unsigned count, ZD_f *data);


/*---------------------------------------------------------
-----------------------------------------------------------
	Modulation and effects
-----------------------------------------------------------
---------------------------------------------------------*/

static void zd_advance_entity(ZD_entity *e, ZD_f dt)
{
	ZD_entity *ce;
	if(e->flags & ZD_ANIMATED)
	{
		e->x += e->dx * dt;
		e->y += e->dy * dt;
		e->z += e->dz * dt;
		e->s += e->ds * dt;
		e->r += e->dr * dt;
		e->flags |= ZD_RETHINK;
	}
	for(ce = e->first; ce; ce = ce->next)
		zd_advance_entity(ce, dt);
}

ZD_errors zd_Advance(ZD_state *state, ZD_f dt)
{
	state->now += dt;
	zd_advance_entity(state->root, dt);
	return ZD_OK;
}


/*---------------------------------------------------------
-----------------------------------------------------------
	Rendering
-----------------------------------------------------------
---------------------------------------------------------*/

static inline void zd_apply_transform(ZD_entity *e)
{
	if(e->parent)
	{
		ZD_entity *p = e->parent;
		zd_TransformPointE(p, e->x, e->y, &e->tx, &e->ty);
		e->tz = e->z + p->tz;
		e->ts = e->s * p->ts;
		e->tr = e->r + p->tr;
		e->tcr = p->tcr * e->cr;
		e->tcg = p->tcg * e->cg;
		e->tcb = p->tcb * e->cb;
		e->tca = p->tca * e->ca;
	}
	else
	{
		e->tx = e->x;
		e->ty = e->y;
		e->tz = e->z;
		e->ts = e->s;
		e->tr = e->r;
		e->tcr = e->cr;
		e->tcg = e->cg;
		e->tcb = e->cb;
		e->tca = e->ca;
	}
	zd_CalculateMatrix(e->tr, e->ts, e->trmx);
}

static ZD_errors zd_render_entity(ZD_entity *e, unsigned fwflags)
{
	ZD_entity *ce;
	e->flags |= fwflags;
	if(e->flags & ZD_RETHINK)
	{
		zd_apply_transform(e);
		if(e->Rethink)
			e->Rethink(e);
		e->flags &= ~ZD_RETHINK;
		fwflags |= ZD_RETHINK;	/* Recursively rethink children! */
	}
	if(e->flags & ZD_VISIBLE)
	{
		if(e->Render)
			e->Render(e);
		for(ce = e->first; ce; ce = ce->next)
		{
			ZD_errors res = zd_render_entity(ce, fwflags);
			if(res)
				return res;
		}
		if(e->RenderPost)
			e->RenderPost(e);
	}
	return ZD_OK;
}

ZD_errors zd_Render(ZD_state *state)
{
	ZD_backend *b = state->backend;
	ZD_errors res;
	if(b->PreRender)
		if((res = b->PreRender(state)))
			return res;
	if((res = zd_render_entity(state->root, 0)))
		return res;
	if(b->PostRender)
		if((res = b->PostRender(state)))
			return res;
	return ZD_OK;
}


/* Global error code */
ZD_errors zd_lasterror = ZD_OK;

ZD_errors zd_LastError(ZD_state *state)
{
	if(state)
		return state->lasterror;
	else
		return zd_lasterror;
}

#define	ZD_DEFERR(x, y)	y,
static const char *zd_errnames[100] = {
	"Ok - no error!",
	ZD_ALLERRORS
};
#undef	ZD_DEFERR

static char zd_errbuf[128];

const char *zd_ErrorString(ZD_errors errorcode)
{
	if(errorcode < ZD_INTERNAL)
		return zd_errnames[errorcode];
	else
	{
		zd_errbuf[sizeof(zd_errbuf) - 1] = 0;
		snprintf(zd_errbuf, sizeof(zd_errbuf) - 1,
				"INTERNAL ERROR #%d; please report to "
				"<david@olofson.net>", errorcode - ZD_INTERNAL);
		return zd_errbuf;
	}
}
