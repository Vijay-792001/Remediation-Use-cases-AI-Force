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
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------

#include <stddef.h>
#include <string.h>

#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"
#include "m_cheat.h"
#include "i_system.h"
#include "v_video.h"
#include "doomstat.h"
#include "r_state.h"
#include "dstrings.h"
#include "am_map.h"

#define REDS        (256-(5*16))
#define REDRANGE   16
#define GREENS     (7*16)
#define GREENRANGE 16
#define GRAYS      (6*16)
#define GRAYSRANGE 16
#define BROWNS     (4*16)
#define BROWNRANGE 16
#define YELLOWS    (256-32+7)
#define YELLOWRANGE 1
#define BLACK      0
#define WHITE      (256-47)
#define BACKGROUND BLACK
#define WALLCOLORS REDS
#define WALLRANGE  REDRANGE
#define TSWALLCOLORS GRAYS
#define TSWALLRANGE  GRAYSRANGE
#define FDWALLCOLORS BROWNS
#define FDWALLRANGE  BROWNRANGE
#define CDWALLCOLORS YELLOWS
#define CDWALLRANGE  YELLOWRANGE
#define THINGCOLORS  GREENS
#define THINGRANGE   GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE  WALLRANGE
#define GRIDCOLORS (GRAYS + (GRAYSRANGE/2))
#define XHAIRCOLORS GRAYS
#define FB 0
#define AM_PANDOWNKEY KEY_DOWNARROW
#define AM_PANUPKEY KEY_UPARROW
#define AM_PANRIGHTKEY KEY_RIGHTARROW
#define AM_PANLEFTKEY KEY_LEFTARROW
#define AM_ZOOMINKEY '='
#define AM_ZOOMOUTKEY '-'
#define AM_STARTKEY KEY_TAB
#define AM_ENDKEY KEY_TAB
#define AM_GOBIGKEY '0'
#define AM_FOLLOWKEY 'f'
#define AM_GRIDKEY 'g'
#define AM_MARKKEY 'm'
#define AM_CLEARMARKKEY 'c'
#define AM_NUMMARKPOINTS 10
#define INITSCALEMTOF ((fixed_t)((FRACUNIT*2)/10))
#define F_PANINC 4
#define M_ZOOMIN ((int)((FRACUNIT*102)/100))
#define M_ZOOMOUT ((int)((FRACUNIT*100)/102))
#define FTOM(x) FixedMul(((x)<<16), scale_ftom)
#define MTOF(x) (FixedMul((x), scale_mtof)>>16)
#define CXMTOF(x) (f_x + MTOF((x)-m_x))
#define CYMTOF(y) (f_y + (f_h - MTOF((y)-m_y)))
#define LINE_NEVERSEE ML_DONTDRAW
#define AM_LEFT 1
#define AM_RIGHT 2
#define AM_BOTTOM 4
#define AM_TOP 8

typedef struct { int x, y; } fpoint_t;
typedef struct { fpoint_t a, b; } fline_t;
typedef struct { fixed_t x, y; } mpoint_t;
typedef struct { mpoint_t a, b; } mline_t;
typedef struct { fixed_t slp, islp; } islope_t;

