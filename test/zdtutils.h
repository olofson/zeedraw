/*
 * ZeeDraw test suite utilities
 *
 * This code is in the public domain. Do what you like with it. NO WARRANTY!
 *
 * 2013-2014 David Olofson
 */

#ifndef	ZDTUTILS_H
#define	ZDTUTILS_H

#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include "zeedraw.h"
#include "SDL.h"

#define	ZDT_DEFAULT_WIDTH	800
#define	ZDT_DEFAULT_HEIGHT	480
#define	ZDT_DEFAULT_BPP		0

typedef struct ZDT_state
{
	SDL_Surface	*screen;
	int		width;
	int		height;
	int		bpp;
	int		flags;
	int		swapcontrol;
	unsigned	framecount;
	unsigned	starttick;
	unsigned	lasttick;
	int		do_exit;
} ZDT_state;

typedef struct ZDT_argspec
{
	const char	*name;
	int		*value;
	const char	*help;
} ZDT_argspec;

/* Parse command line arguments and open */
ZDT_state *zdt_Open(const char *title, int argc, const char *argv[],
		ZDT_argspec *xargs);

/* Close application */
void zdt_Close(ZDT_state *tst);

/* Abort application and print ZeeDraw error status 'err' as text */
void zdt_Fail(ZD_errors err);

/* Handle SDL events. Returns 1 if the application is should keep running. */
int zdt_HandleEvents(ZDT_state *tst);

/* Update frame rate counter and return frame delta time */
double zdt_Frame(ZDT_state *tst);

/* Create a shaded ball sprite texture */
ZD_texture *zdt_CreateBall(ZD_state *state, int size, int aa);

/* Create a checkerboard tile texture */
ZD_texture *zdt_CreateTile(ZD_state *state, int size);

static inline double zdt_Rnd(double min, double max)
{
	return min + rand() * (max - min) / RAND_MAX;
}

#endif /* ZDTUTILS_H */
