/*

	zle_move.c - editor movement

	This file is part of zsh, the Z shell.

	zsh is free software; no one can prevent you from reading the source
   code, or giving it to someone else.

   This file is copyrighted under the GNU General Public License, which
   can be found in the file called COPYING.

   Copyright (C) 1990, 1991 Paul Falstad

   zsh is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY.  No author or distributor accepts
   responsibility to anyone for the consequences of using it or for
   whether it serves any particular purpose or works at all, unless he
   says so in writing.  Refer to the GNU General Public License
   for full details.

   Everyone is granted permission to copy, modify and redistribute
   zsh, but only under the conditions described in the GNU General Public
   License.   A copy of this license is supposed to have been given to you
   along with zsh so you can know your rights and responsibilities.
   It should be in a file named COPYING.

   Among other things, the copyright notice and this notice must be
   preserved on all copies.

*/

#define ZLE
#include "zsh.h"


void beginningofline() /**/
{
	if (mult < 0) { mult = -mult; endofline(); return; }
	while (mult--) {
		if (cs == 0)
			return;
		if (line[cs-1] == '\n')
			if (!--cs)
				return;
		while (cs && line[cs-1] != '\n') cs--;
	}
}

void endofline() /**/
{
	if (mult < 0) { mult = -mult; beginningofline(); return; }
	while (mult--) {
		if (cs >= ll) {
			cs = ll;
			return;
		}
		if (line[cs] == '\n')
			if (++cs == ll)
				return;
		while (cs != ll && line[cs] != '\n') cs++;
	}
}

void beginningoflinehist() /**/
{
	if (mult < 0) { mult = -mult; endoflinehist(); return; }
	while (mult) {
		if (cs == 0)
			break;
		if (line[cs-1] == '\n')
			if (!--cs)
				break;
		while (cs && line[cs-1] != '\n') cs--;
		mult--;
	}
	if (mult) {
		uphistory();
		cs = 0;
	}
}

void endoflinehist() /**/
{
	if (mult < 0) { mult = -mult; beginningoflinehist(); return; }
	while (mult) {
		if (cs >= ll) {
			cs = ll;
			break;
		}
		if (line[cs] == '\n')
			if (++cs == ll)
				break;
		while (cs != ll && line[cs] != '\n') cs++;
		mult--;
	}
	if (mult)
		downhistory();
}

void forwardchar() /**/
{
	cs += mult;
	if (cs > ll) cs = ll;
	if (cs <  0) cs = 0;
}

void backwardchar() /**/
{
	cs -= mult;
	if (cs > ll) cs = ll;
	if (cs <  0) cs = 0;
}

void setmarkcommand() /**/
{
	mark = cs;
}

void exchangepointandmark() /**/
{
int x;

	x = mark;
	mark = cs;
	cs = x;
	if (cs > ll)
		cs = ll;
}

void vigotocolumn() /**/
{
int x,y,ocs = cs;

	if (mult > 0) mult--;
	findline(&x,&y);
	if (mult >= 0) cs = x+mult; else cs = y+mult;
	if (cs < x || cs > y) {
		feep();
		cs = ocs;
	}
}

void vimatchbracket() /**/
{
int ocs = cs,dir,ct;
char oth,me;

otog:
	if (cs == ll)
		{
		feep();
		cs = ocs;
		return;
		}
	switch(me = line[cs])
		{
		case '{': dir = 1; oth = '}'; break;
		case '}': dir = -1; oth = '{'; break;
		case '(': dir = 1; oth = ')'; break;
		case ')': dir = -1; oth = '('; break;
		case '[': dir = 1; oth = ']'; break;
		case ']': dir = -1; oth = '['; break;
		default: cs++; goto otog;
		}
	ct = 1;
	while (cs >= 0 && cs < ll && ct)
		{
		cs += dir;
		if (line[cs] == oth)
			ct--;
		else if (line[cs] == me)
			ct++;
		}
	if (cs < 0 || cs >= ll)
		{
		feep();
		cs = ocs;
		}
}

void viforwardchar() /**/
{
	if (mult < 0) { mult = -mult; vibackwardchar(); return; }
	while (mult--) {
		cs++;
		if (cs >= ll || line[cs] == '\n') {
			cs--;
			break;
		}
	}
}

void vibackwardchar() /**/
{
	if (mult < 0) { mult = -mult; viforwardchar(); return; }
	while (mult--) {
		cs--;
		if (cs < 0 || line[cs] == '\n') {
			cs++;
			break;
		}
	}
}

void viendofline() /**/
{
	cs = findeol();
	if (!virangeflag && cs != 0 && line[cs-1] != '\n') cs--;
}

void vibeginningofline() /**/
{
	cs = findbol();
}


static int vfindchar,vfinddir,tailadd;

void vifindnextchar() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = 1;
		tailadd = 0;
		virepeatfind();
		}
}

void vifindprevchar() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = -1;
		tailadd = 0;
		virepeatfind();
		}
}

void vifindnextcharskip() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = 1;
		tailadd = -1;
		virepeatfind();
		}
}

void vifindprevcharskip() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = -1;
		tailadd = 1;
		virepeatfind();
		}
}

void virepeatfind() /**/
{
int ocs = cs;

	if (mult < 0) { mult = -mult; virevrepeatfind(); return; }
	while (mult--)
		{
		do
			cs += vfinddir;
		while (cs >= 0 && cs < ll && line[cs] != vfindchar && line[cs] != '\n');
		if (cs < 0 || cs >= ll || line[cs] == '\n')
			{
			feep();
			cs = ocs;
			return;
			}
		}
	cs += tailadd;
}

void virevrepeatfind() /**/
{
	if (mult < 0) { mult = -mult; virepeatfind(); return; }
	vfinddir = -vfinddir;
	virepeatfind();
	vfinddir = -vfinddir;
}

void vifirstnonblank() /**/
{
	cs = findbol();
	while (cs != ll && iblank(line[cs]))
		cs++;
}

void visetmark() /**/
{
int ch;

	ch = getkey(1);
	if (ch < 'a' || ch > 'z') {
		feep();
		return;
	}
	ch -= 'a';
	vimarkcs[ch] = cs;
	vimarkline[ch] = histline;
}

void vigotomark() /**/
{
int ch;

	ch = getkey(1);
	if (ch == c) ch = 26;
	else {
		if (ch < 'a' || ch > 'z') {
			feep();
			return;
		}
		ch -= 'a';
	}
	if (!vimarkline[ch]) {
		feep();
		return;
	}
	if (curhist != vimarkline[ch]) {
		mult = vimarkline[ch];
		vifetchhistory();
		if (curhist != vimarkline[ch]) return;
	}
	cs = vimarkcs[ch];
	if (cs > ll) ch = ll;
}

void vigotomarkline() /**/
{
	vigotomark();
	cs = findbol();
}