#define R ((8*PLAYERRADIUS)/7)
static mline_t player_arrow[] = {
    { { -R+(R/8), 0 }, { R, 0 } },
    { { R, 0 }, { R-(R/2), R/4 } },
    { { R, 0 }, { R-(R/2), -(R/4) } },
    { { -R+(R/8), 0 }, { -R-(R/8), R/4 } },
    { { -R+(R/8), 0 }, { -R-(R/8), -(R/4) } },
    { { -R+((3*R)/8), 0 }, { -R+(R/8), R/4 } },
    { { -R+((3*R)/8), 0 }, { -R+(R/8), -(R/4) } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

#define R ((8*PLAYERRADIUS)/7)
static mline_t cheat_player_arrow[] = {
    { { -R+(R/8), 0 }, { R, 0 } }, { { R, 0 }, { R-(R/2), R/6 } }, { { R, 0 }, { R-(R/2), -(R/6) } },
    { { -R+(R/8), 0 }, { -R-(R/8), R/6 } }, { { -R+(R/8), 0 }, { -R-(R/8), -(R/6) } },
    { { -R+((3*R)/8), 0 }, { -R+(R/8), R/6 } }, { { -R+((3*R)/8), 0 }, { -R+(R/8), -(R/6) } },
    { { -(R/2), 0 }, { -(R/2), -(R/6) } }, { { -(R/2), -(R/6) }, { -(R/2)+(R/6), -(R/6) } },
    { { -(R/2)+(R/6), -(R/6) }, { -(R/2)+(R/6), R/4 } }, { { -(R/6), 0 }, { -(R/6), -(R/6) } },
    { { -(R/6), -(R/6) }, { 0, -(R/6) } }, { { 0, -(R/6) }, { 0, R/4 } },
    { { R/6, R/4 }, { R/6, -(R/7) } }, { { R/6, -(R/7) }, { R/6+(R/32), -(R/7)-(R/32) } },
    { { R/6+(R/32), -(R/7)-(R/32) }, { R/6+(R/10), -(R/7) } }
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))

#define R (FRACUNIT)
static mline_t thintriangle_guy[] = {
    { { -(R/2), -((7*R)/10) }, { R, 0 } },
    { { R, 0 }, { -(R/2), ((7*R)/10) } },
    { { -(R/2), ((7*R)/10) }, { -(R/2), -((7*R)/10) } }
};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))

static int cheating = 0;
static int grid = 0;
static int leveljuststarted = 1;
boolean automapactive = false;
static int finit_width = SCREENWIDTH;
static int finit_height = SCREENHEIGHT - 32;
static int f_x;
static int f_y;
static int f_w;
static int f_h;
static int lightlev;
static byte* fb;
static int amclock;
static mpoint_t m_paninc;
static fixed_t mtof_zoommul;
static fixed_t ftom_zoommul;
static fixed_t m_x, m_y;
static fixed_t m_x2, m_y2;
static fixed_t m_w;
static fixed_t m_h;
static fixed_t min_x;
static fixed_t min_y;
static fixed_t max_x;
static fixed_t max_y;
static fixed_t max_w;
static fixed_t max_h;
static fixed_t min_w;
static fixed_t min_h;
static fixed_t min_scale_mtof;
static fixed_t max_scale_mtof;
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;
static mpoint_t f_oldloc;
static fixed_t scale_mtof = INITSCALEMTOF;
static fixed_t scale_ftom;
static player_t *plr;
static patch_t *marknums[AM_NUMMARKPOINTS];
static mpoint_t markpoints[AM_NUMMARKPOINTS];
static int markpointnum = 0;
static int followplayer = 1;
static unsigned char cheat_amap_seq[] = { 0xb2, 0x26, 0x26, 0x2e, 0xff };
static cheatseq_t cheat_amap = { cheat_amap_seq, 0 };
static boolean stopped = true;
extern boolean viewactive;

void V_MarkRect(int x, int y, int width, int height);

static void AM_formatMarkMessage(char *buffer, int buffer_size, int point_number)
{
    int i;
    const char *prefix;

    if ((buffer == NULL) || (buffer_size <= 0)) { return; }
    prefix = AMSTR_MARKEDSPOT;
    i = 0;
    while ((prefix[i] != '\0') && (i < (buffer_size - 3))) { buffer[i] = prefix[i]; i++; }
    if (i < (buffer_size - 1)) { buffer[i] = ' '; i++; }
    if ((point_number >= 0) && (point_number <= 9) && (i < (buffer_size - 1))) { buffer[i] = (char)('0' + point_number); i++; }
    buffer[i] = '\0';
}

