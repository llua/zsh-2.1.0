/*

	hist.c - history expansion

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

#include "zsh.h"

static int lastc;

/* add a character to the current history word */

void hwaddc(c) /**/
int c;
{
	if (hlastw && hline && (!(errflag || lexstop) || c == HISTSPACE))
		{
		*hptr++ = c;
		if (hptr-hline >= hlinesz)
			{
			int ll,flag = 0;

			ll = hptr-hlastw;
			if (full(histlist) && getdata(lastnode(histlist)) == hline)
				flag = 1;
			hline = realloc(hline,hlinesz *= 4);
			if (flag)
				setdata(lastnode(histlist),hline);
			hptr = hline+(hlinesz/4);
			hlastw = hptr-ll;
			}
		}
}

#define habort() { errflag = lexstop = 1; return ' '; }

/* get a character after performing history substitution */

int hgetc() /**/
{
int c,ev,farg,larg,argc,marg = -1,cflag = 0,bflag = 0;
char buf[256],*ptr;
char *sline,*eline;

tailrec:
	c = hgetch();
	if (stophist || alstackind)
		{
		hwaddc(c);
		return c;
		}
	if (isfirstch && c == hatchar)
		{
		isfirstch = 0;
		hungetch(hatchar);
		hungets(":s");
		c = bangchar;
		goto hatskip;
		}
	if (c != ' ')
		isfirstch = 0;
	if (c == '\\')
		{
		int g = hgetch();
		
		if (g != bangchar)
			hungetch(g);
		else
			{
			hwaddc(bangchar);
			return bangchar;
			}
		}
	if (c != bangchar)
		{
		hwaddc(c);
		return c;
		}
hatskip:
	*hptr = '\0';
	if ((c = hgetch()) == '{')
		{
		bflag = cflag = 1;
		c = hgetch();
		}
	if (c == '\"')
		{
		stophist = 1;
		goto tailrec;
		}
	if (!cflag && inblank(c) || c == '=' || c == '(' || lexstop)
		{
		if (lexstop)
			lexstop = 0;
		else
			hungetch(c);
		hwaddc(bangchar);
		return bangchar;
		}
	cflag = 0;
	ptr = buf;

	/* get event number */

	if (c == '?')
		{
		for(;;)
			{
			c = hgetch();
			if (c == '?' || c == '\n' || lexstop)
				break;
			else
				*ptr++ = c;
			}
		if (c != '\n' && !lexstop)
			c = hgetch();
		*ptr = '\0';
		ev = hconsearch(hsubl = ztrdup(buf),&marg);
		if (ev == -1)
			{
			herrflush();
			zerr("no such event: %s",buf,0);
			habort();
			}
		}
	else
		{
		int t0;
 
		for (;;)
			{
			if (inblank(c) || c == ';' || c == ':' || c == '^' || c == '$' ||
					c == '*' || c == '%' || c == '}' || lexstop)
				break;
			if (ptr != buf) {
				if (c == '-') break;
				if (idigit(buf[0]) && !idigit(c)) break;
			}
			*ptr++ = c;
			if (c == '#' || c == bangchar)
				{
				c = hgetch();
				break;
				}
			c = hgetch();
			}
		*ptr = 0;
		if (!*buf)
			ev = defev;
		else if (t0 = atoi(buf))
			ev = (t0 < 0) ? curhist+t0 : t0;
		else if (*buf == bangchar)
			ev = curhist-1;
		else if (*buf == '#')
			ev = curhist;
		else if ((ev = hcomsearch(buf)) == -1)
			{
			zerr("event not found: %s",buf,0);
			while (c != '\n' && !lexstop)
				c = hgetch();
			habort();
			}
		}

	/* get the event */

	if (!(eline = getevent(defev = ev)))
		habort();

	/* extract the relevant arguments */

	argc = getargc(eline);
	if (c == ':')
		{
		cflag = 1;
		c = hgetch();
		}
	if (c == '*')
		{
		farg = 1;
		larg = argc;
		cflag = 0;
		}
	else
		{
		hungetch(c);
		larg = farg = getargspec(argc,marg);
		if (larg == -2)
			habort();
		if (farg != -1)
			cflag = 0;
		c = hgetch();
		if (c == '*')
			{
			cflag = 0;
			larg = argc;
			}
		else if (c == '-')
			{
			cflag = 0;
			larg = getargspec(argc,marg);
			if (larg == -2)
				habort();
			if (larg == -1)
				larg = argc-1;
			}
		else
			hungetch(c);
		}
	if (farg == -1)
		farg = 0;
	if (larg == -1)
		larg = argc;
	if (!(sline = getargs(eline,farg,larg)))
		habort();

	/* do the modifiers */

	for(;;)
		{
		c = (cflag) ? ':' : hgetch();
		cflag = 0;
		if (c == ':')
			{
			int gbal = 0;
		
			if ((c = hgetch()) == 'g')
				{
				gbal = 1;
				c = hgetch();
				}
			switch(c)
				{
				case 'p':
					histdone = HISTFLAG_DONE|HISTFLAG_NOEXEC;
					break;
				case 'h':
					if (!remtpath(&sline))
						{
						herrflush();
						zerr("modifier failed: h",NULL,0);
						habort();
						}
					break;
				case 'e':
					if (!rembutext(&sline))
						{
						herrflush();
						zerr("modifier failed: e",NULL,0);
						habort();
						}
					break;
				case 'r':
					if (!remtext(&sline))
						{
						herrflush();
						zerr("modifier failed: r",NULL,0);
						habort();
						}
					break;
				case 't':
					if (!remlpaths(&sline))
						{
						herrflush();
						zerr("modifier failed: t",NULL,0);
						habort();
						}
					break;
				case 's':
					{
					int del;
					char *ptr1,*ptr2;
				
					del = hgetch();
					ptr1 = hdynread2(del);
					if (!ptr1)
						habort();
					ptr2 = hdynread2(del);
					if (strlen(ptr1))
						{
						if (hsubl)
							free(hsubl);
						hsubl = ptr1;
						}
					if (hsubr)
						free(hsubr);
					hsubr = ptr2;
					}
				case '&':
					if (hsubl && hsubr)
						subst(&sline,hsubl,hsubr,gbal);
					else
						{
						herrflush();
						zerr("no previous substitution with &",NULL,0);
						habort();
						}
					break;
				case 'q':
					quote(&sline);
					break;
				case 'x':
					quotebreak(&sline);
					break;
				case 'l':
					downcase(&sline);
					break;
				case 'u':
					upcase(&sline);
					break;
				default:
					herrflush();
					zerr("illegal modifier: %c",NULL,c);
				habort();
				break;
			}
		}
	else
		{
		if (c != '}' || !bflag)
			hungetch(c);
		if (c != '}' && bflag)
			{
			zerr("'}' expected",NULL,0);
			habort();
			}
		break;
		}
	}

	/* stuff the resulting string in the input queue and start over */

	lexstop = 0;
	if (alstackind != MAXAL)
		{
		hungets(HISTMARK);
		alstack[alstackind++] = NULL;
		}
	hungets(sline);
	histdone |= HISTFLAG_DONE;
	if (isset(HISTVERIFY)) {
		histdone |= HISTFLAG_NOEXEC;
		zrecall = 1;
	}
	goto tailrec;
}

