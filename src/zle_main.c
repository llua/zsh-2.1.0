/*

	zle_main.c - main routines for line editor

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

#define ZLEGLOBALS
#define ZLE
#include "zsh.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

static Key cky;

/* set up terminal */

void setterm() /**/
{
struct ttyinfo ti;
long val;

#ifdef CLOBBERS_TYPEAHEAD
#ifdef FIONREAD
	ioctl(SHTTY, FIONREAD, &val);
	if (val) return;
#endif
#endif
	inittty();
	ti = shttyinfo;
#ifdef TERMIOS
	ti.termios.c_lflag &= ~(ICANON|ECHO);
	ti.termios.c_cc[VQUIT] =
#ifdef VDISCARD
		ti.termios.c_cc[VDISCARD] = 
#endif
#ifdef VSUSP
		ti.termios.c_cc[VSUSP] =
#endif
#ifdef VDSUSP
		ti.termios.c_cc[VDSUSP] =
#endif
		0;
	ti.termios.c_cc[VMIN] = 1;
	ti.termios.c_cc[VTIME] = 0;
	ti.termios.c_iflag &= ~(INLCR|ICRNL);
#else
#ifdef TERMIO
	ti.termio.c_lflag &= ~(ICANON|ECHO);
	ti.termio.c_cc[VQUIT] =
		0;
	ti.termio.c_cc[VMIN] = 1;
	ti.termio.c_cc[VTIME] = 0;
	ti.termio.c_iflag &= ~(INLCR|ICRNL);
#else
	ti.sgttyb.sg_flags = (ti.sgttyb.sg_flags | CBREAK) & ~ECHO;
	ti.tchars.t_quitc =
		ti.ltchars.t_suspc =
		ti.ltchars.t_dsuspc = ti.ltchars.t_lnextc = -1;
#endif
#endif
#ifdef TTY_NEEDS_DRAINING
	drainoutput();
#endif
	settyinfo(&ti);
}

void unsetterm() /**/
{
	settyinfo(&shttyinfo);
}

static char *kungetbuf;
static int kungetct,kungetsz;

void ungetkey(ch) /**/
int ch;
{
	if (kungetct == kungetsz)
		kungetbuf = realloc(kungetbuf,kungetsz *= 2);
	kungetbuf[kungetct++] = ch;
}

void ungetkeys(s,len) /**/
char *s;int len;
{
	s += len;
	while (len--)
		ungetkey(*--s);
}

unsigned int getkey(tmok) /**/
int tmok;
{
char cc;
unsigned int ret;

	if (kungetct)
		ret = (unsigned int) (unsigned char) kungetbuf[--kungetct];
	else {
		while (read(0,&cc,1) != 1)
			if (errno == EINTR) {
				if (!errflag)
					continue;
				errflag = 0;
				if (tmok)
					return -1;
				return 3;
			} else if (errno == EWOULDBLOCK) {
				fcntl(0,F_SETFL,0);
			} else {
				zerr("error on TTY read: %e",NULL,errno);
				exit(1);
			}
		ret = (unsigned int) (unsigned char) cc;
	}
	if (vichgflag) {
		if (vichgbufptr == vichgbufsz)
			vichgbuf = realloc(vichgbuf,vichgbufsz *= 2);
		vichgbuf[vichgbufptr++] = ret;
	}
	return ret;
}

/* read a line */