void AM_getIslope(mline_t* ml, islope_t* is)
{
    int dx, dy;
    if ((ml == NULL) || (is == NULL)) { return; }
    dy = ml->a.y - ml->b.y;
    dx = ml->b.x - ml->a.x;
    if (dy == 0) { is->islp = (dx < 0) ? -MAXINT : MAXINT; }
    else { is->islp = FixedDiv(dx, dy); }
    if (dx == 0) { is->slp = (dy < 0) ? -MAXINT : MAXINT; }
    else { is->slp = FixedDiv(dy, dx); }
}

void AM_activateNewScale(void)
{
    m_x += m_w/2; m_y += m_h/2; m_w = FTOM(f_w); m_h = FTOM(f_h); m_x -= m_w/2; m_y -= m_h/2; m_x2 = m_x + m_w; m_y2 = m_y + m_h;
}

void AM_saveScaleAndLoc(void)
{
    old_m_x = m_x; old_m_y = m_y; old_m_w = m_w; old_m_h = m_h;
}

void AM_restoreScaleAndLoc(void)
{
    m_w = old_m_w; m_h = old_m_h;
    if (followplayer == 0) { m_x = old_m_x; m_y = old_m_y; }
    else if ((plr != NULL) && (plr->mo != NULL)) { m_x = plr->mo->x - (m_w/2); m_y = plr->mo->y - (m_h/2); }
    else { m_x = old_m_x; m_y = old_m_y; }
    m_x2 = m_x + m_w; m_y2 = m_y + m_h;
    if (m_w != 0) { scale_mtof = FixedDiv(f_w<<FRACBITS, m_w); scale_ftom = FixedDiv(FRACUNIT, scale_mtof); }
}

void AM_addMark(void)
{
    markpoints[markpointnum].x = m_x + (m_w/2); markpoints[markpointnum].y = m_y + (m_h/2); markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;
}

void AM_findMinMaxBoundaries(void)
{
    int i; fixed_t a; fixed_t b;
    min_x = min_y = MAXINT; max_x = max_y = -MAXINT;
    for (i = 0; i < numvertexes; i++) {
        if (vertexes[i].x < min_x) { min_x = vertexes[i].x; } else if (vertexes[i].x > max_x) { max_x = vertexes[i].x; }
        if (vertexes[i].y < min_y) { min_y = vertexes[i].y; } else if (vertexes[i].y > max_y) { max_y = vertexes[i].y; }
    }
    max_w = max_x - min_x; max_h = max_y - min_y;
    if (max_w == 0) { max_w = 1; }
    if (max_h == 0) { max_h = 1; }
    min_w = 2*PLAYERRADIUS; min_h = 2*PLAYERRADIUS;
    a = FixedDiv(f_w<<FRACBITS, max_w); b = FixedDiv(f_h<<FRACBITS, max_h);
    min_scale_mtof = (a < b) ? a : b; max_scale_mtof = FixedDiv(f_h<<FRACBITS, 2*PLAYERRADIUS);
}

void AM_changeWindowLoc(void)
{
    if ((m_paninc.x != 0) || (m_paninc.y != 0)) { followplayer = 0; f_oldloc.x = MAXINT; }
    m_x += m_paninc.x; m_y += m_paninc.y;
    if (m_x + (m_w/2) > max_x) { m_x = max_x - (m_w/2); } else if (m_x + (m_w/2) < min_x) { m_x = min_x - (m_w/2); }
    if (m_y + (m_h/2) > max_y) { m_y = max_y - (m_h/2); } else if (m_y + (m_h/2) < min_y) { m_y = min_y - (m_h/2); }
    m_x2 = m_x + m_w; m_y2 = m_y + m_h;
}