/* reset the alias stack for lexrestore () */

void clearalstack() /**/
{
Alias ix;

	while (alstackind)
		{
		ix = alstack[--alstackind];
		ix->inuse = 0;
		}
}

/* get a character without history expansion */

int hgetch() /**/
{
char *line,*pmpt,*pmpt2 = NULL;
int plen;

start:
	if (inbufct)
		{
		inbufct--;
		if ((lastc = *inbufptr++) == ALPOP)
			{
			Alias ix;
			char *t;

			if (!alstackind)
				{
				zerr("alias stack underflow",NULL,0);
				errflag = lexstop = 1;
				return lastc = ' ';
				}
			ix = alstack[--alstackind];
			if (ix)
				{
				ix->inuse = 0;
				t = ix->text;
				if (*t && t[strlen(t)-1] == ' ')
					alstat = ALSTAT_MORE;
				else
					alstat = ALSTAT_JUNK;
				}
			goto start;
			}
		if (itok(lastc))
			goto start;
		return lastc;
		}
	if (strin || errflag)
		{
		lexstop = 1;
		return lastc = ' ';
		}
	if (interact && isset(SHINSTDIN))
		if (!isfirstln)
			pmpt = putprompt(prompt2,&plen);
		else
			{
			int foo;

			pmpt = putprompt(prompt,&plen);
			pmpt2 = (rprompt) ? putprompt(rprompt,&foo) : NULL;
			}
	if (!(interact && isset(SHINSTDIN) && SHTTY != -1 && isset(USEZLE)))
		{
		if (interact && isset(SHINSTDIN))
			write(2,pmpt,strlen(pmpt));
		line = fgets(zalloc(256),256,bshin);
		}
	else
		line = zleread(pmpt,pmpt2,plen);
	if (!line)
		{
		lexstop = 1;
		return lastc = ' ';
		}
	if (errflag)
		{
		lexstop = errflag = 1;
		return lastc = ' ';
		}
	if (interact && isset(SHINSTDIN))
		{
		char *s = getdata(lastnode(lithistlist));

		if (!*s)
			{
			free(s);
			setdata(lastnode(lithistlist),ztrdup(line));
			}
		else
			{
			char *t = zalloc(strlen(s)+strlen(line)+3);

			strcpy(t,s);
			strcat(t,line);
			free(s);
			setdata(lastnode(lithistlist),t);
			}
		}
	if (isfirstln)
		spaceflag = *line == ' ';
	if (isset(VERBOSE))
		{
		fputs(line,stderr);
		fflush(stderr);
		}
	if (line[strlen(line)-1] == '\n')
		{
		lineno++;
		if (interact && isset(SUNKEYBOARDHACK) && isset(SHINSTDIN) && 
				SHTTY != -1 && *line && line[1] && line[strlen(line)-2] == '`')
			{
			int ct;
			char *ptr;

			for (ct = 0, ptr = line; *ptr; ptr++)
				if (*ptr == '`')
					ct++;
			if (ct & 1)
				{
				ptr[-2] = '\n';
				ptr[-1] = '\0';
				}
			}
		}
	isfirstch = 1;
	hungets(line);
	free(line);
	goto start;
}

