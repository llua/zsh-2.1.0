/*

	zle_word.c - word-related editor functions

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


void forwardword() /**/
{
	if (mult < 0) { mult = -mult; backwardword(); return; }
	while (mult--) {
		while (cs != ll && iword(line[cs])) cs++;
		while (cs != ll && !iword(line[cs])) cs++;
	}
}

void viforwardblankword() /**/
{
	if (mult < 0) { mult = -mult; vibackwardblankword(); return; }
	while (mult--) {
		while (cs != ll && !iblank(line[cs])) cs++;
		while (cs != ll && iblank(line[cs])) cs++;
	}
}

void emacsforwardword() /**/
{
	if (mult < 0) { mult = -mult; emacsbackwardword(); return; }
	while (mult--)
		{
		while (cs != ll && !iword(line[cs])) cs++;
		while (cs != ll && iword(line[cs])) cs++;
		}
}

void viforwardblankwordend() /**/
{
	if (mult < 0) return;
	while (mult--) {
		while (cs != ll && iblank(line[cs+1])) cs++;
		while (cs != ll && !iblank(line[cs+1])) cs++;
	}
}

void viforwardwordend() /**/
{
	if (mult < 0) return;
	while (mult--) {
		while (cs != ll && !iword(line[cs+1])) cs++;
		while (cs != ll && iword(line[cs+1])) cs++;
	}
}

void backwardword() /**/
{
	if (mult < 0) { mult = -mult; forwardword(); return; }
	while (mult--) {
		while (cs && !iword(line[cs-1])) cs--;
		while (cs && iword(line[cs-1])) cs--;
	}
}

void vibackwardblankword() /**/
{
	if (mult < 0) { mult = -mult; viforwardblankword(); return; }
	while (mult--) {
		while (cs && iblank(line[cs-1])) cs--;
		while (cs && !iblank(line[cs-1])) cs--;
	}
}

void emacsbackwardword() /**/
{
	if (mult < 0) { mult = -mult; emacsforwardword(); return; }
	while (mult--) {
		while (cs && !iword(line[cs-1])) cs--;
		while (cs && iword(line[cs-1])) cs--;
	}
}

void backwarddeleteword() /**/
{
int x = cs;

	if (mult < 0) { mult = -mult; deleteword(); return; }
	while (mult--) {
		while (x && !iword(line[x-1])) x--;
		while (x && iword(line[x-1])) x--;
	}
	backdel(cs-x);
}

void vibackwardkillword() /**/
{
int x = cs;

	if (mult < 0) { feep(); return; }
	while (mult--) {
		while (x > viinsbegin && !iword(line[x-1])) x--;
		while (x > viinsbegin && iword(line[x-1])) x--;
	}
	backkill(cs-x,1);
}

void backwardkillword() /**/
{
int x = cs;

	if (mult < 0) { mult = -mult; killword(); return; }
	while (mult--) {
		while (x && !iword(line[x-1])) x--;
		while (x && iword(line[x-1])) x--;
	}
	backkill(cs-x,1);
}

void upcaseword() /**/
{
int neg = mult < 0, ocs = cs;

	if (neg) mult = -mult;
	while (mult--) {
		while (cs != ll && !iword(line[cs])) cs++;
		while (cs != ll && iword(line[cs])) {
			line[cs] = tuupper(line[cs]);
			cs++;
		}
	}
	if (neg) cs = ocs;
}

void downcaseword() /**/
{
int neg = mult < 0, ocs = cs;

	if (neg) mult = -mult;
	while (mult--) {
		while (cs != ll && !iword(line[cs])) cs++;
		while (cs != ll && iword(line[cs])) {
			line[cs] = tulower(line[cs]);
			cs++;
		}
	}
	if (neg) cs = ocs;
}

void capitalizeword() /**/
{
int first;
int neg = mult < 0, ocs = cs;
	
	if (neg) mult = -mult;
	while (mult--) {
		first = 1;
		while (cs != ll && !iword(line[cs])) cs++;
		while (cs != ll && iword(line[cs])) {
			line[cs] = (first) ? tuupper(line[cs]) : tulower(line[cs]);
			first = 0;
			cs++;
		}
	}
	if (neg) cs = ocs;
}

void deleteword() /**/
{
int x = cs;

	if (mult < 0) { mult = -mult; backwarddeleteword(); return; }
	while (mult--) {
		while (x != ll && !iword(line[x])) x++;
		while (x != ll && iword(line[x])) x++;
	}
	foredel(x-cs);
}

void killword() /**/
{
int x = cs;

	if (mult < 0) { mult = -mult; backwardkillword(); return; }
	while (mult--) {
		while (x != ll && !iword(line[x])) x++;
		while (x != ll && iword(line[x])) x++;
	}
	forekill(x-cs,0);
}

void transposewords() /**/
{
int p1,p2,p3,p4,x = cs;
char *temp,*pp;
int neg = mult < 0, ocs = cs;

	if (neg) mult = -mult;
	while (mult--) {
		while (x != ll && line[x] != '\n' && !iword(line[x]))
			x++;
		if (x == ll || line[x] == '\n') {
			x = cs;
			while (x && line[x-1] != '\n' && !iword(line[x]))
				x--;
			if (!x || line[x-1] == '\n') {
				feep();
				return;
			}
		}
		for (p4 = x; p4 != ll && iword(line[p4]); p4++);
		for (p3 = p4; p3 && iword(line[p3-1]); p3--);
		if (!p3) {
			feep();
			return;
		}
		for (p2 = p3; p2 && !iword(line[p2-1]); p2--);
		if (!p2) {
			feep();
			return;
		}
		for (p1 = p2; p1 && iword(line[p1-1]); p1--);
		pp = temp = halloc(p4-p1+1);
		struncpy(&pp,line+p3,p4-p3);
		struncpy(&pp,line+p2,p3-p2);
		struncpy(&pp,line+p1,p2-p1);
		strncpy(line+p1,temp,p4-p1);
		cs = p4;
	}
	if (neg) cs = ocs;
}