void AM_initVariables(void)
{
    int pnum; static event_t st_notify = { ev_keyup, AM_MSGENTERED, 0, 0 };
    automapactive = true; fb = screens[0]; f_oldloc.x = MAXINT; amclock = 0; lightlev = 0; m_paninc.x = 0; m_paninc.y = 0; ftom_zoommul = FRACUNIT; mtof_zoommul = FRACUNIT; m_w = FTOM(f_w); m_h = FTOM(f_h);
    pnum = consoleplayer;
    if ((pnum < 0) || (pnum >= MAXPLAYERS) || (playeringame[pnum] == false)) {
        for (pnum = 0; pnum < MAXPLAYERS; pnum++) { if (playeringame[pnum] != false) { break; } }
    }
    if ((pnum >= MAXPLAYERS) || (players[pnum].mo == NULL)) { automapactive = false; return; }
    plr = &players[pnum]; m_x = plr->mo->x - (m_w/2); m_y = plr->mo->y - (m_h/2); AM_changeWindowLoc(); old_m_x = m_x; old_m_y = m_y; old_m_w = m_w; old_m_h = m_h; ST_Responder(&st_notify);
}

void AM_loadPics(void)
{
    int i; char namebuf[9] = { 'A', 'M', 'M', 'N', 'U', 'M', '0', '\0', '\0' };
    for (i = 0; i < 10; i++) { namebuf[6] = (char)('0' + i); marknums[i] = W_CacheLumpName(namebuf, PU_STATIC); }
}

void AM_unloadPics(void)
{
    int i; for (i = 0; i < 10; i++) { Z_ChangeTag(marknums[i], PU_CACHE); }
}

void AM_clearMarks(void)
{
    int i; for (i = 0; i < AM_NUMMARKPOINTS; i++) { markpoints[i].x = -1; } markpointnum = 0;
}