/* put a string in the input queue */

void hungets(str) /**/
char *str;
{
int slen = strlen(str);

/* shrink inbuf if it gets too big */

	if (!inbufct && inbufsz > 65536)
		{
		free(inbuf);
		inbuf = zalloc(inbufsz = 256);
		inbufptr = inbuf+inbufsz;
		inbufct = 0;
		}
	if (slen+inbufct > inbufsz)
		{
		char *x;

		while (slen+inbufct > inbufsz)
			inbufsz *= 4;
		x = zalloc(inbufsz);
		memcpy(x+inbufsz-inbufct,inbufptr,inbufct);
		inbufptr = x+inbufsz-inbufct;
		free(inbuf);
		inbuf = x;
		}
	memcpy(inbufptr -= slen,str,slen);
	inbufct += slen;
}

/* unget a char and remove it from hline */

void hungetc(c) /**/
int c;
{
	if (lexstop)
		return;
	if (hlastw)
		{
		if (hlastw == hptr)
			zerr("hungetc attempted at buffer start",NULL,0);
		else
			hptr--;
		}
	hungetch(c);
}

void hungetch(c) /**/
int c;
{
	if (lexstop)
		return;
	if (inbufct == inbufsz)
		{
		hungets(" ");
		*inbufptr = c;
		}
	else
		{
		*--inbufptr = c;
		inbufct++;
		}
}

/* begin reading a string */

void strinbeg() /**/
{
	strin = 1;
	hbegin();
	lexinit();
}

/* done reading a string */

void strinend() /**/
{
	strin = 0;
	isfirstch = 1;
	histdone = 0;
	hend();
}

/* stuff a whole file into the input queue and print it */

