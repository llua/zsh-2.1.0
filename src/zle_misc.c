/*

	zle_misc.c - miscellaneous editor routines

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


void selfinsert() /**/
{
int ncs = cs+mult;

	if (mult < 0) { mult = -mult; ncs = cs; }
	if (insmode || ll == cs)
		spaceinline(mult);
	else if (mult+cs > ll)
		spaceinline(ll-(mult+cs));
	while (mult--)
		line[cs++] = c;
	cs = ncs;
}

void selfinsertunmeta() /**/
{
	c &= 0x7f;
	if (c == '\r') c = '\n';
	selfinsert();
}

void deletechar() /**/
{
	if (mult < 0) { mult = -mult; backwarddeletechar(); return; }
	if (c == 4 && !ll)
		{
		eofsent = 1;
		return;
		}
	if (!(cs+mult > ll || line[cs] == '\n'))
		{
		cs += mult;
		backdel(mult);
		}
	else
		feep();
}

void backwarddeletechar() /**/
{
	if (mult < 0) { mult = -mult; deletechar(); return; }
	if (mult > cs)
		mult = cs;
	backdel(mult);
}

void videletechar() /**/
{
	if (mult < 0) { mult = -mult; vibackwarddeletechar(); return; }
	if (c == 4 && !ll) {
		eofsent = 1;
		return;
	}
	if (!(cs+mult > ll || line[cs] == '\n')) {
		cs += mult;
		backkill(mult,0);
		if (cs && (cs == ll || line[cs] == '\n')) cs--;
	} else
		feep();
}

void vibackwarddeletechar() /**/
{
	if (mult < 0) { mult = -mult; videletechar(); return; }
	if (mult > cs)
		mult = cs;
	if (cs-mult < viinsbegin) { feep(); return; }
	backkill(mult,1);
}

void vikillline() /**/
{
	if (viinsbegin > cs) { feep(); return; }
	backdel(cs-viinsbegin);
}

void killwholeline() /**/
{
int i,fg;

	if (mult < 0) return;
	while (mult--)
		{
		if (fg = (cs && cs == ll))
			cs--;
		while (cs && line[cs-1] != '\n') cs--;
		for (i = cs; i != ll && line[i] != '\n'; i++);
		forekill(i-cs+(i != ll),fg);
		}
}

void killbuffer() /**/
{
	cs = 0;
	forekill(ll,0);
}

void backwardkillline() /**/
{
int i = 0;

	if (mult < 0) { mult = -mult; killline(); return; }
	while (mult--)
		{
		while (cs && line[cs-1] != '\n') cs--,i++;
		if (mult && cs && line[cs-1] == '\n')
			cs--,i++;
		}
	forekill(i,1);
}

void gosmacstransposechars() /**/
{
int cc;

	if (cs < 2 || line[cs-1] == '\n' || line[cs-2] == '\n')
		{
		if (line[cs] == '\n' || line[cs+1] == '\n')
			{
			feep();
			return;
			}
		cs += (cs == 0 || line[cs-1] == '\n') ? 2 : 1;
		}
	cc = line[cs-2];
	line[cs-2] = line[cs-1];
	line[cs-1] = cc;
}

void transposechars() /**/
{
int cc;
int neg = mult < 0;

	if (neg) mult = -mult;
	while (mult--) {
		if (cs == 0 || line[cs-1] == '\n') {
			if (ll == cs || line[cs] == '\n' || line[cs+1] == '\n') {
				feep();
				return;
			}
			cs++;
		}
		if (!neg) {
			if (cs != ll && line[cs] != '\n') cs++;
		} else {
			if (cs != 0 && line[cs-1] != '\n') cs--;
		}
		cc = line[cs-2];
		line[cs-2] = line[cs-1];
		line[cs-1] = cc;
	}
}

void acceptline() /**/
{
	done = 1;
}

void acceptandhold() /**/
{
	pushnode(bufstack,ztrdup(line));
	stackcs = cs;
	done = 1;
}

void killline() /**/
{
int i = 0;

	if (mult < 0) { mult = -mult; backwardkillline(); return; }
	while (mult--) {
		if (line[cs] == '\n')
			cs++,i++;
		while (cs != ll && line[cs] != '\n') cs++,i++;
	}
	backkill(i,0);
}

