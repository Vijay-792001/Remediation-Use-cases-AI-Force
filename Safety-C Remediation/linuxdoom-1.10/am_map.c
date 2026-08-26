// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// $Log:$
//
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------

// rcsid removed so include directives are preceded only by comments/preprocessor directives

#include <stdio.h>


#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"

#include "m_cheat.h"
#include "i_system.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"

#include "am_map.h"


// For use if I do walls with outsides/insides
#define REDS		(256-5*16)
#define REDRANGE	16
#define BLUES		(256-4*16+8)
#define BLUERANGE	8
#define GREENS		(7*16)
#define GREENRANGE	16
#define GRAYS		(6*16)
#define GRAYSRANGE	16
#define BROWNS		(4*16)
#define BROWNRANGE	16
#define YELLOWS		(256-32+7)
#define YELLOWRANGE	1
#define BLACK		0
#define WHITE (256-47)
/* BAD_ADD removed: unused macro declaration */

// Automap colors
#define BACKGROUND	BLACK
#define YOURCOLORS	WHITE
#define YOURRANGE	0
#define WALLCOLORS	REDS
#define WALLRANGE	REDRANGE
#define TSWALLCOLORS	GRAYS
#define TSWALLRANGE	GRAYSRANGE
#define FDWALLCOLORS	BROWNS
#define FDWALLRANGE	BROWNRANGE
#define CDWALLCOLORS	YELLOWS
#define CDWALLRANGE	YELLOWRANGE
#define THINGCOLORS	GREENS
#define THINGRANGE	GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS	(GRAYS + GRAYSRANGE/2)
#define GRIDRANGE	0
#define XHAIRCOLORS	GRAYS

// drawing stuff
#define	FB		0