char *zleread(ppt,ppt2,plen) /**/
char *ppt;char *ppt2;int plen;
{
int z;
long costmult;
struct timeval tv;
fd_set foofd;
char *s;

	tv.tv_sec = 0;
	fflush(stdout);
	fflush(stderr);
	intr();
	costmult = 3840000L/((baud) ? baud : 2400);
	insmode = 1; eofsent = 0; resetneeded =0 ;
	pmpt = ppt;
	pmpt2 = ppt2;
	permalloc();
	histline = curhist;
	pptlen = plen;
	resetneeded = 1;
	FD_ZERO(&foofd);
	zleactive = 1;
	undoing = 1;
	line = zalloc(linesz = 256);
	*line = '\0';
	virangeflag = lastcmd = done = cs = ll = mark = 0;
	curhistline = NULL;
	mult = 1;
	vibufspec = 0;
	bindtab = mainbindtab;
	vichgflag = 0;
	viinsbegin = 0;
	statusline = NULL;
	if (s = getnode(bufstack))
		{
		setline(s);
		free(s);
		if (stackcs != -1)
			{
			cs = stackcs;
			stackcs = -1;
			if (cs > ll)
				cs = ll;
			}
		if (stackhist != -1)
			{
			histline = stackhist;
			stackhist = -1;
			}
		}
	initundo();
	if (unset(NOPROMPTCLOBBER))
		putchar('\r');
	if (tmout)
		alarm(tmout);
	if (zrecall) {
		zrecall = 0;
		uphistory();
	}
	refresh();
	errflag = 0;
	while (!done && !errflag)
		{
		struct zlecmd *zc;
		
		statusline = NULL;
		bindk = getkeycmd();
		if (c == 4 && !ll)
			{
			eofsent = 1;
			break;
			}
		if (bindk != -1)
			{
			zc = zlecmds+bindk;
			if (!(lastcmd & ZLE_ARG))
				mult = 1;
			if ((lastcmd & ZLE_UNDO) != (zc->flags & ZLE_UNDO) && undoing)
				addundo();
			if (menucmp && !(zc->flags & ZLE_MENUCMP))
				freemenu();
			if (zc->func)
				(zc->func)();
			lastcmd = zc->flags;
			if (!(lastcmd & ZLE_UNDO)) addundo();
			}
		else
			{
			errflag = 1;
			break;
			}
		FD_SET(0,&foofd);
		if ((tv.tv_usec = cost*costmult) > 500000)
			tv.tv_usec = 500000;
		if (!kungetct
#ifdef HAS_SELECT
			&& !select(1,&foofd,NULL,NULL,&tv)
#endif
			)
			refresh();
		}
	if (menucmp)
		freemenu();
	statusline = NULL;
	trashzle();
	alarm(0);
	z = strlen(line);
	line[z] = '\n';
	line[z+1] = 0;
	heapalloc();
	if (curhistline)
		free(curhistline);
	if (eofsent)
		{
		free(line);
		line = NULL;
		}
	zleactive = 0;
	freeundo();
	return line;
}

int getkeycmd() /**/
{
char buf[10];
int t0,ret;
Key ky;

	t0 = 1;
	cky = NULL;
	if ((c = getkey(1)) == -1)
		return -1;
	if ((ret = bindtab[c]) == z_sequenceleadin)
		{
		buf[0] = (c) ? c : 0x80;
		for (;;)
			{
			c = getkey(0);
			buf[t0++] = (c) ? c : 0x80;
			buf[t0] = '\0';
			if (!(ky = (Key) gethnode(buf,xbindtab)))
				return z_undefinedkey;
			if (ky->func != z_sequenceleadin)
				{
				cky = ky;
				ret = ky->func;
				break;
				}
			}
		}
	if (ret == z_vidigitorbeginningofline)
		ret = (lastcmd & ZLE_ARG) ? z_digitargument : z_beginningofline;
	else if (ret == z_executenamedcmd)
		ret = executenamedcommand();
	else if (ret == z_executelastnamedcmd)
		ret = lastnamed;
	return ret;
}

void sendstring() /**/
{
char buf[2];

	buf[0] = c;
	buf[1] = '\0';
	if (!cky)
		cky = (Key) gethnode(buf,xbindtab);
	ungetkeys(cky->str,cky->len);
}

Key makefunckey(fun) /**/
int fun;
{
Key ky = zcalloc(sizeof *ky);

	ky->func = fun;
	return ky;
}

/* initialize the bindings */

