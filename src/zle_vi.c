/*

	zle_vi.c - vi-specific functions

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


static void startvichange(im)
int im;
{
	insmode = im;
	if (vichgbuf) free(vichgbuf);
	vichgbuf = zalloc(vichgbufsz = 16);
	vichgbuf[0] = c;
	vichgbufptr = 1;
	vichgflag = 1;
	viinsbegin = cs;
}

static void startvitext(im)
{
	startvichange(im);
	bindtab = mainbindtab;
	undoing = 0;
}

int vigetkey() /**/
{
int ch;

	if ((ch = getkey(0)) == -1)
		return 0;
	if (ch == 22)
		{
		if ((ch = getkey(0)) == -1)
			return 0;
		return ch;
		}
	else if (ch == 27)
		return 0;
	return ch;
}

int getvirange() /**/
{
int k2,t0,startline;

	startline = findbol();
	for (;;) {
		k2 = getkeycmd();
		if (k2 == -1) {
			feep();
			return -1;
		}
		if (zlecmds[k2].flags & ZLE_ARG)
			zlecmds[k2].func();
		else
			break;
	}
	if (k2 == bindk) {
		findline(&cs,&t0);
		return (t0 == ll) ? t0 : t0+1;
	}
	if (!(zlecmds[k2].flags & ZLE_MOVEMENT)) {
		feep();
		return -1;
	}
	t0 = cs;

	virangeflag = 1;
	zlecmds[k2].func();
	virangeflag = 0;
	if (cs == t0) {
		feep();
		return -1;
	}
	if (startline != findbol()) {
		if (cs < t0) {
			cs = startline;
			t0 = findeol()+1;
		} else {
			t0 = startline;
			cs = findeol()+1;
		}
	}
	if (cs > t0) {
		k2 = cs;
		cs = t0;
		t0 = k2;
	}
	return t0;
}

void viaddnext() /**/
{
	if (cs != ll)
		cs++;
	startvitext(1);
}

void viaddeol() /**/
{
	cs = findeol();
	startvitext(1);
}

void viinsert() /**/
{
	startvitext(1);
}

void viinsertbol() /**/
{
	cs = findbol();
	startvitext(1);
}

void videlete() /**/
{
int c2;

	startvichange(1);
	if ((c2 = getvirange()) == -1)
		{ vichgflag = 0; return; }
	forekill(c2-cs,0);
	vichgflag = 0;
}

void vichange() /**/
{
int c2;

	startvichange(1);
	if ((c2 = getvirange()) == -1)
		{ vichgflag = 0; return; }
	forekill(c2-cs,0);
	bindtab = mainbindtab;
	undoing = 0;
}

void visubstitute() /**/
{
	if (mult < 0) return;
	if (findeol()-cs < mult) mult = findeol()-cs;
	if (mult) {
		foredel(mult);
		startvitext(1);
	}
}

void vichangeeol() /**/
{
	killline();
	startvitext(1);
}

void vichangewholeline() /**/
{
int cq;

	findline(&cs,&cq);
	foredel(cq-cs);
	startvitext(1);
}

void viyank() /**/
{
int c2;

	if ((c2 = getvirange()) == -1)
		return;
	cut(cs,c2-cs,0);
}

void viyankeol() /**/
{
int x = findeol();

	if (x == cs)
		feep();
	else
		cut(cs,x-cs,0);
}

void vireplace() /**/
{
	startvitext(0);
}

void vireplacechars() /**/
{
int ch;

	if (mult < 0) return;
	if (mult+cs > ll) {
		feep();
		return;
	}
	startvichange(1);
	if (ch = vigetkey())
		while (mult--)
			line[cs++] = ch;
	vichgflag = 0;
}

void vicmdmode() /**/
{
	bindtab = altbindtab;
	if (cs) cs--;
	undoing = 1;
	if (vichgflag) vichgflag = 0;
}

void viopenlinebelow() /**/
{
	cs = findeol();
	spaceinline(1);
	line[cs++] = '\n';
	startvitext(1);
}

void viopenlineabove() /**/
{
	cs = findbol();
	spaceinline(1);
	line[cs] = '\n';
	startvitext(1);
}

void vioperswapcase() /**/
{
int c2;

	if ((c2 = getvirange()) == -1)
		return;
	while (cs < c2)
		{
		int ch = line[cs];

		if (ch >= 'a' && ch <= 'z')
			ch = tuupper(ch);
		else if (ch >= 'A' && ch <= 'Z')
			ch = tulower(ch);
		line[cs++] = ch;
		}
}

void virepeatchange() /**/
{
	if (!vichgbuf || bindtab == mainbindtab || vichgflag) feep();
	else ungetkeys(vichgbuf,vichgbufptr);
}

void viindent() /**/
{
int c2,endcs,t0,rmult;

	if (mult < 0) { mult = -mult; viunindent(); return; }
	rmult = mult;
	if ((c2 = getvirange()) == -1)
		return;
	if (cs != findbol()) { feep(); return; }
	endcs = cs+rmult;
	while (cs < c2) {
		spaceinline(rmult);
		for (t0 = 0; t0 != rmult; t0++) line[cs++] = '\t';
		cs = findeol()+1;
	}
	cs = endcs;
}

void viunindent() /**/
{
int c2,endcs,t0,rmult;

	rmult = mult;
	if (mult < 0) { mult = -mult; viindent(); return; }
	if ((c2 = getvirange()) == -1)
		return;
	if (cs != findbol()) { feep(); return; }
	endcs = cs;
	while (cs < c2) {
		for (t0 = 0; t0 != rmult && line[cs] == '\t'; t0++) foredel(1);
		cs = findeol()+1;
	}
	cs = endcs;
}
