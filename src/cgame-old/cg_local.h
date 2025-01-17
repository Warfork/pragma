/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

#include "../qcommon/q_shared.h"

#define CGAME_INCLUDE 1
#include "cgame.h"

#include "cg_uidefs.h"

typedef struct
{
	int width, height;
	float time;
} cglocals;

// for memory allocation
#define TAG_CGAME 801
#define TAG_CGAME_LEVEL 802

//
// hud
//
typedef struct hud_element
{
	int x, y, w, h;
	UI_AlignX xalign;
}hud_element_t;

extern cgame_import_t gi;
extern cglocals loc;