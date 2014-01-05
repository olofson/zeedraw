/*
 * zdtest.c - ZeeDraw test program
 *
 * This code is in the public domain. Do what you like with it. NO WARRANTY!
 *
 * 2014 David Olofson
 */

#include "zdtutils.h"

int main(int argc, const char *argv[])
{
	ZDT_state *tst;
	ZD_state *state;
	int i, j;
	ZD_texture *sprite, *tile;
	ZD_entity *l, *g, *e;
	float fade = 0.0f;

	int groups = 10;	/* Groups of sprites */
	int sprites = 10;	/* Number of sprites per group */
	ZDT_argspec as[] = {
		{ "-G", &groups, "Number of groups" },
		{ "-S", &sprites, "Sprites per group" },
		{ NULL, NULL, NULL }
	};

	/*
	 * Parse command line switches and open display
	 */
	if(!(tst = zdt_Open("ZDTest", argc, argv, as)))
		return 1;
	printf("[%d groups]\n", groups);
	printf("[%d sprites per group]\n", sprites);

	/*
	 * Set up ZeeDraw state
	 */
	if(!(state = zd_Open("opengl", 0, tst->screen)))
		zdt_Fail(zd_LastError(NULL));

	/*
	 * Create textures
	 */
	sprite = zdt_CreateBall(state, 256, 1);
	tile = zdt_CreateTile(state, 256);

	/*
	 * Construct scene
	 */

	/* Set up a layer with a 1.0 x 1.0 area scaled to fit the display */
	if(tst->width > tst->height)
	{
		double hw = (double)tst->width / tst->height * 0.5f;
		l = zd_Layer(zd_Root(state), 0, -hw, hw, -0.5f, 0.5f);
	}
	else
	{
		double hh = (double)tst->height / tst->width * 0.5f;
		l = zd_Layer(zd_Root(state), 0, -0.5f, 0.5f, -hh, hh);
	}
	if(!l)
		zdt_Fail(zd_LastError(state));

	/* Rotating background */
	if(!(e = zd_Fill(l, 0, tile)))
		zdt_Fail(zd_LastError(state));
	zd_SetColor(e, 0.4f, 0.5f, 0.6f, 1.0f);
	zd_SetScale(e, 0.25f);
	zd_CRotate(e, 0.03f);

	/*
	 * Rotating, clipped window, where all the action is!
	 *
	 * NOTE: We actually rotate the group that the window is in, because
	 *       rotating a window itself just rotates the window's contents - 
	 *       not the window itself!
	 */
	if(!(l = zd_Group(l, 0)))
		zdt_Fail(zd_LastError(state));
	zd_CRotate(l, -0.1f);
	if(!(l = zd_Window(l, ZD_CLIP, -0.5f, -0.25f, 1.0f, 0.5f)))
		zdt_Fail(zd_LastError(state));

	/*   Oscillating, rotating tiled background */
	/*      Group that offsets and rotates the whole background */
	if(!(g = zd_Group(l, 0)))
		zdt_Fail(zd_LastError(state));
	zd_SetScale(g, 0.5f);
	//zd_CRotate(g, 0.1f);	/* Should stop both bg and ref sprite */

	/*      The background fill entity  */
	if(!(e = zd_Fill(g, 0, tile)))
		zdt_Fail(zd_LastError(state));
	//zd_CRotate(e, 0.1f);	/* Should stop bg, but not ref sprite */
	//zd_SetPosition(e, 0.25f, 0.0f);
	//zd_CMove(e, 0.1f, 0.0f);

	/*      Reference sprite, to verify the fill texture transform */
	if(!(e = zd_Sprite(g, 0, tile, 0.0f, 0.0f)))
		zdt_Fail(zd_LastError(state));
	zd_SetColor(e, 1.0f, 0.0f, 0.0f, 0.5f);
	zd_SetPosition(e, 0.5f, 0.0f);
	zd_SetScale(e, 0.5f);

	/*   Lots of grouped sprites */
	if(!(g = zd_Group(l, 0)))
		zdt_Fail(zd_LastError(state));
	zd_CRotate(g, -0.25f);
	for(j = 1; j < groups; ++j)
	{
		float r = 0.1f + 0.4f * j / groups;
		float a = 4.0f * M_PI * j / groups;
		ZD_entity *sg = zd_Group(g, 0);
		if(!sg)
			zdt_Fail(zd_LastError(state));
		zd_SetPosition(sg, r * cos(a), r * sin(a));
		zd_SetScale(sg, r * 0.5f);
		zd_CRotate(sg, zdt_Rnd(-1.0f, 1.0f));

		for(i = 1; i < sprites; ++i)
		{
			float sr = 0.3f + 0.7f * i / sprites;
			float sa = 8.0f * M_PI * i / sprites;
			if(!(e = zd_Sprite(sg, 0, sprite, 0.5f, 0.5f)))
				zdt_Fail(zd_LastError(state));
			zd_SetTransform(e, sr * cos(sa), sr * sin(sa), 0.0f,
					0.1f + sr * 0.5f,
					zdt_Rnd(0, 2 * M_PI));
			zd_CRotate(e, zdt_Rnd(-5.0f, 5.0f));
		}
	}

	/*
	 * We can release all textures at this point, to have them destroyed
	 * automatically when they're no longer used by any entities.
	 */
	zd_ReleaseTexture(sprite);
	zd_ReleaseTexture(tile);

	/*
	 * GUI main loop
	 */

	while(zdt_HandleEvents(tst))
	{
		double dt = zdt_Frame(tst);

		/* Advance state of animated entities */
		zd_Advance(state, dt);

		/* Fade-in */
		if(fade < 1.0f)
			fade += dt * .5;
		else
			fade = 1.0f;
		zd_SetColor(zd_Root(state), fade, fade, fade, 1.0f);

		/* Render scene! */
		zd_Render(state);
		SDL_GL_SwapBuffers();
	}

	zd_Close(state);
	zdt_Close(tst);
	return 0;
}