void killregion() /**/
{
	if (mark > ll)
		mark = ll;
	if (mark > cs)
		forekill(mark-cs,0);
	else
		backkill(cs-mark,1);
}

void copyregionaskill() /**/
{
	if (mark > ll)
		mark = ll;
	if (mark > cs)
		cut(cs,mark-cs,0);
	else
		cut(mark,cs-mark,1);
}

static int kct,yankb,yanke;

void yank() /**/
{
int cc;
char *buf = cutbuf;

	if (!cutbuf) {
		feep();
		return;
	}
	if (mult < 0) return;
	if (vibufspec) {
		vibufspec = tolower(vibufspec);
		vibufspec += (idigit(vibufspec)) ? -'1'+26 : -'a';
		if (!(buf = vibuf[vibufspec])) {
			feep();
			vibufspec = 0;
			return;
		}
		vibufspec = 0;
	}
	yankb = cs;
	while (mult--) {
		kct = kringnum;
		cc = strlen(buf);
		spaceinline(cc);
		strncpy(line+cs,buf,cc);
		cs += cc;
		yanke = cs;
	}
}

void viputafter() /**/
{
int cc;
char *buf = cutbuf;

	if (!cutbuf) {
		feep();
		return;
	}
	if (mult < 0) return;
	if (vibufspec) {
		vibufspec = tolower(vibufspec);
		vibufspec += (idigit(vibufspec)) ? -'1'+26 : -'a';
		if (!(buf = vibuf[vibufspec])) {
			feep();
			vibufspec = 0;
			return;
		}
		vibufspec = 0;
	}
	if (strchr(buf,'\n')) {
		cs = findeol();
		if (cs == ll) { spaceinline(1); line[cs] = '\n'; }
	}
	if (cs != ll) cs++;
	yankb = cs;
	while (mult--) {
		kct = kringnum;
		cc = strlen(buf);
		spaceinline(cc);
		strncpy(line+cs,buf,cc);
		cs += cc;
		yanke = cs;
	}
	cs = yankb;
}

void yankpop() /**/
{
int cc;

	if (!(lastcmd & ZLE_YANK) || !kring[kct]) {
		feep();
		return;
	}
	cs = yankb;
	foredel(yanke-yankb);
	cc = strlen(kring[kct]);
	spaceinline(cc);
	strncpy(line+cs,kring[kct],cc);
	cs += cc;
	yanke = cs;
	kct = (kct-1) & (KRINGCT-1);
}

void overwritemode() /**/
{
	insmode ^= 1;
}

void undefinedkey() /**/
{
	feep();
}

void quotedinsert() /**/
{
	if (c = getkey(0))
		selfinsert();
	else
		feep();
}

void digitargument() /**/
{
	if (!(lastcmd & ZLE_ARG))
		mult = 0;
	mult = mult*10+(c&0xf);
	if (lastcmd & ZLE_NEGARG) mult = -mult;
}

void negargument() /**/
{
	if (lastcmd & ZLE_ARG) feep();
}

void universalargument() /**/
{
	if (!(lastcmd & ZLE_ARG))
		mult = 4;
	else
		mult *= 4;
}

void copyprevword() /**/
{
int len,t0;

	for (t0 = cs-1; t0 >= 0; t0--)
		if (iword(line[t0]))
			break;
	for (; t0 >= 0; t0--)
		if (!iword(line[t0]))
			break;
	if (t0)
		t0++;
	len = cs-t0;
	spaceinline(len);
	strncpy(line+cs,line+t0,len);
	cs += len;
}

void sendbreak() /**/
{
	errflag = done = 1;
}

void undo() /**/
{
char *s;
struct undoent *ue;

	ue = undos+undoct;
	if (!ue->change)
		{
		feep();
		return;
		}
	line[ll] = '\0';
	s = ztrdup(line+ll-ue->suff);
	sizeline((ll = ue->pref+ue->suff+ue->len)+1);
	strncpy(line+ue->pref,ue->change,ue->len);
	strcpy(line+ue->pref+ue->len,s);
	free(s);
	ue->change = NULL;
	undoct = (undoct-1) & (UNDOCT-1);
	cs = ue->cs;
}