void initxbindtab() /**/
{
int t0,vi = 0;
char buf[3],*s;

	lastnamed = z_undefinedkey;
	if (s = zgetenv("VISUAL"))
		{
		if (ztrstr(s,"vi"))
			vi = 1;
		}
	else if ((s = zgetenv("EDITOR")) && ztrstr(s,"vi"))
		vi = 1;
	if (vi)
		{
		for (t0 = 0; t0 != 32; t0++)
			mainbindtab[t0] = viinsbind[t0];
		for (t0 = 32; t0 != 256; t0++)
			mainbindtab[t0] = z_selfinsert;
		mainbindtab[127] = z_backwarddeletechar;
		}
	else
		{
		for (t0 = 0; t0 != 128; t0++)
			mainbindtab[t0] = emacsbind[t0];
		for (t0 = 128; t0 != 256; t0++)
			mainbindtab[t0] = z_selfinsert;
		}
	for (t0 = 0200; t0 != 0240; t0++)
		mainbindtab[t0] = z_undefinedkey;
	for (t0 = 0; t0 != 128; t0++)
		altbindtab[t0] = vicmdbind[t0];
	for (t0 = 128; t0 != 256; t0++)
		altbindtab[t0] = emacsbind[t0];
	bindtab = mainbindtab;
	kungetbuf = zalloc(kungetsz = 32);
	kungetct = 0;
	xbindtab = newhtable(67);
	addhperm("\33\133C",makefunckey(z_forwardchar),xbindtab,NULL);
	addhperm("\33\133D",makefunckey(z_backwardchar),xbindtab,NULL);
	addhperm("\33\133A",makefunckey(z_uplineorhistory),xbindtab,NULL);
	addhperm("\33\133B",makefunckey(z_downlineorhistory),xbindtab,NULL);
	addhperm("\30*",makefunckey(z_expandword),xbindtab,NULL);
	addhperm("\30g",makefunckey(z_listexpand),xbindtab,NULL);
	addhperm("\30G",makefunckey(z_listexpand),xbindtab,NULL);
	addhperm("\30\16",makefunckey(z_infernexthistory),xbindtab,NULL);
	addhperm("\30\13",makefunckey(z_killbuffer),xbindtab,NULL);
	addhperm("\30\6",makefunckey(z_vifindnextchar),xbindtab,NULL);
	addhperm("\30\17",makefunckey(z_overwritemode),xbindtab,NULL);
	addhperm("\30\25",makefunckey(z_undo),xbindtab,NULL);
	addhperm("\30\26",makefunckey(z_vicmdmode),xbindtab,NULL);
	addhperm("\30\12",makefunckey(z_vijoin),xbindtab,NULL);
	addhperm("\30\2",makefunckey(z_vimatchbracket),xbindtab,NULL);
	addhperm("\30s",makefunckey(z_historyincrementalsearchforward),
		xbindtab,NULL);
	addhperm("\30r",makefunckey(z_historyincrementalsearchbackward),
		xbindtab,NULL);
	addhperm("\30u",makefunckey(z_undo),xbindtab,NULL);
	addhperm("\30\30",makefunckey(z_exchangepointandmark),
		xbindtab,NULL);
	addhperm("run-help",mkanode(ztrdup("man"),1),aliastab,NULL);
	addhperm("which-command",mkanode(ztrdup("whence"),1),aliastab,NULL);
	strcpy(buf,"\33q");
	if (!vi)
		for (t0 = 128; t0 != 256; t0++)
			if (emacsbind[t0] != z_undefinedkey) {
				buf[1] = t0 & 0x7f;
				addhnode(ztrdup(buf),makefunckey(emacsbind[t0]),xbindtab,NULL);
			}
	for (t0 = 0; t0 != 36; t0++)
		vibuf[t0] = NULL;
	for (t0 = 0; t0 != 26; t0++)
		vimarkline[t0] = 0;
	stackhist = stackcs = -1;
	vichgbufsz = 0;
	vichgbuf = NULL;
}

