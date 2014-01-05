/*
 * ZeeDraw - Software rendering backend
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

#include "zd_software.h"

static ZD_errors zdsw_Open(ZD_state *st)
{
	return ZD_NOTIMPLEMENTED;
}

static void zdsw_Close(ZD_state *st)
{
}


/*
 * Top level scene rendering
 */

static ZD_errors zdsw_PreRender(ZD_state *st)
{
	return ZD_OK;
}

static ZD_errors zdsw_PostRender(ZD_state *st)
{
	return ZD_OK;
}


/*
 * Entity initializers
 */

static ZD_errors zdsw_InitLayer(ZD_entity *e)
{
	return ZD_OK;
}

static ZD_errors zdsw_InitWindow(ZD_entity *e)
{
	return ZD_OK;
}

static ZD_errors zdsw_InitGroup(ZD_entity *e)
{
	return ZD_OK;
}

static ZD_errors zdsw_InitSprite(ZD_entity *e)
{
	return ZD_OK;
}

static ZD_errors zdsw_InitPrimitive(ZD_entity *e)
{
	return ZD_OK;
}

static ZD_errors zdsw_InitFill(ZD_entity *e)
{
	return ZD_OK;
}


/*
 * Texture management
 */

static ZD_errors zdsw_InitTexture(ZD_texture *tx)
{
	return ZD_OK;
}


static ZD_errors zdsw_UploadTexture(ZD_pixels *px)
{
	return ZD_OK;
}


static ZD_errors zdsw_CloseTexture(ZD_texture *tx)
{
	return ZD_OK;
}


ZD_backend zd_software_backend = {
	zdsw_Open,
	zdsw_Close,

	zdsw_PreRender,
	zdsw_PostRender,

	zdsw_InitLayer,
	zdsw_InitWindow,
	zdsw_InitGroup,
	zdsw_InitSprite,
	zdsw_InitPrimitive,
	zdsw_InitFill,

	zdsw_InitTexture,
	zdsw_UploadTexture,
	zdsw_CloseTexture
};
