/*
 * ZeeDraw - OpenGL backend
 *
 * Copyright 2013-2014 David Olofson
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

#include "zd_opengl.h"
#include "zd_gli.h"
#include "SDL.h"
#include <stdio.h>
#include <math.h>

/*
 * Define this to use OpenGL matrix transforms instead of custom code. Disabled
 * by default, as it tends to be significantly slower when dealing with the
 * small numbers of vertices per object we have here. This MIGHT be different
 * on platforms with slow CPUs and fast 3D accelerators, such as tablets and
 * cell phones, so it's left in as an easy-to-try potential optimization.
 */
#undef	ZDOGL_USE_OGL_MATRIX


#ifdef	ZDOGL_USE_OGL_MATRIX
/* Multiply the transform for entity 'e' into the current OpenGL matrix */
static inline void zdogl_apply_matrix(ZD_entity *e)
{
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	GLdouble m[16];
	m[0] = e->trmx[0]; m[4] = e->trmx[1]; m[8] = 0.0f;  m[12] = e->tx;
	m[1] = e->trmx[2]; m[5] = e->trmx[3]; m[9] = 0.0f;  m[13] = e->ty;
	m[2] = 0.0f;       m[6] = 0.0f;       m[10] = 1.0f; m[14] = 0.0f;
	m[3] = 0.0f;       m[7] = 0.0f;       m[11] = 0.0f; m[15] = 1.0f;
	gli->MultMatrixd(m);
}
#endif


typedef struct ZDOGL_texture {
	ZD_texture	tx;
	GLuint		name;
	ZD_f		x1, y1, x2, y2;
} ZDOGL_texture;


typedef struct ZDOGL_window {
	ZD_window	w;
	ZD_layer	*layer;	/* Layer we're on, if any, for clipping setup */
} ZDOGL_window;


static void zdogl_get_display_size(ZD_state *st, int *w, int *h)
{
	SDL_Surface *s = (SDL_Surface *)st->context;
	*w = s->w;
	*h = s->h;
}


static ZD_errors zdogl_Open(ZD_state *st)
{
	ZD_glinterface *gli;
	zd_BumpEntitySize(st, sizeof(ZDOGL_window));
	zd_BumpTextureSize(st, sizeof(ZDOGL_texture));
	st->bdata = gli = gli_Open(NULL);
	if(!gli)
		return ZD_DRIVEROPEN;
	gli->ShadeModel(GL_SMOOTH);
	gli_Disable(gli, GL_DEPTH_TEST);
	gli_Disable(gli, GL_CULL_FACE);
	gli->Hint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	gli->ClearStencil(0);
	return ZD_OK;
}

static void zdogl_Close(ZD_state *st)
{
	ZD_glinterface *gli = (ZD_glinterface *)st->bdata;
	gli_Close(gli);
}


/*
 * Top level scene rendering
 */