int stuff(fn) /**/
char *fn;
{
FILE *in;
char *buf;
int len;

	if (!(in = fopen(fn,"r")))
		{
		zerr("can't open %s",fn,0);
		return 1;
		}
	fseek(in,0,2);
	len = ftell(in);
	fseek(in,0,0);
	buf = alloc(len+1);
	if (!(fread(buf,len,1,in)))
		{
		zerr("read error on %s",fn,0);
		fclose(in);
		free(buf);
		return 1;
		}
	fclose(in);
	buf[len] = '\0';
	fwrite(buf,len,1,stdout);
	hungets(buf);
	return 0;
}

/* flush input queue */

void hflush() /**/
{
	inbufptr += inbufct;
	inbufct = 0;
}

/* initialize the history mechanism */

void hbegin() /**/
{
	isfirstln = isfirstch = 1;
	histremmed = errflag = histdone = spaceflag = 0;
	stophist = isset(NOBANGHIST);
	lithist = isset(HISTLIT);
	hline = hptr = zalloc(hlinesz = 32);
	if (interact && isset(SHINSTDIN) && !strin) {
		inittty();
		defev = curhist++;
		while (curhist-firsthist >= histsiz) {
			free(getnode(histlist));
			firsthist++;
		}
		while (curhist-firstlithist >= lithistsiz) {
			free(getnode(lithistlist));
			firstlithist++;
		}
		permalloc();
		addnode(histlist,hline);
		addnode(lithistlist,ztrdup(""));
		heapalloc();
	} else
		histremmed = 1;
}

void inittty() /**/
{
	attachtty(mypgrp);
}

/* say we're done using the history mechanism */

int hend() /**/
{
int flag,save = 1;

	if (!hline)
		return 1;
	if (!interact || strin || unset(SHINSTDIN)) {
		free(hline);
		return 1;
	}
	flag = histdone;
	histdone = 0;
	if (hptr < hline+2)
		save = 0;
	else {
		char *s = getdata(lastnode(lithistlist));

		if (*s)
			s[strlen(s)-1] = '\0';
		hptr[-1] = '\0';
		if (hptr[-2] == '\n')
			if (hline[1])
				hptr[-3] = '\0';
			else
				save = 0;
		if (!strcmp(hline,"\n") ||
				(isset(HISTIGNOREDUPS) && firstnode(histlist) &&
				nextnode(firstnode(histlist)) &&
				!strcmp(hline,getdata(prevnode(lastnode(histlist))))) ||
				(isset(HISTIGNORESPACE) && spaceflag) )
			save = 0;
	}
	if (flag & HISTFLAG_DONE) {
		char *ptr,*p;

		p = ptr = ztrdup(hline);
		for (;*p;p++)
			if (*p == HISTSPACE)
				*p = ' ';
		fprintf(stderr,"%s\n",ptr);
		fflush(stderr);
		free(ptr);
	}
	if (!save) {
		free(hline);
		if (!histremmed) {
			remnode(histlist,lastnode(histlist));
			free(remnode(lithistlist,lastnode(lithistlist)));
			curhist--;
		}
	}
	hline = NULL;
	return !(flag & HISTFLAG_NOEXEC || errflag);
}

/* remove the current line from the history List */

void remhist() /**/
{
	if (!histremmed)
		{
		histremmed = 1;
		free(remnode(histlist,lastnode(histlist)));
		free(remnode(lithistlist,lastnode(lithistlist)));
		curhist--;
		}
}

/* begin a word */

void hwbegin() /**/
{
	hlastw = hptr;
}

/* add a word to the history List */

char *hwadd() /**/
{
char *ret = hlastw;

	if (hlastw && hline)
		{
		hwaddc(HISTSPACE);
		if (alstackind || strin)
			if (!(alstackind == 1 && !alstack[0]))
				hptr = hlastw;
		}
	if (alstat == ALSTAT_JUNK)
		alstat = 0;
	return ret;
}

/* get an argument specification */

int getargspec(argc,marg) /**/
int argc;int marg;
{
int c,ret = -1;
 
	if ((c = hgetch()) == '0')
		return 0;
	if (idigit(c))
		{
		ret = 0;
		while (idigit(c))
			{
			ret = ret*10+c-'0';
			c = hgetch();
			}
		hungetch(c);
		}
	else if (c == '^')
		ret = 1;
	else if (c == '$')
		ret = argc;
	else if (c == '%')
		{
		if (marg == -1)
			{
			herrflush();
			zerr("%% with no previous word matched",NULL,0);
			return -2;
			}
		ret = marg;
		}
	else
		hungetch(c);
	return ret;
}