void AM_LevelInit(void)
{
    leveljuststarted = 0; f_x = 0; f_y = 0; f_w = finit_width; f_h = finit_height; AM_clearMarks(); AM_findMinMaxBoundaries(); scale_mtof = FixedDiv(min_scale_mtof, (int)((7*FRACUNIT)/10)); if (scale_mtof > max_scale_mtof) { scale_mtof = min_scale_mtof; } scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

void AM_Stop(void)
{
    static event_t st_notify = { ev_keyup, AM_MSGEXITED, 0, 0 }; AM_unloadPics(); automapactive = false; ST_Responder(&st_notify); stopped = true;
}

void AM_Start(void)
{
    static int lastlevel = -1, lastepisode = -1;
    if (stopped == false) { AM_Stop(); }
    stopped = false;
    if ((lastlevel != gamemap) || (lastepisode != gameepisode)) { AM_LevelInit(); lastlevel = gamemap; lastepisode = gameepisode; }
    AM_initVariables(); if (automapactive != false) { AM_loadPics(); }
}

void AM_minOutWindowScale(void)
{
    scale_mtof = min_scale_mtof; scale_ftom = FixedDiv(FRACUNIT, scale_mtof); AM_activateNewScale();
}

void AM_maxOutWindowScale(void)
{
    scale_mtof = max_scale_mtof; scale_ftom = FixedDiv(FRACUNIT, scale_mtof); AM_activateNewScale();
}

boolean AM_Responder(event_t* ev)
{
    int rc; static int bigstate = 0; static char buffer[20]; int msg_index;
    rc = false;
    if (ev == NULL) { return false; }
    if (automapactive == false) {
        if ((ev->type == ev_keydown) && (ev->data1 == AM_STARTKEY)) { AM_Start(); viewactive = false; rc = true; }
    } else if (ev->type == ev_keydown) {
        rc = true;
        switch (ev->data1) {
          case AM_PANRIGHTKEY: if (followplayer == 0) { m_paninc.x = FTOM(F_PANINC); } else { rc = false; } break;
          case AM_PANLEFTKEY: if (followplayer == 0) { m_paninc.x = -FTOM(F_PANINC); } else { rc = false; } break;
          case AM_PANUPKEY: if (followplayer == 0) { m_paninc.y = FTOM(F_PANINC); } else { rc = false; } break;
          case AM_PANDOWNKEY: if (followplayer == 0) { m_paninc.y = -FTOM(F_PANINC); } else { rc = false; } break;
          case AM_ZOOMOUTKEY: mtof_zoommul = M_ZOOMOUT; ftom_zoommul = M_ZOOMIN; break;
          case AM_ZOOMINKEY: mtof_zoommul = M_ZOOMIN; ftom_zoommul = M_ZOOMOUT; break;
          case AM_ENDKEY: bigstate = 0; viewactive = true; AM_Stop(); break;
          case AM_GOBIGKEY: bigstate = (bigstate == 0) ? 1 : 0; if (bigstate != 0) { AM_saveScaleAndLoc(); AM_minOutWindowScale(); } else { AM_restoreScaleAndLoc(); } break;
          case AM_FOLLOWKEY: followplayer = (followplayer == 0) ? 1 : 0; f_oldloc.x = MAXINT; plr->message = (followplayer != 0) ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF; break;
          case AM_GRIDKEY: grid = (grid == 0) ? 1 : 0; plr->message = (grid != 0) ? AMSTR_GRIDON : AMSTR_GRIDOFF; break;
          case AM_MARKKEY: for (msg_index = 0; (msg_index < 18) && (AMSTR_MARKEDSPOT[msg_index] != '\0'); msg_index++) { buffer[msg_index] = AMSTR_MARKEDSPOT[msg_index]; } if (msg_index < 18) { buffer[msg_index++] = ' '; } buffer[msg_index++] = (char)('0' + markpointnum); buffer[msg_index] = '\0'; plr->message = buffer; AM_addMark(); break;
          case AM_CLEARMARKKEY: AM_clearMarks(); plr->message = AMSTR_MARKSCLEARED; break;
          default: rc = false; break;
        }
        if (deathmatch == false) { if (cht_CheckCheat(&cheat_amap, ev->data1) != 0) { rc = false; cheating = (cheating + 1) % 3; } }
    } else if (ev->type == ev_keyup) {
        rc = false;
        switch (ev->data1) {
          case AM_PANRIGHTKEY: if (followplayer == 0) { m_paninc.x = 0; } break;
          case AM_PANLEFTKEY: if (followplayer == 0) { m_paninc.x = 0; } break;
          case AM_PANUPKEY: if (followplayer == 0) { m_paninc.y = 0; } break;
          case AM_PANDOWNKEY: if (followplayer == 0) { m_paninc.y = 0; } break;
          case AM_ZOOMOUTKEY:
          case AM_ZOOMINKEY: mtof_zoommul = FRACUNIT; ftom_zoommul = FRACUNIT; break;
          default: break;
        }
    } else { rc = false; }
    return rc;
}

void AM_changeWindowScale(void)
{
    scale_mtof = FixedMul(scale_mtof, mtof_zoommul); scale_ftom = FixedDiv(FRACUNIT, scale_mtof); if (scale_mtof < min_scale_mtof) { AM_minOutWindowScale(); } else if (scale_mtof > max_scale_mtof) { AM_maxOutWindowScale(); } else { AM_activateNewScale(); }
}

void AM_doFollowPlayer(void)
{
    if ((plr != NULL) && (plr->mo != NULL) && ((f_oldloc.x != plr->mo->x) || (f_oldloc.y != plr->mo->y))) { m_x = FTOM(MTOF(plr->mo->x)) - (m_w/2); m_y = FTOM(MTOF(plr->mo->y)) - (m_h/2); m_x2 = m_x + m_w; m_y2 = m_y + m_h; f_oldloc.x = plr->mo->x; f_oldloc.y = plr->mo->y; }
}

void AM_updateLightLev(void)
{
    static int nexttic = 0; static int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 }; static int litelevelscnt = 0;
    if (amclock != 0) { lightlev = litelevels[litelevelscnt]; litelevelscnt++; if (litelevelscnt == (int)(sizeof(litelevels)/sizeof(int))) { litelevelscnt = 0; } nexttic = amclock + 6 - (amclock % 6); }
}