static ZD_errors zdogl_PreRender(ZD_state *st)
{
	ZD_glinterface *gli = (ZD_glinterface *)st->bdata;
	gli->MatrixMode(GL_PROJECTION);
	gli->PushMatrix();
	gli->LoadIdentity();
	gli->MatrixMode(GL_MODELVIEW);
	gli->PushMatrix();
	gli->LoadIdentity();
	gli_Enable(gli, GL_TEXTURE_2D);
	gli_Enable(gli, GL_BLEND);
	gli_BlendFunc(gli, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return ZD_OK;
}

static ZD_errors zdogl_PostRender(ZD_state *st)
{
	ZD_glinterface *gli = (ZD_glinterface *)st->bdata;
	gli->MatrixMode(GL_PROJECTION);
	gli->PopMatrix();
	gli->MatrixMode(GL_MODELVIEW);
	gli->PopMatrix();
	return ZD_OK;
}


/*
 * Layer
 */

static ZD_errors zdogl_render_layer(ZD_entity *e)
{
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	ZD_layer *le = (ZD_layer *)e;
	gli->PushMatrix();
	gli->Ortho(le->left, le->right, le->bottom, le->top, 0.0f, 10.0f);
	if(e->flags & ZD_CLEAR)
	{
		if(le->bga * e->tca >= 1.0f)
		{
			gli->ClearColor(le->bgr * e->tcr, le->bgg * e->tcg,
					le->bgb * e->tcb, 1.0f);
			gli->Clear(GL_COLOR_BUFFER_BIT);
		}
		else
		{
			gli_Disable(gli, GL_TEXTURE_2D);
			gli->Color4f(le->bgr * e->tcr, le->bgg * e->tcg,
					le->bgb * e->tcb, le->bga * e->tca);
			gli->Begin(GL_QUADS);
			gli->Vertex2d(le->left, le->bottom);
			gli->Vertex2d(le->right, le->bottom);
			gli->Vertex2d(le->right, le->top);
			gli->Vertex2d(le->left, le->top);
			gli->End();
		}
	}
	return ZD_OK;
}

static ZD_errors zdogl_render_post_layer(ZD_entity *e)
{
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	gli->PopMatrix();
	return ZD_OK;
}

static ZD_errors zdogl_InitLayer(ZD_entity *e)
{
	e->Render = zdogl_render_layer;
	e->RenderPost = zdogl_render_post_layer;
	return ZD_OK;
}


/*
 * Window
 */

static ZD_errors zdogl_render_window(ZD_entity *e)
{
	ZDOGL_window *we = (ZDOGL_window *)e;
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	ZD_layer *le = (ZD_layer *)e;
	ZD_f wx[4], wy[4];
	int use_stencil = 0;

	/* Transform the window position to viewport coordinates */
	zd_TransformPointE(e->parent, le->left, le->bottom, &wx[0], &wy[0]);
	zd_TransformPointE(e->parent, le->right, le->bottom, &wx[1], &wy[1]);
	zd_TransformPointE(e->parent, le->right, le->top, &wx[2], &wy[2]);
	zd_TransformPointE(e->parent, le->left, le->top, &wx[3], &wy[3]);

	/* Set up scissor and/or stencil */
	if(e->flags & ZD_CLIP)
	{
		int w, h, i;
		ZD_f sx, sy, ox, oy, xmin, xmax, ymin, ymax;
		xmin = xmax = wx[0];
		ymin = ymax = wy[0];
		for(i = 1; i < 4; ++i)
		{
			if(wx[i] < xmin)
				xmin = wx[i];
			if(wy[i] < ymin)
				ymin = wy[i];
			if(wx[i] > xmax)
				xmax = wx[i];
			if(wy[i] > ymax)
				ymax = wy[i];
		}

		/* OpenGL viewport to window coordinate transform */
		zdogl_get_display_size(e->state, &w, &h);
		if(we->layer)
		{
			ox = -we->layer->left;
			oy = -we->layer->bottom;
			sx = w / (we->layer->right - we->layer->left);
			sy = h / (we->layer->top - we->layer->bottom);
		}
		else
		{
			/* No layer! PreRender() setup applies. */
			ox = oy = 1.0f;
			sx = w * 0.5f;
			sy = h * 0.5f;
		}

		/* Transform and apply! */
		gli->Scissor((xmin + ox) * sx, (ymin + oy) * sy,
				ceil((xmax - xmin) * sx),
				ceil((ymax - ymin) * sy));
		gli_Enable(gli, GL_SCISSOR_TEST);

		if(wx[0] != wx[1])
		{
/*
TODO: Manage stencil bits/values centrally, and only repaint the stencil
TODO: buffer when windows have been moved!
*/
			/* Not axis aligned - prepare the stencil! */
			gli->Clear(GL_STENCIL_BUFFER_BIT);
			gli_Enable(gli, GL_STENCIL_TEST);
			gli->StencilFunc(GL_ALWAYS, 0x1, 0x1);
			gli->StencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			use_stencil = 1;
		}
	}

	/* Clear and/or fill background */
	if((e->flags & ZD_CLEAR) || use_stencil)
	{
		if(!(e->flags & ZD_CLEAR))
			gli->ColorMask(0, 0, 0, 0);
		gli_Disable(gli, GL_TEXTURE_2D);
		gli->Color4f(le->bgr * e->tcr, le->bgg * e->tcg,
				le->bgb * e->tcb, le->bga * e->tca);
		gli->Begin(GL_QUADS);
		gli->Vertex2d(wx[0], wy[0]);
		gli->Vertex2d(wx[1], wy[1]);
		gli->Vertex2d(wx[2], wy[2]);
		gli->Vertex2d(wx[3], wy[3]);
		gli->End();
		if(!(e->flags & ZD_CLEAR))
			gli->ColorMask(1, 1, 1, 1);
	}

	/* Set up stencil clipping for subsequent rendering */
	if(use_stencil)
	{
		gli->StencilFunc(GL_EQUAL, 0x1, 0x1);
		gli->StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	return ZD_OK;
}

static ZD_errors zdogl_render_post_window(ZD_entity *e)
{
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	if(e->flags & ZD_CLIP)
	{
		gli_Disable(gli, GL_SCISSOR_TEST);
		gli_Disable(gli, GL_STENCIL_TEST);
	}
	return ZD_OK;
}

static ZD_errors zdogl_InitWindow(ZD_entity *e)
{
	ZDOGL_window *we = (ZDOGL_window *)e;
	ZD_entity *le;
	e->Render = zdogl_render_window;
	e->RenderPost = zdogl_render_post_window;
	we->layer = NULL;
	for(le = e->parent; le; le = le->parent)
		if(le->kind == ZD_ELAYER)
		{
			we->layer = (ZD_layer *)le;
			break;
		}
	return ZD_OK;
}


/*
 * Group
 */

static ZD_errors zdogl_InitGroup(ZD_entity *e)
{
	e->Render = NULL;
	return ZD_OK;
}


/*
 * Sprite
 */

static ZD_errors zdogl_render_sprite(ZD_entity *e)
{
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	ZD_sprite *spr = (ZD_sprite *)e;
	ZDOGL_texture *xtx = (ZDOGL_texture *)spr->txe.texture;
	ZD_f sx1 = -spr->cx;
	ZD_f sy1 = -spr->cy;
	ZD_f sx2 = 1.0f - spr->cx;
	ZD_f sy2 = 1.0f - spr->cy;
#ifdef ZDOGL_USE_OGL_MATRIX
	gli->PushMatrix();
	zdogl_apply_matrix(e);
#else
	ZD_f x[4], y[4];
	zd_TransformPointE(e, sx1, sy1, &x[0], &y[0]);
	zd_TransformPointE(e, sx2, sy1, &x[1], &y[1]);
	zd_TransformPointE(e, sx2, sy2, &x[2], &y[2]);
	zd_TransformPointE(e, sx1, sy2, &x[3], &y[3]);
#endif
	if(xtx)
	{
		gli_Enable(gli, GL_TEXTURE_2D);
		gli_BindTexture(gli, GL_TEXTURE_2D, xtx->name);
	}
	else
		gli_Disable(gli, GL_TEXTURE_2D);
	gli->Color4f(e->tcr, e->tcg, e->tcb, e->tca);
	gli->Begin(GL_QUADS);
	if(xtx)
	{
#ifdef ZDOGL_USE_OGL_MATRIX
		gli->TexCoord2d(xtx->x1, xtx->y2);
		gli->Vertex3d(sx1, sy1, e->tz);
		gli->TexCoord2d(xtx->x2, xtx->y2);
		gli->Vertex3d(sx2, sy1, e->tz);
		gli->TexCoord2d(xtx->x2, xtx->y1);
		gli->Vertex3d(sx2, sy2, e->tz);
		gli->TexCoord2d(xtx->x1, xtx->y1);
		gli->Vertex3d(sx1, sy2, e->tz);
#else
		gli->TexCoord2d(xtx->x1, xtx->y2);
		gli->Vertex3d(x[0], y[0], e->tz);
		gli->TexCoord2d(xtx->x2, xtx->y2);
		gli->Vertex3d(x[1], y[1], e->tz);
		gli->TexCoord2d(xtx->x2, xtx->y1);
		gli->Vertex3d(x[2], y[2], e->tz);
		gli->TexCoord2d(xtx->x1, xtx->y1);
		gli->Vertex3d(x[3], y[3], e->tz);
#endif
	}
	else
	{
#ifdef ZDOGL_USE_OGL_MATRIX
		gli->Vertex3d(sx1, sy1, e->tz);
		gli->Vertex3d(sx2, sy1, e->tz);
		gli->Vertex3d(sx2, sy2, e->tz);
		gli->Vertex3d(sx1, sy2, e->tz);
#else
		gli->Vertex3d(x[0], y[0], e->tz);
		gli->Vertex3d(x[1], y[1], e->tz);
		gli->Vertex3d(x[2], y[2], e->tz);
		gli->Vertex3d(x[3], y[3], e->tz);
#endif
	}
	gli->End();
#ifdef ZDOGL_USE_OGL_MATRIX
	gli->PopMatrix();
#endif
	return ZD_OK;
}

static ZD_errors zdogl_InitSprite(ZD_entity *e)
{
	e->Render = zdogl_render_sprite;
	return ZD_OK;
}


/*
 * Primitive
 */

static ZD_errors zdogl_render_primitive(ZD_entity *e)
{
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	ZD_primitive *pe = (ZD_primitive *)e;
	ZDOGL_texture *xtx = (ZDOGL_texture *)pe->txe.texture;
	int i;
	GLenum mode;
	switch(pe->pkind)
	{
	  case ZD_POINTS:	mode = GL_POINTS;		break;
	  case ZD_LINES:	mode = GL_LINES;		break;
	  case ZD_LINESTRIP:	mode = GL_LINE_STRIP;		break;
	  case ZD_LINELOOP:	mode = GL_LINE_LOOP;		break;
	  case ZD_TRIANGLES:	mode = GL_TRIANGLES;		break;
	  case ZD_TRIANGLESTRIP:mode = GL_TRIANGLE_STRIP;	break;
	  case ZD_TRIANGLEFAN:	mode = GL_TRIANGLE_FAN;		break;
	  case ZD_QUADS:	mode = GL_QUADS;		break;
	}
	if(xtx)
	{
		gli_Enable(gli, GL_TEXTURE_2D);
		gli_BindTexture(gli, GL_TEXTURE_2D, xtx->name);
	}
	else
		gli_Disable(gli, GL_TEXTURE_2D);
	gli->Color4f(e->tcr, e->tcg, e->tcb, e->tca);
	gli->Begin(mode);
	if(xtx)
		for(i = 0; i < pe->nvertices; ++i)
		{
			ZD_f x, y;
			ZD_vertex *vx = pe->vertices + i;
			zd_TransformPointE(e, vx->x, vx->y, &x, &y);
			gli->TexCoord2d(vx->tx, vx->ty);
			gli->Vertex3d(x, y, vx->z + e->tz);
		}
	else
		for(i = 0; i < pe->nvertices; ++i)
		{
			ZD_f x, y;
			ZD_vertex *vx = pe->vertices + i;
			zd_TransformPointE(e, vx->x, vx->y, &x, &y);
			gli->Vertex3d(x, y, vx->z + e->tz);
		}
	gli->End();
	return ZD_OK;
}

static void zdogl_destroy_primitive(ZD_entity *e)
{
	ZD_primitive *pe = (ZD_primitive *)e;
	free(pe->vertices);
}

static ZD_errors zdogl_InitPrimitive(ZD_entity *e)
{
	ZD_primitive *pe = (ZD_primitive *)e;
	switch(pe->pkind)
	{
	  case ZD_POINTS:
	  case ZD_LINES:
	  case ZD_LINESTRIP:
	  case ZD_LINELOOP:
	  case ZD_TRIANGLES:
	  case ZD_TRIANGLESTRIP:
	  case ZD_TRIANGLEFAN:
	  case ZD_QUADS:
		break;
	  default:
		return ZD_BADPRIMITIVE;
	}
	e->Render = zdogl_render_primitive;
	e->Destroy = zdogl_destroy_primitive;
	return ZD_OK;
}


/*
 * Fill
 */

static ZD_errors zdogl_render_fill(ZD_entity *e)
{
	ZD_glinterface *gli = (ZD_glinterface *)e->state->bdata;
	ZD_fill *fe = (ZD_fill *)e;
	ZDOGL_texture *xtx = (ZDOGL_texture *)fe->txe.texture;
	ZD_layer *c = fe->client;
	ZD_f cr;
	ZD_f wx[4], wy[4];

	if(c->e.kind == ZD_EWINDOW)
	{
		/* Transform the window position to viewport coordinates */
		ZD_entity *wp = c->e.parent;
		zd_TransformPointE(wp, c->left, c->bottom, &wx[0], &wy[0]);
		zd_TransformPointE(wp, c->right, c->bottom, &wx[1], &wy[1]);
		zd_TransformPointE(wp, c->right, c->top, &wx[2], &wy[2]);
		zd_TransformPointE(wp, c->left, c->top, &wx[3], &wy[3]);
		cr = c->e.tr;
	}
	else /* if(c->e.kind == ZD_ELAYER) */
	{
		wx[0] = c->left;	wy[0] = c->bottom;
		wx[1] = c->right;	wy[1] = c->bottom;
		wx[2] = c->right;	wy[2] = c->top;
		wx[3] = c->left;	wy[3] = c->top;
		cr = 0.0f;
	}

	if(xtx)
	{
		gli_Enable(gli, GL_TEXTURE_2D);
		gli_BindTexture(gli, GL_TEXTURE_2D, xtx->name);
	}
	else
		gli_Disable(gli, GL_TEXTURE_2D);
	gli->Color4f(e->tcr, e->tcg, e->tcb, e->tca);
	if(xtx)
	{
		ZD_errors res;
		ZD_f xs, ys, tx1, ty1, tx2, ty2;
		ZD_f tx[4], ty[4];
		ZD_f m[4], xo, yo;
		ZD_f ctz = e->tz - c->e.tz;

		/* Map texture coordinates to match the client rectangle */
		xs = c->right - c->left;
		ys = c->top - c->bottom;
		tx1 = xtx->x1 * xs + c->left;
		ty1 = xtx->y1 * ys + c->bottom;
		tx2 = xtx->x2 * xs + c->left;
		ty2 = xtx->y2 * ys + c->bottom;

		/* Set up texcoord transform, adjusting for window rotation */
		zd_CalculateMatrix(cr - e->tr, e->ts, m);
		/* FIXME: This isn't doing what it's supposed to... */
		zd_TransformPoint(m, 0.0f, 0.0f, e->tx, e->ty, &xo, &yo);

		/* Invert matrix, because we're dealing with texcoords! */
		if((res = zd_InverseMatrix(m, m)))
			return res;

		/* Now we can transform the texture coordinates! */
		zd_InvTransformPoint(m, xo, yo, tx1, ty2, &tx[0], &ty[0]);
		zd_InvTransformPoint(m, xo, yo, tx2, ty2, &tx[1], &ty[1]);
		zd_InvTransformPoint(m, xo, yo, tx2, ty1, &tx[2], &ty[2]);
		zd_InvTransformPoint(m, xo, yo, tx1, ty1, &tx[3], &ty[3]);

		gli->Begin(GL_QUADS);
		gli->TexCoord2d(tx[0], ty[0]);
		gli->Vertex3d(wx[0], wy[0], ctz);
		gli->TexCoord2d(tx[1], ty[1]);
		gli->Vertex3d(wx[1], wy[1], ctz);
		gli->TexCoord2d(tx[2], ty[2]);
		gli->Vertex3d(wx[2], wy[2], ctz);
		gli->TexCoord2d(tx[3], ty[3]);
		gli->Vertex3d(wx[3], wy[3], ctz);
		gli->End();
	}
	else
	{
		gli->Begin(GL_QUADS);
		gli->Vertex3d(wx[0], wy[0], e->tz);
		gli->Vertex3d(wx[1], wy[1], e->tz);
		gli->Vertex3d(wx[2], wy[2], e->tz);
		gli->Vertex3d(wx[3], wy[3], e->tz);
		gli->End();
	}
	return ZD_OK;
}

static ZD_errors zdogl_InitFill(ZD_entity *e)
{
	e->Render = zdogl_render_fill;
	return ZD_OK;
}


/*
 * Texture management
 */

static ZD_errors zdogl_InitTexture(ZD_texture *tx)
{
	ZD_glinterface *gli = (ZD_glinterface *)tx->state->bdata;
	ZDOGL_texture *xtx = (ZDOGL_texture *)tx;
	gli->GenTextures(1, &xtx->name);
	xtx->x1 = xtx->y1 = 0.0f;
	xtx->x2 = xtx->y2 = 1.0f;
	return ZD_OK;
}


static ZD_errors zdogl_UploadTexture(ZD_pixels *px)
{
	ZD_texture *tx = px->texture;
	ZD_glinterface *gli = (ZD_glinterface *)tx->state->bdata;
	ZDOGL_texture *xtx = (ZDOGL_texture *)tx;
	GLint iformat;
	GLenum format;

	/* Setup... */
	gli_BindTexture(gli, GL_TEXTURE_2D, xtx->name);
	gli->PixelStorei(GL_UNPACK_ROW_LENGTH,
			px->pitch / zd_PixelSize(px->format));

	/* Magnification filtering */
	switch(tx->flags & ZD__SMODE)
	{
	  case ZD_NEAREST:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				GL_NEAREST);
		break;
	  case ZD_BILINEAR:
	  case ZD_BILINEAR_MIPMAP:
	  case ZD_TRILINEAR_MIPMAP:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				GL_LINEAR);
		break;
	}

	/* Minification filtering */
	switch(tx->flags & ZD__SMODE)
	{
	  case ZD_NEAREST:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_NEAREST);
		break;
	  case ZD_BILINEAR:
	  default:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR);
		break;
	  case ZD_BILINEAR_MIPMAP:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_NEAREST);
		break;
	  case ZD_TRILINEAR_MIPMAP:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR_MIPMAP_LINEAR);
		break;
	}

	/* If mipmaps requested, try to handle it one way or another. */
	switch(tx->flags & ZD__SMODE)
	{
	  case ZD_BILINEAR_MIPMAP:
	  case ZD_TRILINEAR_MIPMAP:
		gli->Hint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
		if(gli->_GenerateMipmap)
		{
			/* The nice, efficient OpenGL 3.0 way */
			gli_Enable(gli, GL_TEXTURE_2D);	/* For the ATI bug! */
		}
		else if((gli->version >= 14) && (gli->version < 31))
		{
			/* The OpenGL 1.4 way */
			gli->TexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP,
					GL_TRUE);
		}
		else
		{
/*TODO: Custom mipmap generator, to cover any OpenGL version! */
			/* Give up! No mipmapping... */
			gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
					GL_LINEAR);
		}
		break;
	}

	/* Clamping */
	switch(tx->flags & ZD__HMODE)
	{
	  case ZD_HCLAMP:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
				GL_CLAMP_TO_EDGE);
		break;
	  case ZD_HWRAP:
	  default:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		break;
	}
	switch(tx->flags & ZD__VMODE)
	{
	  case ZD_VCLAMP:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				GL_CLAMP_TO_EDGE);
		break;
	  case ZD_VWRAP:
	  default:
		gli->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	}

	/* Upload! */
	switch(px->format)
	{
	  case ZD_OFF:
		return ZD_INTERNAL + 1010;
	  case ZD_I:
		return ZD_NOTIMPLEMENTED;
	  case ZD_RGB:
		iformat = GL_RGB8;
		format = GL_RGBA;
		break;
	  case ZD_RGBA:
		iformat = GL_RGBA8;
		format = GL_RGBA;
		break;
	}
	gli->TexImage2D(GL_TEXTURE_2D, 0, iformat, px->w, px->h, 0,
			format, GL_UNSIGNED_BYTE, (char *)px->pixels);

	switch(tx->flags & ZD__SMODE)
	{
	  case ZD_BILINEAR_MIPMAP:
	  case ZD_TRILINEAR_MIPMAP:
		if(gli->_GenerateMipmap)
			gli->_GenerateMipmap(GL_TEXTURE_2D);
		break;
	}

	return ZD_OK;
}


static ZD_errors zdogl_CloseTexture(ZD_texture *tx)
{
	ZD_glinterface *gli = (ZD_glinterface *)tx->state->bdata;
	ZDOGL_texture *xtx = (ZDOGL_texture *)tx;
	gli->DeleteTextures(1, &xtx->name);
	return ZD_OK;
}


ZD_backend zd_opengl_backend = {
	zdogl_Open,
	zdogl_Close,

	zdogl_PreRender,
	zdogl_PostRender,

	zdogl_InitLayer,
	zdogl_InitWindow,
	zdogl_InitGroup,
	zdogl_InitSprite,
	zdogl_InitPrimitive,
	zdogl_InitFill,

	zdogl_InitTexture,
	zdogl_UploadTexture,
	zdogl_CloseTexture
};