/* do ?foo? search */

int hconsearch(str,marg) /**/
char *str;int *marg;
{
int t0,t1 = 0;
Lknode node;
char *s;
 
	if (curhist-firsthist < 1)
		return -1;
	for (t0 = curhist-1,node = prevnode(lastnode(histlist));
			t0 >= firsthist; t0--,node = prevnode(node))
		if (s = ztrstr(getdata(node),str))
			{
			while (s != (char *) getdata(node))
				if (*s-- == HISTSPACE)
					t1++;
			*marg = t1;
			return t0;
			}
	return -1;
}

/* do !foo search */

int hcomsearch(str) /**/
char *str;
{
int t0;
Lknode node;

	if (curhist-firsthist < 1)
		return -1;
	for (t0 = curhist-1,node = prevnode(lastnode(histlist)); t0 >= firsthist;
			t0--,node = prevnode(node))
		if (!strncmp(getdata(node),str,strlen(str)))
			return t0;
	return -1;
}

/* various utilities for : modifiers */

int remtpath(junkptr) /**/
char **junkptr;
{
char *str = *junkptr,*cut;
 
	if (cut = strrchr(str,'/')) {
		if (str != cut) *cut = '\0';
		else str[1] = '\0';
		return 1;
	}
	return 0;
}
 
int remtext(junkptr) /**/
char **junkptr;
{
char *str = *junkptr,*cut;
 
	if ((cut = strrchr(str,'.')) && cut != str)
		{
		*cut = '\0';
		return 1;
		}
	return 0;
}
 
int rembutext(junkptr) /**/
char **junkptr;
{
char *str = *junkptr,*cut;
 
	if ((cut = strrchr(str,'.')) && cut != str)
		{
		*junkptr = strdup(cut+1);  /* .xx or xx? */
		return 1;
		}
	return 0;
}
 
int remlpaths(junkptr) /**/
char **junkptr;
{
char *str = *junkptr,*cut;
 
	if (cut = strrchr(str,'/'))
		{
		*cut = '\0';
		*junkptr = strdup(cut+1);
		return 1;
		}
	return 0;
}

int makeuppercase(junkptr) /**/
char **junkptr;
{
char *str = *junkptr;

	for (; *str; str++)
		*str = tuupper(*str);
	return 1;
}

int makelowercase(junkptr) /**/
char **junkptr;
{
char *str = *junkptr;

	for (; *str; str++)
		*str = tulower(*str);
	return 1;
}

void subst(strptr,in,out,gbal) /**/
char **strptr;char *in;char *out;int gbal;
{
char *str = *strptr,*cut,*sptr;
int off;

	while (cut = (char *) ztrstr(str,in)) {
		*cut = '\0';
		cut += strlen(in);
		off = cut-*strptr;
		*strptr = tricat(*strptr,sptr = convamps(out,in),cut);
		if (gbal) {
			str = (char *) *strptr+off+strlen(sptr);
			continue;
		}
		break;
	}
}
 
char *convamps(out,in) /**/
char *out;char *in;
{
char *ptr,*ret,*pp;
int slen,inlen = strlen(in);
 
	for (ptr = out, slen = 0; *ptr; ptr++,slen++)
		if (*ptr == '\\')
			ptr++;
		else if (*ptr == '&')
			slen += inlen-1;
	ret = pp = alloc(slen+1);
	for (ptr = out; *ptr; ptr++)
		if (*ptr == '\\')
			*pp++ = *++ptr;
		else if (*ptr == '&')
			{
			strcpy(pp,in);
			pp += inlen;
			}
		else
			*pp++ = *ptr;
	*pp = '\0';
	return ret;
}

char *makehstr(s) /**/
char *s;
{
char *t;

	t = s = strdup(s);
	for (; *t; t++)
		if (*t == HISTSPACE)
			*t = ' ';
	return s;
}