void quoteregion() /**/
{
char *s,*t;
int x,y;

	if (mark > ll)
		mark = ll;
	if (mark < cs)
		{
		x = mark;
		mark = cs;
		cs = x;
		}
	s = hcalloc((y = mark-cs)+1);
	strncpy(s,line+cs,y);
	s[y] = '\0';
	foredel(mark-cs);
	t = makequote(s);
	spaceinline(x = strlen(t));
	strncpy(line+cs,t,x);
	mark = cs;
	cs += x;
}

void quoteline() /**/
{
char *s;

	line[ll] = '\0';
	s = makequote(line);
	setline(s);
}

char *makequote(s) /**/
char *s;
{
int qtct = 0;
char *l,*ol;

	for (l = s; *l; l++)
		if (*l == '\'')
			qtct++;
	l = ol = halloc((qtct*3)+3+strlen(s));
	*l++ = '\'';
	for (; *s; s++)
		if (*s == '\'')
			{
			*l++ = '\'';
			*l++ = '\\';
			*l++ = '\'';
			*l++ = '\'';
			}
		else
			*l++ = *s;
	*l++ = '\'';
	*l = '\0';
	return ol;
}

#define NAMLEN 70

int executenamedcommand() /**/
{
char buf[NAMLEN],*ptr;
int len,ch,t0;

	strcpy(buf,"execute: ");
	ptr = buf+9;
	len = 0;
	statusline = buf;
	refresh();
	for (;ch = getkey(1);refresh())
		{
		switch (ch)
			{
			case 8: case 127:
				if (len)
					{
					len--;
					*--ptr = '\0';
					}
				break;
			case 23:
				while (len && (len--, *--ptr != '-'))
					*ptr = '\0';
				break;
			case 21:
				len = 0;
				ptr = buf+9;
				*ptr = '\0';
				break;
			case 10: case 13: goto brk;
			case 7: case -1: statusline = NULL; return z_undefinedkey;
			case 9: case 32:
				{
				Lklist ll;
				int ambig = 100;

				heapalloc();
				ll = newlist();
				for (t0 = 0; t0 != ZLECMDCOUNT; t0++)
					if (strpfx(buf+9,zlecmds[t0].name))
						{
						int xx;

						addnode(ll,zlecmds[t0].name);
						xx = pfxlen(peekfirst(ll),zlecmds[t0].name);
						if (xx < ambig)
							ambig = xx;
						}
				permalloc();
				if (!full(ll))
					feep();
				else if (!nextnode(firstnode(ll)))
					{
					strcpy(buf+9,peekfirst(ll));
					ptr = buf+(len = strlen(buf));
					}
				else
					{
					strcpy(buf+9,peekfirst(ll));
					len = ambig;
					ptr = buf+9+len;
					*ptr = '\0';
					feep();
					listmatches(ll,NULL);
					}
				break;
				}
			default:
				if (len == NAMLEN-10 || icntrl(ch))
					feep();
				else
					*ptr++ = ch, *ptr = '\0', len++;
				break;
			}
		}
brk:
	statusline = NULL;
	ptr = buf+9;
	for (t0 = 0; t0 != ZLECMDCOUNT; t0++)
		if (!strcmp(ptr,zlecmds[t0].name))
			break;
	if (t0 != ZLECMDCOUNT)
		return lastnamed = t0;
	else
		return z_undefinedkey;
}

void vijoin() /**/
{
int x;

	if ((x = findeol()) == ll)
		{
		feep();
		return;
		}
	cs = x+1;
	for (x = 1; cs != ll && iblank(line[cs]); cs++,x++);
	backdel(x);
	spaceinline(1);
	line[cs] = ' ';
}

void viswapcase() /**/
{
	if (cs < ll)
		{
		int ch = line[cs];

		if (ch >= 'a' && ch <= 'z')
			ch = tuupper(ch);
		else if (ch >= 'A' && ch <= 'Z')
			ch = tulower(ch);
		line[cs++] = ch;
		}
}

void vicapslockpanic() /**/
{
char ch;

	statusline = "press a lowercase key to continue";
	refresh();
	do
		ch = getkey(0);
	while (!(ch >= 'a' && ch <= 'z'));
}

void visetbuffer() /**/
{
int ch;

	ch = getkey(1);
	if (!ialnum(ch)) {
		feep();
		return;
	}
	vibufspec = ch;
}
