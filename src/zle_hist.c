/*

	zle_hist.c - history editing

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

void toggleliteralhistory() /**/
{
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	lithist ^= 1;
	if (!(s = qgetevent(histline)))
		feep();
	else
		sethistline(s);
}

void uphistory() /**/
{
char *s;

	if (mult < 0) { mult = -mult; downhistory(); return; }
	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	histline -= mult;
	if (!(s = qgetevent(histline)))
		{
		feep();
		histline += mult;
		}
	else
		sethistline(s);
}

void uplineorhistory() /**/
{
int ocs = cs;

	if (mult < 0) { mult = -mult; downlineorhistory(); return; }
	if ((lastcmd & ZLE_LINEMOVE) != ZLE_LINEMOVE)
		lastcol = cs-findbol();
	cs = findbol();
	while (mult) {
		if (!cs)
			break;
		cs--;
		cs = findbol();
		mult--;
	}
	if (mult) {
		cs = ocs;
		if (virangeflag) {
			feep();
			return;
		}
		uphistory();
	} else {
		int x = findeol();
		if ((cs += lastcol) > x)
			cs = x;
	}
}

void downlineorhistory() /**/
{
int ocs = cs;

	if (mult < 0) { mult = -mult; uplineorhistory(); return; }
	if ((lastcmd & ZLE_LINEMOVE) != ZLE_LINEMOVE)
		lastcol = cs-findbol();
	while (mult) {
		int x = findeol();
		if (x == ll)
			break;
		cs = x+1;
		mult--;
	}
	if (mult) {
		cs = ocs;
		if (virangeflag) {
			feep();
			return;
		}
		downhistory();
	} else {
		int x = findeol();
		if ((cs += lastcol) > x)
			cs = x;
	}
}

void acceptlineanddownhistory() /**/
{
char *s,*t;

	if (!(s = qgetevent(histline+1)))
		{
		feep();
		return;
		}
	pushnode(bufstack,t = ztrdup(s));
	for (; *t; t++)
		if (*t == HISTSPACE)
			*t = ' ';
	done = 1;
	stackhist = histline+1;
}

void downhistory() /**/
{
char *s;

	if (mult < 0) { mult = -mult; uphistory(); return; }
	histline += mult;
	if (!(s = qgetevent(histline)))
		{
		feep();
		histline -= mult;
		return;
		}
	sethistline(s);
}

static int histpos;

void historysearchbackward() /**/
{
int t0,ohistline = histline;
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	if (lastcmd & ZLE_HISTSEARCH) t0 = histpos;
	else for (t0 = 0; line[t0] && iword(line[t0]); t0++);
	histpos = t0;
	for (;;)
		{
		histline--;
		if (!(s = qgetevent(histline)))
			{
			feep();
			histline = ohistline;
			return;
			}
		if (!hstrncmp(s,line,t0) && hstrcmp(s,line))
			break;
		}
	sethistline(s);
}

void historysearchforward() /**/
{
int t0,ohistline = histline;
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	if (lastcmd & ZLE_HISTSEARCH) t0 = histpos;
	else for (t0 = 0; line[t0] && iword(line[t0]); t0++);
	histpos = t0;
	for (;;)
		{
		histline++;
		if (!(s = qgetevent(histline)))
			{
			feep();
			histline = ohistline;
			return;
			}
		if (!hstrncmp(s,line,t0) && hstrcmp(s,line))
			break;
		}
	sethistline(s);
}

void beginningofbufferorhistory() /**/
{
	if (findbol())
		cs = 0;
	else
		beginningofhistory();
}

void beginningofhistory() /**/
{
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	if (!(s = qgetevent(firsthist)))
		{
		feep();
		return;
		}
	histline = firsthist;
	sethistline(s);
}

void endofbufferorhistory() /**/
{
	if (findeol() != ll)
		cs = ll;
	else
		endofhistory();
}

void endofhistory() /**/
{
	if (histline == curhist)
		feep();
	else
		{
		histline = curhist;
		sethistline(curhistline);
		}
}

void insertlastword() /**/
{
char *s,*t;
int len,z = lithist;

	/* multiple calls will now search back through the history, pem */
	static char	*lastinsert;
	static int	lasthist, lastpos;
	int		evhist = curhist - 1;

	if (lastinsert) {
	    int len = strlen(lastinsert);
	    int pos = cs;
	    if (	lastpos <= pos &&
			len == pos - lastpos &&
			strncmp(lastinsert, &line[lastpos], len) == 0) {
		evhist = --lasthist;
		cs = lastpos;
		foredel(pos-cs);
	    }
	    free(lastinsert);
	    lastinsert = NULL;
	}
	lithist = 0;
	if (!(s = qgetevent(evhist), lithist = z, s))
		{
		feep();
		return;
		}
	for (t = s+strlen(s); t > s; t--)
		if (*t == HISTSPACE)
			break;
	if (t != s)
		t++;
	lasthist = evhist;
	lastpos = cs;
	lastinsert = ztrdup(t);
	spaceinline(len = strlen(t));
	strncpy(line+cs,t,len);
	cs += len;
}

char *qgetevent(ev) /**/
int ev;
{
	if (ev > curhist)
		return NULL;
	return ((ev == curhist) ? curhistline : quietgetevent(ev));
}

void pushline() /**/
{
	if (mult < 0) return;
	pushnode(bufstack,ztrdup(line));
	while (--mult)
		pushnode(bufstack,ztrdup(""));
	stackcs = cs;
	*line = '\0';
	ll = cs = 0;
}

void getline() /**/
{
char *s = getnode(bufstack);

	if (!s)
		feep();
	else
		{
		int cc;

		cc = strlen(s);
		spaceinline(cc);
		strncpy(line+cs,s,cc);
		cs += cc;
		free(s);
		}
}