char *getkeystring(s,len) /**/
char *s;int *len;
{
static char buf[512];
char *t = buf;
int x,first = 1,metanext = 0;

	for (;*s;s++)
		{
		if (*s == '\\' && s[1])
			switch(*++s)
				{
				case 'a': *t++ = '\07'; break;
				case 'n': *t++ = '\n'; break;
				case 'b': *t++ = '\010'; break;
				case 't': *t++ = '\t'; break;
				case 'v': *t++ = '\v'; break;
				case 'f': *t++ = '\f'; break;
				case 'r': *t++ = '\r'; break;
				case 'e': *t++ = '\033'; break;
				case 'M':
					if (s[1] == '-')
						s++;
					metanext = 2;
					break;
				default:
					if (idigit(*s))
						{
						for (x = 0; idigit(*s); s++)
							x = x*8+(*s-'0');
						s--;
						*t++ = x;
						}
					else
						*t++ = *s;
					break;
				}
		else if (*s == '^')
			if (*++s == '?')
				*t++ = 0x7f;
			else
				*t++ = *s & 0x9f;
		else
			*t++ = *s;
		if (metanext && !(--metanext))
			{
			t[-1] |= 0x80;
			metanext = 0;
			}
		if (t > buf+500)
			break;
		first = 0;
		}
	*t = '\0';
	*len = t-buf;
	return buf;
}

void printbind(s,len) /**/
char *s;int len;
{
int ch;

	while (len--)
		{
		ch = (unsigned char) *s++;
		if (ch & 0x80)
			{
			printf("\\M-");
			ch &= 0x7f;
			}
		if (icntrl(ch))
			switch(ch)
				{
				case 0x7f: printf("^?"); break;
				default: printf("^%c",(ch|0x40)); break;
				}
		else
			putchar(ch);
		}
}

void printbinding(str,k) /**/
char *str;Key k;
{
	if (k->func == z_sequenceleadin)
		return;
	putchar('\"');
	printbind(str,strlen(str));
	printf("\"\t");
	if (k->func == z_sendstring)
		{
		putchar('\"');
		printbind(k->str,k->len);
		printf("\"\n");
		}
	else
		printf("%s\n",zlecmds[k->func].name);
}

