/*
 * ZeeDraw test suite utilities
 *
 * This code is in the public domain. Do what you like with it. NO WARRANTY!
 *
 * 2014 David Olofson
 */

#include "zdtutils.h"


static int do_interrupt = 0;


void zdt_Fail(ZD_errors err)
{
	fprintf(stderr, "ERROR, ZeeDraw result: %s\n", zd_ErrorString(err));
	exit(100);
}


ZD_texture *zdt_CreateBall(ZD_state *state, int size, int aa)
{
	int x, y;
	ZD_errors res;
	ZD_pixels px;
	ZD_texture *tx;
	unsigned char *pixels;
	if(!(tx = zd_Texture(state, ZD_RGBA, ZD_TRILINEAR_MIPMAP, size, size)))
		zdt_Fail(zd_LastError(state));
	if((res = zd_LockTexture(tx, &px)))
		zdt_Fail(res);
	pixels = (unsigned char *)px.pixels;
	for(y = 0; y < size; ++y)
		for(x = 0; x < size; ++x)
		{
			unsigned char *p = pixels + y * px.pitch + x * 4;
			float dx = x - size * 0.5f + 0.5f;
			float dy = y - size * 0.5f + 0.5f;
			float d = sqrt(dx*dx + dy*dy);
			int q = (x < size / 2) + (y < size / 2) * 2;
			int r, g, b, v;
			switch(q)
			{
			  case 0:
			  case 3:
				r = g = b = 255;
				break;
			  case 1:
				r = 255;
				g = b = 100;
				break;
			  case 2:
				b = 255;
				r = g = 100;
				break;
			  default:
				r = g = b = 0;	/* Warning eliminator */
				break;
			}
			v = (int)(255.0f * (1.0f - d / (size - 1) * 1.5f));
			p[0] = v * r >> 8;
			p[1] = v * g >> 8;
			p[2] = v * b >> 8;
			if(d > (size - 2) * 0.5)
			{
				p[3] = 0;
				continue;
			}
			if(aa)
			{
				v = (d - (size - 4) * 0.5f) * 255.0f;
				if(v > 0)
					p[3] = 255 - v;
				else
					p[3] = 255;
			}
			else
				p[3] = 255;
		}
	zd_UnlockTexture(&px);
	return tx;
}


ZD_texture *zdt_CreateTile(ZD_state *state, int size)
{
	int x, y;
	ZD_errors res;
	ZD_pixels px;
	ZD_texture *tx;
	unsigned char *pixels;
	if(!(tx = zd_Texture(state, ZD_RGB, ZD_TRILINEAR_MIPMAP, size, size)))
		zdt_Fail(zd_LastError(state));
	if((res = zd_LockTexture(tx, &px)))
		zdt_Fail(res);
	pixels = (unsigned char *)px.pixels;
	for(y = 0; y < size; ++y)
		for(x = 0; x < size; ++x)
		{
			unsigned char *p = pixels + y * px.pitch + x * 4;

			/* Mark the (0, 0) corner with a "shadow" */
			unsigned c = 128 + (x + y) * 2 * 255 / (size * 2);
			if(c > 255)
				c = 255;

			/* Checkerboard! */
			if((x >= size / 2) != (y >= size / 2))
			{
				p[0] = 48 * c >> 8;
				p[1] = 96 * c >> 8;
				p[2] = 48 * c >> 8;
			}
			else
			{
				p[0] = 96 * c >> 8;
				p[1] = 128 * c >> 8;
				p[2] = 96 * c >> 8;
			}
		}
	zd_UnlockTexture(&px);
	return tx;
}


int zdt_HandleEvents(ZDT_state *tst)
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
		  case SDL_QUIT:
			tst->do_exit = 1;
			break;
		  case SDL_MOUSEBUTTONDOWN:
			break;
		  case SDL_MOUSEBUTTONUP:
			break;
		  case SDL_KEYDOWN:
			switch(event.key.keysym.sym)
			{
			  case SDLK_ESCAPE:
				tst->do_exit = 1;
				break;
			  default:
				break;
			}
			break;
		  case SDL_KEYUP:
			switch(event.key.keysym.sym)
			{
			  default:
				break;
			}
			break;
		  default:
			break;
		}
	}
	if(do_interrupt)
		tst->do_exit = 1;
	return !tst->do_exit;
}