char *quietgetevent(ev) /**/
int ev;
{
Lknode node;
 
	ev -= (lithist) ? firstlithist : firsthist;
	if (ev < 0)
		return NULL;
	for (node = firstnode((lithist) ? lithistlist : histlist);
			ev && node; incnode(node), ev--);
	if (!node)
		return NULL;
	return getdata(node);
}

char *getevent(ev) /**/
int ev;
{
Lknode node;
int oev = ev;
 
	ev -= firsthist;
	for (node = firstnode(histlist); ev && node; incnode(node), ev--);
	if (!node)
		{
		herrflush();
		zerr("no such event: %d",NULL,oev);
		return NULL;
		}
	return getdata(node);
}
 
int getargc(list) /**/
char *list;
{
int argc = 0;

	for (; *list; list++)
		if (*list == HISTSPACE)
			argc++;
	return argc;
}
 
char *getargs(elist,arg1,arg2) /**/
char *elist;int arg1;int arg2;
{
char *ret = elist,*retn;
int acnt = arg2-arg1+1;

	while (arg1--)
		while (*ret && *ret++ != HISTSPACE);
	if (!*ret)
		{
		herrflush();
		zerr("no such word in event",NULL,0);
		return NULL;
		}
	retn = ret = strdup(ret);
	while (acnt > 0)
		{
		while (*ret && *ret != HISTSPACE)
			ret++;
		if (*ret == HISTSPACE)
			*ret = ' ';
		else
			break;
		acnt--;
		}
	if (acnt > 1 && !*ret)
		{
		herrflush();
		zerr("no such word in event",NULL,0);
		return NULL;
		}
	*ret = '\0';
	return retn;
}

void upcase(x) /**/
char **x;
{
char *pp = *(char **) x;

	for (; *pp; pp++)
		*pp = tuupper(*pp);
}

void downcase(x) /**/
char **x;
{
char *pp = *(char **) x;

	for (; *pp; pp++)
		*pp = tulower(*pp);
}

int quote(tr) /**/
char **tr;
{
char *ptr,*rptr,**str = (char **) tr;
int len = 1;
 
	for (ptr = *str; *ptr; ptr++,len++)
		if (*ptr == '\'')
			len += 3;
	ptr = *str;
	*str = rptr = zalloc(len);
	for (ptr = *str; *ptr; )
		if (*ptr == '\'')
			{
			*rptr++ = '\''; *rptr++ = '\\'; *rptr++ = '\''; *rptr++ = '\'';
			ptr++;
			}
		else
			*rptr++ = *ptr++;
	return 0;
}
 
int quotebreak(tr) /**/
char **tr;
{
char *ptr,*rptr,**str = (char **) tr;
int len = 1;
 
	for (ptr = *str; *ptr; ptr++,len++)
		if (*ptr == '\'')
			len += 3;
		else if (inblank(*ptr))
			len += 2;
	ptr = *str;
	*str = rptr = zalloc(len);
	for (ptr = *str; *ptr; )
		if (*ptr == '\'')
			{
			*rptr++ = '\''; *rptr++ = '\\'; *rptr++ = '\''; *rptr++ = '\'';
			ptr++;
			}
		else if (inblank(*ptr))
			{
			*rptr++ = '\''; *rptr++ = *ptr++; *rptr++ = '\'';
			}
		else
			*rptr++ = *ptr++;
	return 0;
}

static char *bp;
static int lensb,countp;

void stradd(d) /**/
char *d;
{
	while (*bp++ = *d++);
	bp--;
}

int putstr(d) /**/
int d;
{
	*bp++ = d;
	if (countp)
		lensb++;
	return 0;
}

#define tstradd(X) \
	if (termok && unset(SINGLELINEZLE)) { \
		char tbuf[2048],*tptr = tbuf; \
		if (tgetstr(X,&tptr)) \
			tputs(tbuf,1,putstr); \
	} \
	break

/* get a prompt string */