void AM_Ticker(void)
{
    if (automapactive == false) { return; }
    amclock++;
    if (followplayer != 0) { AM_doFollowPlayer(); }
    if (ftom_zoommul != FRACUNIT) { AM_changeWindowScale(); }
    if ((m_paninc.x != 0) || (m_paninc.y != 0)) { AM_changeWindowLoc(); }
}

void AM_clearFB(int color)
{
    memset(fb, color, f_w*f_h);
}

static int AM_outCode(int mx, int my)
{
    int oc = 0; if (my < 0) { oc |= AM_TOP; } else if (my >= f_h) { oc |= AM_BOTTOM; } if (mx < 0) { oc |= AM_LEFT; } else if (mx >= f_w) { oc |= AM_RIGHT; } return oc;
}

boolean AM_clipMline(mline_t* ml, fline_t* fl)
{
    int outcode1 = 0; int outcode2 = 0; int outside = 0; fpoint_t tmp; int dx; int dy;
    if ((ml == NULL) || (fl == NULL)) { return false; }
    if (ml->a.y > m_y2) { outcode1 = AM_TOP; } else if (ml->a.y < m_y) { outcode1 = AM_BOTTOM; }
    if (ml->b.y > m_y2) { outcode2 = AM_TOP; } else if (ml->b.y < m_y) { outcode2 = AM_BOTTOM; }
    if ((outcode1 & outcode2) != 0) { return false; }
    if (ml->a.x < m_x) { outcode1 |= AM_LEFT; } else if (ml->a.x > m_x2) { outcode1 |= AM_RIGHT; }
    if (ml->b.x < m_x) { outcode2 |= AM_LEFT; } else if (ml->b.x > m_x2) { outcode2 |= AM_RIGHT; }
    if ((outcode1 & outcode2) != 0) { return false; }
    fl->a.x = CXMTOF(ml->a.x); fl->a.y = CYMTOF(ml->a.y); fl->b.x = CXMTOF(ml->b.x); fl->b.y = CYMTOF(ml->b.y);
    outcode1 = AM_outCode(fl->a.x, fl->a.y); outcode2 = AM_outCode(fl->b.x, fl->b.y);
    if ((outcode1 & outcode2) != 0) { return false; }
    while ((outcode1 | outcode2) != 0) {
        outside = (outcode1 != 0) ? outcode1 : outcode2;
        if ((outside & AM_TOP) != 0) { dy = fl->a.y - fl->b.y; if (dy == 0) { return false; } dx = fl->b.x - fl->a.x; tmp.x = fl->a.x + ((dx * fl->a.y) / dy); tmp.y = 0; }
        else if ((outside & AM_BOTTOM) != 0) { dy = fl->a.y - fl->b.y; if (dy == 0) { return false; } dx = fl->b.x - fl->a.x; tmp.x = fl->a.x + ((dx * (fl->a.y - f_h)) / dy); tmp.y = f_h - 1; }
        else if ((outside & AM_RIGHT) != 0) { dy = fl->b.y - fl->a.y; dx = fl->b.x - fl->a.x; if (dx == 0) { return false; } tmp.y = fl->a.y + ((dy * ((f_w - 1) - fl->a.x)) / dx); tmp.x = f_w - 1; }
        else { dy = fl->b.y - fl->a.y; dx = fl->b.x - fl->a.x; if (dx == 0) { return false; } tmp.y = fl->a.y + ((dy * (-fl->a.x)) / dx); tmp.x = 0; }
        if (outside == outcode1) { fl->a = tmp; outcode1 = AM_outCode(fl->a.x, fl->a.y); } else { fl->b = tmp; outcode2 = AM_outCode(fl->b.x, fl->b.y); }
        if ((outcode1 & outcode2) != 0) { return false; }
    }
    return true;
}

int multi(int x)
{
    return (x != 0) ? 1 : 0;
}