void historyincrementalsearchbackward() /**/
{
	doisearch(-1);
}

void historyincrementalsearchforward() /**/
{
	doisearch(1);
}

void doisearch(dir) /**/
int dir;
{
char *s,*oldl;
char ibuf[256],*sbuf = ibuf+10;
int sbptr = 0,ch,ohl = histline,ocs = cs;
int nomatch = 0,chequiv = 0;

	strcpy(ibuf,"i-search: ");
	statusline = ibuf;
	oldl = ztrdup(line);
	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	for (;;)
		{
		nomatch = 0;
		if (sbptr > 1 || (sbptr == 1 && sbuf[0] != '^'))
			{
			int ohistline = histline;

			for (;;)
				{
				char *t;

				if (!(s = qgetevent(histline)))
					{
					feep();
					nomatch = 1;
					histline = ohistline;
					break;
					}
				if ((sbuf[0] == '^') ?
						(t = (hstrncmp(s,sbuf+1,sbptr-1)) ? NULL : s) :
						(t = hstrnstr(s,sbuf,sbptr)))
					if (!(chequiv && !hstrcmp(line,s)))
						{
						sethistline(s);
						cs = t-s+sbptr-(sbuf[0] == '^');
						break;
						}
				histline += dir;
				}
			chequiv = 0;
			}
		refresh();
		if ((ch = getkey(1)) == -1)
			break;
		if (ch == 22 || ch == 17) {
			if ((ch = getkey(1)) == -1)
				break;
		} else if (ch == 8 || ch == 127) {
			if (sbptr)
				sbuf[--sbptr] = '\0';
			else
				feep();
			histline = ohl;
			continue;
		} else if (ch == 7 || ch == 3) {
			setline(oldl);
			cs = ocs;
			histline = ohl;
			statusline = NULL;
			break;
		} else if (ch == 27)
			break;
		else if (ch == 10 || ch == 13) {
			ungetkey(ch);
			break;
		} else if (ch == 18) {
			ohl = (histline += (dir = -1));
			chequiv = 1;
			continue;
		} else if (ch == 19) {
			ohl = (histline += (dir = 1));
			chequiv = 1;
			continue;
		} else if (!(ch & 0x60)) {
			ungetkey(ch);
			break;
		}
		if (!nomatch && sbptr != 39 && !icntrl(ch)) {
			sbuf[sbptr++] = ch;
			sbuf[sbptr] = '\0';
		}
	}
	free(oldl);
	statusline = NULL;
}

void acceptandinfernexthistory() /**/
{
int t0;
char *s,*t;

	done = 1;
	for (t0 = histline-2;;t0--)
		{
		if (!(s = qgetevent(t0)))
			return;
		if (!hstrncmp(s,line,ll))
			break;
		}
	if (!(s = qgetevent(t0+1)))
		return;
	pushnode(bufstack,t = ztrdup(s));
	for (; *t; t++)
		if (*t == HISTSPACE)
			*t = ' ';
	stackhist = t0+1;
}

void infernexthistory() /**/
{
int t0;
char *s,*t;

	if (!(t = qgetevent(histline-1)))
		{
		feep();
		return;
		}
	for (t0 = histline-2;;t0--)
		{
		if (!(s = qgetevent(t0)))
			{
			feep();
			return;
			}
		if (!strcmp(s,t))
			break;
		}
	if (!(s = qgetevent(t0+1)))
		{
		feep();
		return;
		}
	histline = t0+1;
	sethistline(s);
}

void vifetchhistory() /**/
{
char *s;

	if (mult < 0) return;
	if (histline == curhist) {
		if (!(lastcmd & ZLE_ARG)) {
			cs = ll;
			cs = findbol();
			return;
		}
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
	}
	if (!(lastcmd & ZLE_ARG)) mult = curhist;
	if (!(s = qgetevent(mult)))
		feep();
	else {
		histline = mult;
		sethistline(s);
	}
}

int getvisrchstr() /**/
{
char sbuf[80];
int sptr = 1;

	if (visrchstr)
		{
		free(visrchstr);
		visrchstr = NULL;
		}
	statusline = sbuf;
	sbuf[0] = c;
	sbuf[1] = '\0';
	while (sptr)
		{
		refresh();
		c = getkey(0);
		if (c == '\r' || c == '\n' || c == '\033')
			{
			visrchstr = ztrdup(sbuf+1);
			return 1;
			}
		if (c == '\b')
			{
			sbuf[--sptr] = '\0';
			continue;
			}
		if (sptr != 79)
			{
			sbuf[sptr++] = c;
			sbuf[sptr] = '\0';
			}
		}
	return 0;
}

void vihistorysearchforward() /**/
{
	visrchsense = 1;
	if (getvisrchstr())
		virepeatsearch();
}

void vihistorysearchbackward() /**/
{
	visrchsense = -1;
	if (getvisrchstr())
		virepeatsearch();
}

void virepeatsearch() /**/
{
int ohistline = histline,t0;
char *s;

	if (!visrchstr)
		{
		feep();
		return;
		}
	t0 = strlen(visrchstr);
	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	for (;;)
		{
		histline += visrchsense;
		if (!(s = qgetevent(histline)))
			{
			feep();
			histline = ohistline;
			return;
			}
		if (!hstrcmp(line,s))
			continue;
		if (*visrchstr == '^')
			{
			if (!hstrncmp(s,visrchstr+1,t0-1))
				break;
			}
		else
			if (hstrnstr(s,visrchstr,t0))
				break;
		}
	sethistline(s);
}

void virevrepeatsearch() /**/
{
	visrchsense = -visrchsense;
	virepeatsearch();
	visrchsense = -visrchsense;
}