char *putprompt(fm,lenp) /**/
char *fm;int *lenp;
{
char *ss,*ttyname DCLPROTO((int)),*bl0;
static char buf1[256],buf2[256],*buf;
char buf3[MAXPATHLEN];
int t0,bracepos = 0;
struct tm *tm = NULL;
time_t timet;

	lensb = 0; countp = 1;
	if (!fm)
		{
		*lenp = 0;
		return "";
		}
	/* kludge alert! */
	buf = (buf == buf1) ? buf2 : buf1;
	bp = bl0 = buf;
	if (!columns)
		columns = 80;
	clearerr(stdin);
	for(;*fm;fm++)
		{
		if (bp-buf >= 220)
			break;
		if (*fm == '%')
			switch (*++fm)
				{
				case '~':
					t0 = finddir(cwd);
					if (t0 != -1) {
						*bp++ = '~';
						stradd(usernames[t0]);
						stradd(cwd+strlen(userdirs[t0]));
						break;
					}
					if (!strncmp(cwd,home,t0 = strlen(home)) && t0 > 1) {
						*bp++ = '~';
						stradd(cwd+t0);
						break;
					}
				case 'd': case '/': stradd(cwd); break;
				case 'c': case '.':
					t0 = finddir(cwd);
					if (t0 != -1) {
						sprintf(buf3,"~%s%s",usernames[t0],
							cwd+strlen(userdirs[t0]));
					} else if (!strncmp(cwd,home,t0 = strlen(home)) && t0 > 1) {
						sprintf(buf3,"~%s",cwd+t0);
					} else {
						strcpy(buf3,cwd);
					}
					t0 = 1;
					if (idigit(fm[1])) { t0 = fm[1]-'0'; fm++; }
					for (ss = buf3+strlen(buf3); ss > buf3; ss--)
						if (*ss == '/' && !--t0) {
							ss++;
							break;
						}
					if (*ss == '/' && ss[1]) ss++;
					stradd(ss);
					break;
				case 'C':
					strcpy(buf3,cwd);
					t0 = 1;
					if (idigit(fm[1])) { t0 = fm[1]-'0'; fm++; }
					for (ss = buf3+strlen(buf3); ss > buf3; ss--)
						if (*ss == '/' && !--t0) {
							ss++;
							break;
						}
					if (*ss == '/' && ss[1]) ss++;
					stradd(ss);
					break;
				case 'h': case '!':
					sprintf(bp,"%d",curhist);
					bp += strlen(bp);
					break;
				case 'M': stradd(hostnam); break;
				case 'm':
					if (idigit(fm[1]))
						t0 = (*++fm)-'0';
					else
						t0 = 1;
					for (ss = hostnam; *ss; ss++)
						if (*ss == '.' && !--t0)
							break;
					t0 = *ss;
					*ss = '\0';
					stradd(hostnam);
					*ss = t0;
					break;
				case 'S': tstradd("so"); /* <- this is a macro */
				case 's': tstradd("se");
				case 'B': tstradd("md");
				case 'b': tstradd("me");
				case 'U': tstradd("us");
				case 'u': tstradd("ue");
				case '{': bracepos = bp-buf; countp = 0; break;
				case '}': lensb += (bp-buf)-bracepos; countp = 1; break;
				case 't': case '@':
					timet = time(NULL);
					tm = localtime(&timet);
					ztrftime(bp,16,"%l:%M%p",tm);
					if (*bp == ' ')
						chuck(bp);
					bp += strlen(bp);
					break;
				case 'T':
					timet = time(NULL);
					tm = localtime(&timet);
					ztrftime(bp,16,"%k:%M",tm);
					bp += strlen(bp);
					break;
				case '*':
					timet = time(NULL);
					tm = localtime(&timet);
					ztrftime(bp,16,"%k:%M:%S",tm);
					bp += strlen(bp);
					break;
				case 'n': stradd(username); break;
				case 'w':
					timet = time(NULL);
					tm = localtime(&timet);
					ztrftime(bp,16,"%a %e",tm);
					bp += strlen(bp);
					break;
				case 'W':
					timet = time(NULL);
					tm = localtime(&timet);
					ztrftime(bp,16,"%m/%d/%y",tm);
					bp += strlen(bp);
					break;
				case 'D':
					timet = time(NULL);
					tm = localtime(&timet);
					ztrftime(bp,16,"%y-%m-%d",tm);
					bp += strlen(bp);
					break;
				case 'l':
					if (ss = ttyname(SHTTY))
						stradd((strncmp(ss,"/dev/tty",8) ? ss : ss+8));
					else
						stradd("()");
					break;
				case '?':
					sprintf(bp,"%d",lastval);
					bp += strlen(bp);
					break;
				case '%': *bp++ = '%'; break;
				case '#': *bp++ = (geteuid()) ? '%' : '#'; break;
				default: *bp++ = '%'; *bp++ = *fm; break;
				}
		else if (*fm == '!')
			{
			sprintf(bp,"%d",curhist);
			bp += strlen(bp);
			}
		else
			{
			if (fm[0] == '\\' && fm[1])
				fm++;
			if ((*bp++ = *fm) == '\n')
				bl0 = bp;
			}
		}
	*lenp = (bp-bl0)-lensb;
	*lenp %= columns;
	if (*lenp == columns-1)
		{
		*lenp = 0;
		*bp++ = ' ';
		}
	*bp = '\0';
	return buf;
}