void AM_drawFline(fline_t* fl, int color)
{
    int x; int y; int dx; int dy; int sx; int sy; int ax; int ay; int d; boolean done; static int debug_count = 0;
    if ((fl == NULL) || (fb == NULL)) { return; }
    if ((fl->a.x < 0) || (fl->a.x >= f_w) || (fl->a.y < 0) || (fl->a.y >= f_h) || (fl->b.x < 0) || (fl->b.x >= f_w) || (fl->b.y < 0) || (fl->b.y >= f_h)) { debug_count++; return; }
    dx = fl->b.x - fl->a.x; ax = 2 * ((dx < 0) ? -dx : dx); sx = (dx < 0) ? -1 : 1; dy = fl->b.y - fl->a.y; ay = 2 * ((dy < 0) ? -dy : dy); sy = (dy < 0) ? -1 : 1; x = fl->a.x; y = fl->a.y; done = false;
    if (ax > ay) { d = ay - (ax/2); while (done == false) { fb[(y*f_w)+x] = color; if (x == fl->b.x) { done = true; } else { if (d >= 0) { y += sy; d -= ax; } x += sx; d += ay; } } }
    else { d = ax - (ay/2); while (done == false) { fb[(y*f_w)+x] = color; if (y == fl->b.y) { done = true; } else { if (d >= 0) { x += sx; d -= ay; } y += sy; d += ax; } } }
}

void AM_drawMline(mline_t* ml, int color)
{
    static fline_t fl; if (AM_clipMline(ml, &fl) != false) { AM_drawFline(&fl, color); }
}

void AM_drawGrid(int color)
{
    fixed_t x, y; fixed_t start, end; fixed_t block; mline_t ml;
    block = MAPBLOCKUNITS<<FRACBITS;
    start = m_x; if (((start - bmaporgx) % block) != 0) { start += block - ((start - bmaporgx) % block); }
    end = m_x + m_w; ml.a.y = m_y; ml.b.y = m_y + m_h; for (x = start; x < end; x += block) { ml.a.x = x; ml.b.x = x; AM_drawMline(&ml, color); }
    start = m_y; if (((start - bmaporgy) % block) != 0) { start += block - ((start - bmaporgy) % block); }
    end = m_y + m_h; ml.a.x = m_x; ml.b.x = m_x + m_w; for (y = start; y < end; y += block) { ml.a.y = y; ml.b.y = y; AM_drawMline(&ml, color); }
}

void AM_drawWalls(void)
{
    int i; static mline_t l;
    for (i = 0; i < numlines; i++) {
        l.a.x = lines[i].v1->x; l.a.y = lines[i].v1->y; l.b.x = lines[i].v2->x; l.b.y = lines[i].v2->y;
        if ((cheating != 0) || ((lines[i].flags & ML_MAPPED) != 0)) {
            if (((lines[i].flags & LINE_NEVERSEE) != 0) && (cheating == 0)) { continue; }
            if (lines[i].backsector == NULL) { AM_drawMline(&l, WALLCOLORS + lightlev); }
            else { if (lines[i].special == 39) { AM_drawMline(&l, WALLCOLORS + (WALLRANGE/2)); }
              else if ((lines[i].flags & ML_SECRET) != 0) { if (cheating != 0) { AM_drawMline(&l, SECRETWALLCOLORS + lightlev); } else { AM_drawMline(&l, WALLCOLORS + lightlev); } }
              else if (lines[i].backsector->floorheight != lines[i].frontsector->floorheight) { AM_drawMline(&l, FDWALLCOLORS + lightlev); }
              else if (lines[i].backsector->ceilingheight != lines[i].frontsector->ceilingheight) { AM_drawMline(&l, CDWALLCOLORS + lightlev); }
              else if (cheating != 0) { AM_drawMline(&l, TSWALLCOLORS + lightlev); }
              else { }
            }
        } else if (plr->powers[pw_allmap] != 0) { if ((lines[i].flags & LINE_NEVERSEE) == 0) { AM_drawMline(&l, GRAYS + 3); } }
    }
}