int bin_bindkey(name,argv,ops,junc) /**/
char *name;char **argv;char *ops;int junc;
{
int t0,len;
char *s;
int func,*tab;

	tab = (ops['a']) ? altbindtab : mainbindtab;
	if (ops['v'] || ops['e'] || ops['d'])
		{
		if (*argv)
			{
			zerrnam(name,"too many arguments",NULL,0);
			return 1;
			}
		if (ops['d'] || ops['e'])
			if (ops['m'])
				for (t0 = 0; t0 != 256; t0++)
					tab[t0] = emacsbind[t0];
			else
				{
				for (t0 = 0; t0 != 128; t0++)
					tab[t0] = emacsbind[t0];
				for (t0 = 128; t0 != 256; t0++)
					tab[t0] = z_selfinsert;
				}
		else
			{
			for (t0 = 0; t0 != 32; t0++)
				mainbindtab[t0] = viinsbind[t0];
			for (t0 = 32; t0 != 256; t0++)
				mainbindtab[t0] = z_selfinsert;
			mainbindtab[127] = z_backwarddeletechar;
			}
		for (t0 = 0; t0 != 128; t0++)
			altbindtab[t0] = vicmdbind[t0];
		for (t0 = 128; t0 != 256; t0++)
			altbindtab[t0] = emacsbind[t0];
		for (t0 = 0200; t0 != 0240; t0++)
			tab[t0] = z_undefinedkey;
		return 0;
		}
	if (!*argv)
		{
		char buf[2];
		
		buf[0] = 'x'; buf[1] = '\0';
		for (t0 = 0; t0 != 256; t0++)
			{
			buf[0] = t0;
			putchar('\"');
			printbind(buf,1);
			if (t0 < 254 && tab[t0] == tab[t0+1] && tab[t0] == tab[t0+2])
				{
				printf("\" to \"");
				while (tab[t0] == tab[t0+1]) t0++;
				buf[0] = t0;
				printbind(buf,1);
				}
			printf("\"\t%s\n",zlecmds[tab[t0]].name);
			}
		listhtable(xbindtab,(HFunc) printbinding);
		return 0;
		}
	while (*argv)
		{
		s = getkeystring(*argv++,&len);
		if (len > 8)
			{
			zerrnam(name,"in-string too long",NULL,0);
			return 1;
			}
		if (!*argv || ops['r'])
			{
			Key ky;

			ky = gethnode(s,xbindtab);
			if (len == 1)
				func = tab[(unsigned char) *s];
			else
				func = (ky) ? ky->func : z_undefinedkey;
			if (func == z_undefinedkey)
				{
				zerrnam(name,"in-string is not bound",NULL,0);
				return 1;
				}
			if (ops['r'])
				{
				if (len == 1)
					tab[(unsigned char) *s] = z_undefinedkey;
				else
					{
					while (strlen(s) > 1)
						{
						free(remhnode(s,xbindtab));
						s[strlen(s)-1] = '\0';
						}
					}
				continue;
				}
			if (func == z_sendstring)
				{
				printbind(ky->str,ky->len);
				putchar('\n');
				return 0;
				}
			printf("%s\n",zlecmds[func].name);
			return 0;
			}
		if (!ops['s'])
			{
			for (t0 = 0; t0 != ZLECMDCOUNT; t0++)
				if (!strcmp(*argv,zlecmds[t0].name))
					break;
			if (t0 == ZLECMDCOUNT)
				{
				zerr("undefined function: %s",*argv,0);
				return 1;
				}
			func = t0;
			}
		else
			func = z_sendstring;
		if (len == 1)
			{
			Key ky;

			tab[(unsigned char) *s] = (ops['s']) ? z_sendstring : t0;
			if (ops['s'])
				{
				addhnode(ztrdup(s),ky = makefunckey(z_sendstring),xbindtab,freekey);
				ky->str = ztrdup(getkeystring(*argv,&ky->len));
				}
			}
		else
			{
			int t1;
			Key ky;

			if (tab[(unsigned char) *s] != z_undefinedkey &&
					tab[(unsigned char) *s] != z_sequenceleadin)
				{
				zerrnam(name,"in-string has already bound prefix",NULL,0);
				return 1;
				}
			tab[(unsigned char) *s] = z_sequenceleadin;
			if (!s[1])
				s[1] = 0x80;
			for (t1 = 1; t1 != len-1; t1++)
				{
				char sav;

				sav = s[t1+1];
				s[t1+1] = '\0';
				ky = gethnode(s,xbindtab);
				if (ky && ky->func != z_sequenceleadin)
					{
					zerrnam(name,"in-string has already bound prefix",NULL,0);
					return 1;
					}
				if (!ky)
					addhnode(ztrdup(s),makefunckey(z_sequenceleadin),xbindtab,
						freekey);
				if (!sav)
					sav = 0x80;
				s[t1+1] = sav;
				}
			addhnode(ztrdup(s),ky = makefunckey(func),xbindtab,freekey);
			if (ops['s'])
				ky->str = ztrdup(getkeystring(*argv,&ky->len));
			}
		argv++;
		}
	return 0;
}

void freekey(x) /**/
vptr x;
{
Key k = x;

	if (k->str)
		free(k->str);
	free(k);
}

/* this is mostly stolen from bash's draino() */

void drainoutput() /**/
{
int n = 0;

	if (!baud) return;
#ifdef TIOCOUTQ
#ifdef HAS_SELECT
	while ((ioctl(SHTTY,TIOCOUTQ,&n) >= 0) && n) {
		struct timeval tv;
		tv.tv_sec = n/baud;
		tv.tv_usec = ((n%baud)*1000000)/baud;
		select (0,(fd_set *)0,(fd_set *)0,(fd_set *)0,&tv);
	}
#endif
#endif
}