void herrflush() /**/
{
	if (strin)
		hflush();
	else while (lastc != '\n' && !lexstop)
		hgetch();
}

/* read an arbitrary amount of data into a buffer until stop is found */

char *hdynread(stop) /**/
int stop;
{
int bsiz = 256,ct = 0,c;
char *buf = zalloc(bsiz),*ptr;
 
	ptr = buf;
	while ((c = hgetch()) != stop && c != '\n' && !lexstop)
		{
		if (c == '\\')
			c = hgetch();
		*ptr++ = c;
		if (++ct == bsiz)
			{
			buf = realloc(buf,bsiz *= 2);
			ptr = buf+ct;
			}
		}
	*ptr = 0;
	if (c == '\n')
		{
		hungetch('\n');
		zerr("delimiter expected",NULL,0);
		free(buf);
		return NULL;
		}
	return buf;
}
 
char *hdynread2(stop) /**/
int stop;
{
int bsiz = 256,ct = 0,c;
char *buf = zalloc(bsiz),*ptr;
 
	ptr = buf;
	while ((c = hgetch()) != stop && c != '\n' && !lexstop)
		{
		if (c == '\n')
			{
			hungetch(c);
			break;
			}
		if (c == '\\')
			c = hgetch();
		*ptr++ = c;
		if (++ct == bsiz)
			{
			buf = realloc(buf,bsiz *= 2);
			ptr = buf+ct;
			}
		}
	*ptr = 0;
	if (c == '\n')
		hungetch('\n');
	return buf;
}

/* set cbreak mode, or the equivalent */

void setcbreak() /**/
{
struct ttyinfo ti;

	ti = shttyinfo;
#ifdef TERMIOS
	ti.termios.c_lflag &= ~ICANON;
	ti.termios.c_cc[VMIN] = 1;
	ti.termios.c_cc[VTIME] = 0;
#else
#ifdef TERMIO
	ti.termio.c_lflag &= ~ICANON;
	ti.termio.c_cc[VMIN] = 1;
	ti.termio.c_cc[VTIME] = 0;
#else
	ti.sgttyb.sg_flags |= CBREAK;
#endif
#endif
	settyinfo(&ti);
}

int getlineleng() /**/
{
int z;

#ifdef TIOCSWINSZ
	z = shttyinfo.winsize.ws_col;
	return (z) ? z : 80;
#else
	return 80;
#endif
}

void unsetcbreak() /**/
{
	settyinfo(&shttyinfo);
}

/* give the tty to some process */

void attachtty(pgrp) /**/
long pgrp;
{
static int ep = 0;
int arg = pgrp;

	if (jobbing)
#ifndef TIOCSPGRP
		if (SHTTY != -1 && tcsetpgrp(SHTTY,pgrp) == -1 && !ep)
#else
		if (SHTTY != -1 && ioctl(SHTTY,TIOCSPGRP,&arg) == -1 && !ep)
#endif
			{
			zerr("can't set tty pgrp: %e",NULL,errno);
			fflush(stderr);
			opts[MONITOR] = OPT_UNSET;
			ep =1;
			errflag = 0;
			}
}

