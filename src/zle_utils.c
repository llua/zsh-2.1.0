/*

	zle_utils.c - miscellaneous line editor utilities

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

/* make sure that the line buffer has at least sz chars */

void sizeline(sz) /**/
int sz;
{
	while (sz > linesz)
		line = realloc(line,(linesz *= 4)+1);
}

/* insert space for ct chars at cursor position */

void spaceinline(ct) /**/
int ct;
{
int i;

	while (ct+ll > linesz)
		line = realloc(line,(linesz *= 4)+1);
	for (i = ll; i >= cs; i--)
		line[i+ct] = line[i];
	ll += ct;
	line[ll] = '\0';
}

void backkill(ct,dir) /**/
int ct;int dir;
{
int i = (cs -= ct);

	cut(i,ct,dir);
	while (line[i] = line[i+ct])
		i++;
	ll -= ct;
}

void forekill(ct,dir) /**/
int ct;int dir;
{
int i = cs;

	cut(i,ct,dir);
	while (line[i] = line[i+ct])
		i++;
	ll -= ct;
}

void cut(i,ct,dir) /**/
int i;int ct;int dir;
{
	if (vibufspec) {
		int owrite = 1;
		if (vibufspec >= 'A' && vibufspec <= 'Z') {
			owrite = 0; vibufspec = tolower(vibufspec);
		}
		vibufspec += (idigit(vibufspec)) ? - '1' +26 : - 'a';
		if (owrite || !vibuf[vibufspec]) {
			if (vibuf[vibufspec]) free(vibuf[vibufspec]);
			vibuf[vibufspec] = zalloc(ct+1);
			ztrncpy(vibuf[vibufspec],line+i,ct);
		} else {
			int len = strlen(vibuf[vibufspec]);
			vibuf[vibufspec] = realloc(vibuf[vibufspec],ct+len);
			ztrncpy(vibuf[vibufspec]+len,line+i,ct);
		}
		vibufspec = 0;
		return;
	}
	if (!cutbuf)
		cutbuf = ztrdup("");
	else if (!(lastcmd & ZLE_KILL)) {
		kringnum = (kringnum+1)&(KRINGCT-1);
		if (kring[kringnum])
			free(kring[kringnum]);
		kring[kringnum] = cutbuf;
		cutbuf = ztrdup("");
	}
	if (dir) {
		char *s = zalloc(strlen(cutbuf)+ct+1);
		strncpy(s,line+i,ct);
		strcpy(s+ct,cutbuf);
		free(cutbuf);
		cutbuf = s;
	} else {
		int x;

		cutbuf = realloc(cutbuf,(x = strlen(cutbuf))+ct+1);
		ztrncpy(cutbuf+x,line+i,ct);
	}
}

void backdel(ct) /**/
int ct;
{
int i = (cs -= ct);

	while (line[i] = line[i+ct])
		i++;
	ll -= ct;
}

void foredel(ct) /**/
int ct;
{
int i = cs;

	while (line[i] = line[i+ct])
		i++;
	ll -= ct;
}

void setline(s) /**/
char *s;
{
	sizeline(strlen(s));
	strcpy(line,s);
	cs = ll = strlen(s);
	if (cs && bindtab == altbindtab) cs--;
}

void sethistline(s) /**/
char *s;
{
	setline(s);
	for (s = line; *s; s++)
		if (*s == HISTSPACE)
			*s = ' ';
}

int findbol() /**/
{
int x = cs;

	while (x > 0 && line[x-1] != '\n') x--;
	return x;
}

int findeol() /**/
{
int x = cs;

	while (x != ll && line[x] != '\n') x++;
	return x;
}

void findline(a,b) /**/
int *a;int *b;
{
	*a = findbol();
	*b = findeol();
}

static int lastlinelen;

void initundo() /**/
{
int t0;

	for (t0 = 0; t0 != UNDOCT; t0++)
		undos[t0].change = NULL;
	undoct = 0;
	lastline = zalloc(lastlinelen = (ll+1 < 32) ? 32 : ll+1);
	strcpy(lastline,line);
	lastcs = cs;
}

void addundo() /**/
{
int pf,sf;
char *s,*s2,*t,*t2;
struct undoent *ue;

	for (s = line, t = lastline; *s && *s==*t; s++,t++);
	if (!*s && !*t)
		return;
	pf = s-line;
	for (s2 = line+strlen(line), t2 = lastline+strlen(lastline);
		s2 > s && t > t2 && s2[-1] == t2[-1]; s2--,t2--);
	sf = strlen(s2);
	ue = undos+(undoct = (UNDOCT-1) & (undoct+1));
	ue->pref = pf;
	ue->suff = sf;
	ue->len = t2-t;
	ue->cs = lastcs;
	strncpy(ue->change = halloc(ue->len),t,ue->len);
	while (ll+1 > lastlinelen)
		{
		free(lastline);
		lastline = zalloc(lastlinelen *= 2);
		}
	strcpy(lastline,line);
	lastcs = cs;
}

void freeundo() /**/
{
	free(lastline);
}

int hstrncmp(s,t,len) /**/
char *s;char *t;int len;
{
	while (len && *s && (*s == *t || (*s == ' ' && *t == HISTSPACE) ||
			(*s == HISTSPACE && *t == ' ')))
		s++,t++,len--;
	return len;
}

int hstrcmp(s,t) /**/
char *s;char *t;
{
	while (*s && (*s == *t || (*s == ' ' && *t == HISTSPACE) ||
			(*s == HISTSPACE && *t == ' ')))
		s++,t++;
	return !(*s == '\0' && *t == '\0');
}

char *hstrnstr(s,t,len) /**/
char *s;char *t;int len;
{
	for (; *s; s++)
		if (!hstrncmp(t,s,len))
			return s;
	return NULL;
}