void AM_rotate(fixed_t* x, fixed_t* y, angle_t a)
{
    fixed_t tmpx; if ((x == NULL) || (y == NULL)) { return; } tmpx = FixedMul(*x, finecosine[a>>ANGLETOFINESHIFT]) - FixedMul(*y, finesine[a>>ANGLETOFINESHIFT]); *y = FixedMul(*x, finesine[a>>ANGLETOFINESHIFT]) + FixedMul(*y, finecosine[a>>ANGLETOFINESHIFT]); *x = tmpx;
}

void AM_drawLineCharacter(mline_t* lineguy, int lineguylines, fixed_t scale, angle_t angle, int color, fixed_t x, fixed_t y)
{
    int i; mline_t l;
    if (lineguy == NULL) { return; }
    for (i = 0; i < lineguylines; i++) { l.a.x = lineguy[i].a.x; l.a.y = lineguy[i].a.y; if (scale != 0) { l.a.x = FixedMul(scale, l.a.x); l.a.y = FixedMul(scale, l.a.y); } if (angle != 0) { AM_rotate(&l.a.x, &l.a.y, angle); } l.a.x += x; l.a.y += y; l.b.x = lineguy[i].b.x; l.b.y = lineguy[i].b.y; if (scale != 0) { l.b.x = FixedMul(scale, l.b.x); l.b.y = FixedMul(scale, l.b.y); } if (angle != 0) { AM_rotate(&l.b.x, &l.b.y, angle); } l.b.x += x; l.b.y += y; AM_drawMline(&l, color); }
}

void AM_drawPlayers(void)
{
    int i; player_t* p; static int their_colors[] = { GREENS, GRAYS, BROWNS, REDS }; int their_color = -1; int color;
    if ((plr == NULL) || (plr->mo == NULL)) { return; }
    if (netgame == false) { if (cheating != 0) { AM_drawLineCharacter(cheat_player_arrow, NUMCHEATPLYRLINES, 0, plr->mo->angle, WHITE, plr->mo->x, plr->mo->y); } else { AM_drawLineCharacter(player_arrow, NUMPLYRLINES, 0, plr->mo->angle, WHITE, plr->mo->x, plr->mo->y); } return; }
    for (i = 0; i < MAXPLAYERS; i++) { their_color++; p = &players[i]; if (((deathmatch != false) && (singledemo == false)) && (p != plr)) { continue; } if (playeringame[i] == false) { continue; } if (p->powers[pw_invisibility] != 0) { color = 246; } else { color = their_colors[their_color]; } if (p->mo != NULL) { AM_drawLineCharacter(player_arrow, NUMPLYRLINES, 0, p->mo->angle, color, p->mo->x, p->mo->y); } }
}

void AM_drawThings(int colors, int colorrange)
{
    int i; mobj_t* t; (void)colorrange;
    for (i = 0; i < numsectors; i++) { t = sectors[i].thinglist; while (t != NULL) { AM_drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES, 16<<FRACBITS, t->angle, colors + lightlev, t->x, t->y); t = t->snext; } }
}

void AM_drawMarks(void)
{
    int i, fx, fy, w, h;
    for (i = 0; i < AM_NUMMARKPOINTS; i++) { if (markpoints[i].x != -1) { w = 5; h = 6; fx = CXMTOF(markpoints[i].x); fy = CYMTOF(markpoints[i].y); if ((fx >= f_x) && (fx <= (f_w - w)) && (fy >= f_y) && (fy <= (f_h - h))) { V_DrawPatch(fx, fy, FB, marknums[i]); } } }
}

void AM_drawCrosshair(int color)
{
    if (fb != NULL) { fb[(f_w*(f_h+1))/2] = color; }
}

void AM_Drawer(void)
{
    if (automapactive == false) { return; }
    AM_clearFB(BACKGROUND); if (grid != 0) { AM_drawGrid(GRIDCOLORS); } AM_drawWalls(); AM_drawPlayers(); if (cheating == 2) { AM_drawThings(THINGCOLORS, THINGRANGE); } AM_drawCrosshair(XHAIRCOLORS); AM_drawMarks(); V_MarkRect(f_x, f_y, f_w, f_h);
}