/* Print command line option usage info */
static void usage(const char *name, ZDT_argspec *xargs)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "usage: %s [switches]\n", name);
	fprintf(stderr, "  -f    Fullscreen\n");
	fprintf(stderr, "  -s    Disable swap control\n");
	fprintf(stderr, "  -w<n> Screen/window width [%d]\n",
			ZDT_DEFAULT_WIDTH);
	fprintf(stderr, "  -h<n> Screen/window height [%d]\n",
			ZDT_DEFAULT_HEIGHT);
	fprintf(stderr, "  -b<n> Bits per pixel [%d]\n",
			ZDT_DEFAULT_BPP);
	while(xargs->name)
	{
		fprintf(stderr, "  %s<n> %s [%d]\n",
				xargs->name, xargs->help, *xargs->value);
		++xargs;
	}
	fprintf(stderr, "  -?    This text.\n");
	fprintf(stderr, "  -h    This text.\n");
	fprintf(stderr, "\n");
}


/* Parse custom command line switches */
static int parse_arg(const char *argv, ZDT_argspec *xargs)
{
	while(xargs->name)
	{
		int len = strlen(xargs->name);
		if(strncmp(argv, xargs->name, len) == 0)
		{
			*(xargs->value) = atoi(&argv[len]);
			return 1;
		}
		++xargs;
	}
	return 0;
}


/* Parse ZDT_state built-in command line switches */
static int parse_args(ZDT_state *tst, int argc, const char *argv[],
		ZDT_argspec *xargs)
{
	int i;
	for(i = 1; i < argc; ++i)
	{
		if(strncmp(argv[i], "-f", 2) == 0)
		{
			tst->flags |= SDL_FULLSCREEN;
			printf("[Fullscreen mode]\n");
		}
		else if(strncmp(argv[i], "-w", 2) == 0)
		{
			tst->width = atoi(&argv[i][2]);
			printf("[Display width: %d]\n", tst->width);
		}
		else if(strncmp(argv[i], "-h", 2) == 0)
		{
			if(!argv[i][2])
			{
				usage(argv[0], xargs);
				return 0;
			}
			tst->height = atoi(&argv[i][2]);
			printf("[Display height: %d]\n", tst->height);
		}
		else if(strncmp(argv[i], "-b", 2) == 0)
		{
			tst->bpp = atoi(&argv[i][2]);
			printf("[Display bpp: %d]\n", tst->bpp);
		}
		else if(strncmp(argv[i], "-s", 2) == 0)
		{
			tst->swapcontrol = 0;
			printf("[Swap control off]\n");
		}
		else if(strncmp(argv[i], "-?", 2) == 0)
		{
			usage(argv[0], xargs);
			return 0;
		}
		else if(!parse_arg(argv[i], xargs))
		{
			fprintf(stderr, "Unknown switch '%s'!\n", argv[i]);
			usage(argv[0], xargs);
			return 0;
		}
	}
	return 1;
}


static void breakhandler(int a)
{
	do_interrupt = 1;
}


ZDT_state *zdt_Open(const char *title, int argc, const char *argv[],
		ZDT_argspec *xargs)
{
	char buf[256];
	ZDT_state *tst = (ZDT_state *)calloc(1, sizeof(ZDT_state));
	if(!tst)
		return NULL;

	/* Defaults */
	tst->width = ZDT_DEFAULT_WIDTH;
	tst->height = ZDT_DEFAULT_HEIGHT;
	tst->bpp = ZDT_DEFAULT_BPP;
	tst->flags = SDL_OPENGL;
	tst->swapcontrol = 1;

	/* Parse options */
	if(!parse_args(tst, argc, argv, xargs))
	{
		free(tst);
		return NULL;
	}

	/* Init SDL video subsystem */
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "ERROR: Could not init SDL video!\n");
		free(tst);
		return NULL;
	}

	/* Set up break/interrupt handling */
	atexit(SDL_Quit);
	signal(SIGTERM, breakhandler);
	signal(SIGINT, breakhandler);
	do_interrupt = 0;

	/* Open window */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, tst->swapcontrol);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	tst->screen = SDL_SetVideoMode(tst->width, tst->height,
			tst->bpp, tst->flags);
	snprintf(buf, sizeof(buf), "ZeeDraw Test Suite: %s", title);
	SDL_WM_SetCaption(buf, title);
	SDL_ShowCursor(1);

	return tst;
}


void zdt_Close(ZDT_state *tst)
{
	if(tst->starttick != tst->lasttick)
		printf("Average fps: %f\n", (tst->framecount - 1) * 1000.0f /
				(tst->lasttick - tst->starttick));
	SDL_Quit();
	free(tst);
}


double zdt_Frame(ZDT_state *tst)
{
	double dt;
	if(!tst->framecount)
	{
		tst->starttick = tst->lasttick = SDL_GetTicks();
		dt = 0.0f;
	}
	else
	{
		int tick = SDL_GetTicks();
		dt = (tick - tst->lasttick) / 1000.0f;
		tst->lasttick = tick;
	}
	++tst->framecount;
	return dt;
}