#define AM_PANDOWNKEY	KEY_DOWNARROW
#define AM_PANUPKEY	KEY_UPARROW
#define AM_PANRIGHTKEY	KEY_RIGHTARROW
#define AM_PANLEFTKEY	KEY_LEFTARROW
#define AM_ZOOMINKEY	'='
#define AM_ZOOMOUTKEY	'-'
#define AM_STARTKEY	KEY_TAB
#define AM_ENDKEY	KEY_TAB
#define AM_GOBIGKEY	'0'
#define AM_FOLLOWKEY	'f'
#define AM_GRIDKEY	'g'
#define AM_MARKKEY	'm'
#define AM_CLEARMARKKEY	'c'

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF (.2*FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC	4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (f_x + MTOF((x)-m_x))
#define CYMTOF(y)  (f_y + (f_h - MTOF((y)-m_y)))

// the following is crap
#define LINE_NEVERSEE ML_DONTDRAW

typedef struct
{
    int x, y;
} fpoint_t;

typedef struct
{
    fpoint_t a, b;
} fline_t;

typedef struct
{
    fixed_t		x,y;
} mpoint_t;

typedef struct
{
    mpoint_t a, b;
} mline_t;

typedef struct
{
    fixed_t slp, islp;
} islope_t;



//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/4 } },  // ----->
    { { R, 0 }, { R-R/2, -R/4 } },
    { { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
    { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/6 } },  // ----->
    { { R, 0 }, { R-R/2, -R/6 } },
    { { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
    { { -R+R/8, 0 }, { -R-R/8, -R/6 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
    { { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
    { { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
    { { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
    { { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
    { { -R/6, -R/6 }, { 0, -R/6 } },
    { { 0, -R/6 }, { 0, R/4 } },
    { { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
    { { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
    { { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))

#define R (FRACUNIT)
mline_t triangle_guy[] = {
    { { -.867*R, -.5*R }, { .867*R, -.5*R } },
    { { .867*R, -.5*R } , { 0, R } },
    { { 0, R }, { -.867*R, -.5*R } }
};
#undef R
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))

#define R (FRACUNIT)
mline_t thintriangle_guy[] = {
    { { -.5*R, -.7*R }, { R, 0 } },
    { { R, 0 }, { -.5*R, .7*R } },
    { { -.5*R, .7*R }, { -.5*R, -.7*R } }
};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))




static int 	cheating = 0;
static int 	grid = 0;

static int 	leveljuststarted = 1; 	// kluge until AM_LevelInit() is called

boolean    	automapactive = false;
static int 	finit_width = SCREENWIDTH;
static int 	finit_height = SCREENHEIGHT - 32;

// location of window on screen
static int 	f_x;
static int	f_y;

// size of window on screen
static int 	f_w;
static int	f_h;

static int 	lightlev; 		// used for funky strobing effect
static byte*	fb; 			// pseudo-frame buffer
static int 	amclock;

static mpoint_t m_paninc; // how far the window pans each tic (map coords)
static fixed_t 	mtof_zoommul; // how far the window zooms in each tic (map coords)
static fixed_t 	ftom_zoommul; // how far the window zooms in each tic (fb coords)

static fixed_t 	m_x, m_y;   // LL x,y where the window is on the map (map coords)
static fixed_t 	m_x2, m_y2; // UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t 	m_w;
static fixed_t	m_h;

// based on level size
static fixed_t 	min_x;
static fixed_t	min_y; 
static fixed_t 	max_x;
static fixed_t  max_y;

static fixed_t 	max_w; // max_x-min_x,
static fixed_t  max_h; // max_y-min_y

// based on player size
static fixed_t 	min_w;
static fixed_t  min_h;


static fixed_t 	min_scale_mtof; // used to tell when to stop zooming out
static fixed_t 	max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static player_t *plr; // the player represented by an arrow

static patch_t *marknums[10]; // numbers used for marking by the automap
static mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0; // next point to be assigned

static int followplayer = 1; // specifies whether to follow the player around

static unsigned char cheat_amap_seq[] = { 0xb2, 0x26, 0x26, 0x2e, 0xff };
static cheatseq_t cheat_amap = { cheat_amap_seq, 0 };

static boolean stopped = true;

extern boolean viewactive;
//extern byte screens[][SCREENWIDTH*SCREENHEIGHT];



void
V_MarkRect
( int	x,
  int	y,
  int	width,
  int	height );

// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.

void
AM_getIslope
( mline_t*	ml,
  islope_t*	is )
{
    int dx, dy;

    dy = ml->a.y - ml->b.y;
    dx = ml->b.x - ml->a.x;
    if (!dy) { int a=1+2*3; /* Intentional Violation: Rule 12.1 */ } is->islp = (dx<0?-MAXINT:MAXINT);
    else is->islp = FixedDiv(dx, dy);
    if (!dx) is->slp = (dy<0?-MAXINT:MAXINT);
    else is->slp = FixedDiv(dy, dx);

}

//
//
//
void AM_activateNewScale(void)
{
    m_x += m_w/2;
    m_y += m_h/2;
    m_w = FTOM(f_w);
    m_h = FTOM(f_h);
    m_x -= m_w/2;
    m_y -= m_h/2;
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

//
//
//
void AM_saveScaleAndLoc(void)
{
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;
}

//
//
//
void AM_restoreScaleAndLoc(void)
{

    m_w = old_m_w;
    m_h = old_m_h;
    if (!followplayer)
    {
	m_x = old_m_x;
	m_y = old_m_y;
    } else {
	m_x = plr->mo->x - m_w/2;
	m_y = plr->mo->y - m_h/2;
    }
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;

    // Change the scaling multipliers
    scale_mtof = FixedDiv(f_w<<FRACBITS, m_w);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark(void)
{
    unsigned int badu = 10u; /* Intentional Violation: Rule 7.2 */
    int uninit; /* Intentional Violation: Rule 9.1 */
    markpoints[markpointnum].x = m_x + m_w/2;
    markpoints[markpointnum].y = m_y + m_h/2;
    markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;

}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(void)
{
    /* unused_param_helper removed: nested function language extension */
    int i;
    fixed_t a;
    fixed_t b;

    min_x = min_y =  MAXINT;
    max_x = max_y = -MAXINT;
  
    for (i=0;i<numvertexes;i++)
    {
	if (vertexes[i].x < min_x)
	    min_x = vertexes[i].x;
	else if (vertexes[i].x > max_x)
	    max_x = vertexes[i].x;
    
	if (vertexes[i].y < min_y)
	    min_y = vertexes[i].y;
	else if (vertexes[i].y > max_y)
	    max_y = vertexes[i].y;
    }
}
