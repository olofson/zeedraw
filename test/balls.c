/*
 * balls.c - ZeeDraw test program
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
	ZD_texture *sprite;
	ZD_entity *l, *g, *e;

	int groups = 30;	/* Groups of sprites */
	int sprites = 9;	/* Number of sprites per group */
	ZDT_argspec as[] = {
		{ "-G", &groups, "Number of groups" },
		{ "-S", &sprites, "Sprites per group" },
		{ NULL, NULL, NULL }
	};

	/*
	 * Parse command line switches and open display
	 */
	if(!(tst = zdt_Open("Balls", argc, argv, as)))
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

	/*
	 * Construct scene
	 */

	/* Set up a layer with a 1.0 x 1.0 area scaled to fit the display */
	if(tst->width > tst->height)
	{
		double hw = (double)tst->width / tst->height * 0.5f;
		l = zd_Layer(zd_Root(state), ZD_CLEAR, -hw, hw, -0.5f, 0.5f);
	}
	else
	{
		double hh = (double)tst->height / tst->width * 0.5f;
		l = zd_Layer(zd_Root(state), ZD_CLEAR, -0.5f, 0.5f, -hh, hh);
	}
	if(!l)
		zdt_Fail(zd_LastError(state));

	/* Lots of grouped sprites */
	if(!(g = zd_Group(l, 0)))
		zdt_Fail(zd_LastError(state));
	zd_CRotate(g, -0.25f);
	for(j = 1; j < groups; ++j)
	{
		float s = (float)j / groups;
		float r = 0.1f + 0.4f * s;
		float a = 4.0f * M_PI * s;
		float c = 0.1f + 0.9f * s;
		ZD_entity *sg = zd_Group(g, 0);
		if(!sg)
			zdt_Fail(zd_LastError(state));
		zd_SetPosition(sg, r * cos(a), r * sin(a));
		zd_SetScale(sg, r);
		zd_SetColor(sg, c, c, c, 1.0f);
		zd_CRotate(sg, zdt_Rnd(-1.0f, 1.0f));

		for(i = 1; i < sprites; ++i)
		{
			float sr = 0.3f + 0.7f * i / sprites;
			float sa = 8.0f * M_PI * i / sprites;
			if(!(e = zd_Sprite(sg, 0, sprite, 0.5f, 0.5f)))
				zdt_Fail(zd_LastError(state));
			zd_SetTransform(e, sr * cos(sa), sr * sin(sa), 0.0f,
					0.1f + sr * 0.25f, M_PI);
			zd_CRotate(e, zdt_Rnd(-5.0f, 5.0f));
		}
	}

	/*
	 * We can release all textures at this point, to have them destroyed
	 * automatically when they're no longer used by any entities.
	 */
	zd_ReleaseTexture(sprite);

	/*
	 * GUI main loop
	 */
	while(zdt_HandleEvents(tst))
	{
		/* Advance state of animated entities */
		zd_Advance(state, zdt_Frame(tst));

		/* Render scene! */
		zd_Render(state);
		SDL_GL_SwapBuffers();
	}

	zd_Close(state);
	zdt_Close(tst);
	return 0;
}
