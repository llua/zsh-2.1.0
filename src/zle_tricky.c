/*

	zle_tricky.c - expansion and completion

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
#ifdef __hpux
#include <ndir.h>
#else
#ifdef SYSV
#define direct dirent
#else
#include <sys/dir.h>
#endif
#endif
#include	<pwd.h>

static int we,wb,usemenu,useglob;

static int menub,menue,menuw;
static Lklist menulist;
static Lknode menunode;

#define inststr(X) inststrlen((X),-1)

int usetab() /**/
{
char *s = line+cs-1;

	for (; s >= line && *s != '\n'; s--)
		if (*s != '\t' && *s != ' ')
			return 0;
	return 1;
}

#define COMP_COMPLETE 0
#define COMP_LIST_COMPLETE 1
#define COMP_SPELL 2
#define COMP_EXPAND 3
#define COMP_EXPAND_COMPLETE 4
#define COMP_LIST_EXPAND 5
#define COMP_ISEXPAND(X) ((X) >= COMP_EXPAND)

void completeword() /**/
{
	usemenu = isset(MENUCOMPLETE) || (useglob = isset(GLOBCOMPLETE));
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_COMPLETE);
}

void menucompleteword() /**/
{
	usemenu = 1; useglob = isset(GLOBCOMPLETE);
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_COMPLETE);
}

void listchoices() /**/
{
	usemenu = isset(MENUCOMPLETE) || (useglob = isset(GLOBCOMPLETE));
	docomplete(COMP_LIST_COMPLETE);
}

void spellword() /**/
{
	usemenu = useglob = 0;
	docomplete(COMP_SPELL);
}

void deletecharorlist() /**/
{
	usemenu = isset(MENUCOMPLETE) || (useglob = isset(GLOBCOMPLETE));
	if (cs != ll)
		deletechar();
	else
		docomplete(COMP_LIST_COMPLETE);
}

void expandword() /**/
{
	usemenu = useglob = 0;
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_EXPAND);
}

void expandorcomplete() /**/
{
	usemenu = isset(MENUCOMPLETE) || (useglob = isset(GLOBCOMPLETE));
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_EXPAND_COMPLETE);
}

void menuexpandorcomplete() /**/
{
	usemenu = 1; useglob = isset(GLOBCOMPLETE);
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_EXPAND_COMPLETE);
}

void listexpand() /**/
{
	usemenu = isset(MENUCOMPLETE); useglob = isset(GLOBCOMPLETE);
	docomplete(COMP_LIST_EXPAND);
}

void reversemenucomplete() /**/
{
char *s;

	if (!menucmp)
		menucompleteword();	/* better than just feep'ing, pem */
	if (!menucmp) return;
	cs = menub;
	foredel(menue-menub);
	if (menunode == firstnode(menulist))
		menunode = lastnode(menulist);
	else
		menunode = prevnode(menunode);
	inststr(s = menunode->dat);
	menue = cs;
}

/*
 * Accepts the current completion and starts a new arg,
 * with the next completions. This gives you a way to accept
 * several selections from the list of matches.
 */
void acceptandmenucomplete() /**/
{
int t0,t1;

	if (!menucmp) {
		feep();
		return;
	}
	spaceinline(1);
	line[cs++] = ' ';
	spaceinline(menub-menuw);
	t1 = cs;
	for (t0 = menuw; t0 != menub; t0++)
		line[cs++] = line[t0];
	menue = menub = cs;
	menuw = t1;
	menucompleteword();
}

static char *lastmenu = NULL;
static int lastmenupos = -1;
static int lincmd,lastambig;
static char *cmdstr;

void docomplete(lst) /**/
int lst;
{
char *s;

	if (isset(AUTOMENU) && !menucmp && c == '\t' &&
		(lastcmd & ZLE_MENUCMP) && lastambig) usemenu = 1;
	if (menucmp) { do_menucmp(lst); return; }
	if (doexpandhist()) return;
	s = get_comp_string();
	if (s) {
		if (lst == COMP_EXPAND_COMPLETE) {
			char *q = s;

			if (*q == Tilde) q++;
			else if (*q == Equals) {
				for (q++; *q; q++)
					if (*q == '/')
						break;
				if (!*q)
					lst = COMP_EXPAND;
				q = s+1;
			} else {
				for (; *q && *q != String; q++);
				if (*q == String) {
					if (getsparam(q+1)) lst = COMP_EXPAND;
					else lst = COMP_COMPLETE;
				}
				q = s;
			}
			if (lst == COMP_EXPAND_COMPLETE) {
				for (; *q; q++)
					if (itok(*q))
						break;
				if (!*q)
					lst = COMP_COMPLETE;
			}
		}
		if (lst == COMP_SPELL) {
			char	**x = &s;
			untokenize(s);
			cs = wb;
			foredel(we-wb);
			/* call the real spell checker, ash@aaii.oz.zu */
			spckword(x, NULL, NULL, lincmd, 0);
			inststr(*x);
		} else if (COMP_ISEXPAND(lst))
			doexpansion(s,lst,lincmd);
		else {
			docompletion(s,lst,lincmd);
		}
		free(s);
	}
	popheap();
	lexrestore();
}

void do_menucmp(lst) /**/
int lst;
{
char *s;

	if (isset(LASTMENU) && lastmenu) {
		if (COMP_ISEXPAND(lst) || cs != lastmenupos ||
				strcmp(line, lastmenu) != 0) {
			free(lastmenu);
			lastmenu = NULL;
			lastmenupos = -1;
			freemenu();
		}
	}
	if (lst == COMP_LIST_COMPLETE) {
		listmatches(menulist, NULL);
		return;
	}
	cs = menub;
	foredel(menue-menub);
	incnode(menunode);
	if (!menunode)
		menunode = firstnode(menulist);
	s = menunode->dat;
	if (*s == '~' || *s == '=' || *s == '$') {
		spaceinline(1);
		line[cs++] = *s++;
	}
	inststr(s = menunode->dat);
	if (isset(LASTMENU)) {
		if (lastmenu) free(lastmenu);
		lastmenu = ztrdup(line);
		lastmenupos = cs;
	}
	menue = cs;
}

char *get_comp_string() /**/
{
int t0;
char *s,*linptr;

	linptr = line;
start:
	lincmd = incmdpos;
	cmdstr = NULL;
	zleparse = 1;
	lexsave();
	hungets(" "); /* KLUDGE! */
	hungets(linptr);
	strinbeg();
	pushheap();
	do {
		lincmd = incmdpos;
		ctxtlex();
		if (tok == ENDINPUT) break;
		if (lincmd) cmdstr = strdup(tokstr);
	} while (tok != ENDINPUT && zleparse);
	t0 = tok;
	if (t0 == ENDINPUT) {
		s = ztrdup("");
		we = wb = cs;
		t0 = STRING;
	} else if (t0 == STRING) {
		s = ztrdup(tokstr);
	} else if (t0 == ENVSTRING) {
		for (s = tokstr; *s && *s != '='; s++, wb++);
		if (*s) { s++; wb++; t0 = STRING; s = ztrdup(s); }
		lincmd = 1;
	}
	hflush();
	strinend();
	errflag = zleparse = 0;
	if (we > ll) we = ll;
	if (t0 == LEXERR && parbegin != -1) {
		linptr += ll+1-parbegin;
		popheap();
		lexrestore();
		goto start;
	}
	if (t0 != STRING) { feep(); return NULL; }
	return s;
}

void doexpansion(s,lst,lincmd) /**/
char *s;int lst;int lincmd;
{
Lklist vl = newlist();
char *ss;

	pushheap();
	addnode(vl,s);
	prefork(vl);
	if (errflag)
		goto end;
	postfork(vl,1);
	if (errflag)
		goto end;
	if (!full(vl) || !*(char *) peekfirst(vl)) {
		feep();
		goto end;
	}
	if (lst == COMP_LIST_EXPAND) {
		listmatches(vl,NULL);
		goto end;
	} else if (peekfirst(vl) == s) {
		if (lst == COMP_EXPAND_COMPLETE) {
			docompletion(s,COMP_COMPLETE,lincmd);
		} else
			feep();
		goto end;
	}
	cs = wb;
	foredel(we-wb);
	while (ss = ugetnode(vl)) {
		untokenize(ss);
		inststr(ss);
		if (full(vl)) {
			spaceinline(1);
			line[cs++] = ' ';
		}
	}
end:
	popheap();
	setterm();
}

void gotword(s) /**/
char *s;
{
	we = ll+1-inbufct;
	if (cs <= we)
		{
		wb = ll-wordbeg;
		zleparse = 0;
		/* major hack ahead */
		if (wb && line[wb] == '!' && line[wb-1] == '\\')
			wb--;
		}
}

void inststrlen(s,l) /**/
char *s;int l;
{
char *t,*u,*v;

	t = halloc(strlen(s)*2+2);
	u = s;
	v = t;
	for (; *u; u++)
		{
		if (l != -1 && !l--)
			break;
		if (ispecial(*u))
			if (*u == '\n')
				{
				*v++ = '\'';
				*v++ = '\n';
				*v++ = '\'';
				continue;
				}
			else
				*v++ = '\\';
		*v++ = *u;
		}
	*v = '\0';
	spaceinline(strlen(t));
	strncpy(line+cs,t,strlen(t));
	cs += strlen(t);
}

static int ambig,haspath,exact;
static Lklist matches;
static char *pat;
static int typechar;

void addmatch(s) /**/
char *s;
{
	if (full(matches))
		{
		int y = pfxlen(peekfirst(matches),s);

		if (y < ambig)
			ambig = y;
		}
	else
		ambig = strlen(s);
	if (!strcmp(pat,s))
		exact = 1;
	addnodeinorder(matches,strdup(s));
}


void addcmdmatch(s,t) /**/
char *s;char *t;
{
	if (strpfx(pat,s))
		addmatch(s);
}

void addcmdnodis(s,t) /**/
char *s;char *t;
{
	if (strpfx(pat,s) && ((Cmdnam) t)->type != DISABLED)
		addmatch(s);
}

void maketildelist(s) /**/
char	*s;
{
	struct passwd	*pwd;
	int		len;

	s++;
	len = strlen(s);
	if (len < 1) {
		addmatch(s);
		*s = 0;
		return;
	}
	while ((pwd = getpwent()) != NULL && !errflag)
		if (strncmp(pwd->pw_name, s, len) == 0)
			addmatch(pwd->pw_name);
	endpwent();
	*s = 0;
}

/*
 * opendir that handles '~' and '=' and '$'.
 * orig. by ash@aaii.oz.au, mod. by pf
 */
DIR *OPENDIR(s)
char	*s;
{
	if (*s != '~' && *s != '=' && *s != '$')
		return(opendir(s));
	s = strdup(s);
	*s = (*s == '=') ? Equals : (*s == '~') ? Tilde : String;
	singsub(&s);
	return(opendir(s));
}
char *dirname(s)
char	*s;
{
	if (*s == '~' || *s == '=' || *s == '$') {
	  s = strdup(s);
	  *s = (*s == '=') ? Equals : (*s == '~') ? Tilde : String;
	  singsub(&s);
	}
	return(s);
}

int Isdir(s) /**/
char *s;
{
struct stat sbuf;

   if (stat(s,&sbuf) == -1)
      return 0;
   return S_ISDIR(sbuf.st_mode);
}

/* this will work whether s is tokenized or not */
int isdir(t,s) /**/
char *t;char *s;
{
char buf[MAXPATHLEN];

	if (typechar != '$')
		sprintf(buf,"%s/%s",(s) ? s : ".",t);
	else
		sprintf(buf,"$%s",t);
	s = buf;
	if (*s != '~' && *s != '=' && *s != Tilde && *s != Equals &&
		 *s != '$' && *s != String)
		return(Isdir(s));
	s = strdup(s);
	if (*s == '~' || *s == '=' || *s == '$')
		*s = (*s == '=') ? Equals : (*s == '~') ? Tilde : String;
	singsub(&s);
	return(Isdir(s));
}

#define SLASH_YES   0
#define SLASH_NO    1
#define SLASH_MAYBE 2

int slashflag;
int addedstar;
char *pathprefix;

void docompletion(s,lst,incmd) /**/
char *s;int lst;int incmd;
{
char *tokorigs;
char *origs;

	slashflag = SLASH_MAYBE;
	addedstar = 0;
	lastambig = 0;

	heapalloc();
	pushheap();
	if (useglob)
	    tokorigs = strdup(s);
	untokenize(s);
	origs = strdup(s);
	matches = newlist();
	if (cmdstr && inarray(cmdstr,hostcmds)) {
		char **x;
		haspath = exact = 0;
		slashflag = SLASH_NO;
		pat = s;
		for (x = hosts; *x; x++) addcmdmatch(*x,NULL);
	} else if (cmdstr && inarray(cmdstr,optcmds)) {
		struct option *o;
		haspath = exact = 0;
		slashflag = SLASH_NO;
		pat = s;
		for (o = optns; o->name; o++) addcmdmatch(o->name,NULL);
	} else if (cmdstr && inarray(cmdstr,varcmds)) {
		haspath = exact = 0;
		slashflag = SLASH_NO;
		pat = s;
		listhtable(paramtab,addcmdmatch);
	} else if (cmdstr && inarray(cmdstr,bindcmds)) {
		int t0;
		haspath = exact = 0;
		slashflag = SLASH_NO;
		pat = s;
		for (t0 = 0; t0 != ZLECMDCOUNT; t0++)
			if (*zlecmds[t0].name) addcmdmatch(zlecmds[t0].name,NULL);
	} else {
		gen_matches_reg(s,incmd);
		/* only do "globbed" completion if regular completion fails.
		   pem, 7Oct91 */
		if ((!full(matches) || errflag) && useglob) {
		    gen_matches_glob(tokorigs,incmd);
		    /*
		     * gen_matches_glob changes the insert line to be correct up
		     * to the match, so the prefix string must be "". ash, 7Oct91
		     */
		    *s = 0;
		}
	}
	if (lst != COMP_LIST_COMPLETE) do_fignore(origs);
	if (!full(matches) || errflag) {
		feep();
	} else if (lst == COMP_LIST_COMPLETE) {
		listmatches(matches,
			unset(LISTTYPES) ? NULL :
				(haspath) ? pathprefix : "./");
	} else if (nextnode(firstnode(matches))) {
		do_ambiguous(s);
	} else {
		do_single(s);
	}
	ll = strlen(line);
	setterm();
	popheap();
	permalloc();
}

void gen_matches_glob(s,incmd) /**/
char *s;int incmd;
{
char *pt,*u;
int hasp = 0;
DIR *d;
struct direct *de;

	/*
	 * Find the longest prefix string without any
	 * chars special to glob - ash.
	 */
	for (pt = s; *pt; pt++) {
		if (pt == s && (*pt == Tilde || *pt == Equals)) continue;
		if (ispecial(*pt) || itok(*pt)) break;
	}
	for (; pt > s && *pt != '/'; pt--) ;
	if (*pt == '/') {
		*pt = 0;
		u = pt + 1;
		wb += strlen(s);
		hasp = 1;
	} else u = s;
	if (!hasp && (*s == Tilde || *s == Equals)) {
		/* string contains only ~xx, so do tilde expansion */
		maketildelist(s);
		wb++;
		pathprefix = s;
		slashflag = SLASH_YES;
	} else if (incmd && !hasp) {
		slashflag = SLASH_NO;
		pat = s;
		listhtable(aliastab ,addcmdmatch);
		listhtable(cmdnamtab,addcmdnodis);
		if (d = opendir(".")) {
			char *q;

			readdir(d); readdir(d);
			while ((de = readdir(d)) && !errflag)
				if (strpfx(pat,q = de->d_name) &&
							(*q != '.' || *u == '.' || isset(GLOBDOTS)))
					addmatch(q);
			closedir(d);
		}
	} else {
		 int 	commonprefix = 0;
		 char	*prefix;
		 Lknode	n;
		 int		nonomatch = isset(NONOMATCH);

		 opts[NONOMATCH] = 1;
		 if (hasp) {
			/* Find the longest common prefix string
			 * after globbing the input. All expansions
			 * ~foo/bar/* will turn into something like
			 * /tmp_mnt/hosts/somehost/home/foo/...
			 * We will remove this common prefix from the matches.
			 * ash, 7 May '91
			 */
			pathprefix = s;
			addnode(matches,s);
			prefork(matches);
			if (!errflag) postfork(matches,1);
			if (!errflag) {
				prefix = peekfirst(matches);
				if (prefix) commonprefix = strlen(prefix) + 1;
				*pt = '/';
			}
		}
		if (s[strlen(s) - 1] == '/') {
			/* if strings ends in a '/' always add a '*' */
			s = dyncat(s,"x");
			s[strlen(s)-1] = Star;
			addedstar = 1;
		}
		matches = newlist();
		addnode(matches,s);
		prefork(matches);
		if (!errflag) postfork(matches,1);
		opts[NONOMATCH] = nonomatch;
		if (errflag || !full(matches) || !nextnode(firstnode(matches))) {
			/* if there were no matches (or only one)
				add a trailing * and try again */
			s = dyncat(s,"x");
			s[strlen(s)-1] = Star;
			addedstar = 1;
			matches = newlist();
			addnode(matches,s);
			prefork(matches);
			if (errflag) return;
			postfork(matches,1);
			if (errflag) return;
		}
		/* remove the common prefix from all the matches */
		if (commonprefix)
			for (n = firstnode(matches); n; incnode(n))
				n->dat = (char *) n->dat+commonprefix;
		s = pt;
		*s = 0;
	}
}

void gen_matches_reg(s,incmd) /**/
char *s;int incmd;
{
char *u;
DIR *d;
struct direct *de;

	haspath = exact = 0;
	for (u = s+strlen(s); u >= s; u--)
		if (*u == '/' || *u == '@' || *u == '$') break;
	typechar = *u;
	if (u >= s) {
		*u++ = '\0';
		haspath = 1;
	} else u = s;
	pat = u;
	if (typechar == '$' && haspath) {
		/* slashflag = SLASH_NO; */
		listhtable(paramtab,addcmdmatch);
	} else if (typechar == '@' && haspath) {
		char **x;
		slashflag = SLASH_NO;
		for (x = hosts; *x; x++) addcmdmatch(*x,NULL);
	} else if (*s == '~' && !haspath) {
		maketildelist(s);
		pathprefix = s;
		slashflag = SLASH_YES;
	} else if (incmd && !haspath) {
		slashflag = SLASH_NO;
		listhtable(aliastab ,addcmdmatch);
		listhtable(cmdnamtab,addcmdnodis);
		if (d = opendir(".")) {
			char *q;
			struct stat buf;

			readdir(d); readdir(d);
			while ((de = readdir(d)) && !errflag)
				if (strpfx(pat,q = de->d_name) &&
						(*q != '.' || *u == '.' || isset(GLOBDOTS)) &&
						stat(q,&buf) >= 0 &&
						(buf.st_mode & (S_IFMT|S_IEXEC)) == (S_IFREG|S_IEXEC))
					addmatch(q);
			closedir(d);
		}
	} else if (d = OPENDIR(pathprefix =
			((haspath || *s == '~') ? ((*s) ? s : "/") : "."))) {
		char *q,buf2[MAXPATHLEN];
		struct stat buf;
		char dn[MAXPATHLEN];
		
		strcpy(dn,dirname(pathprefix));
		readdir(d); readdir(d);
		while ((de = readdir(d)) && !errflag)
		  if (strpfx(pat,q = de->d_name) &&
				(*q != '.' || *u == '.' || isset(GLOBDOTS))) {
			 if (incmd) {
				sprintf(buf2,"%s/%s",dn,q);
				if (stat(buf2,&buf) < 0 ||
					 (buf.st_mode & S_IEXEC) == S_IEXEC) {
				  addmatch(q);
				}
			 } else {
				addmatch(q);
			 }
		  }
		closedir(d);
	}
}

void do_fignore(origstr) /**/
char *origstr;
{
	if (full(matches) && nextnode(firstnode(matches))) {
		Lknode z,zn;

		for (z = firstnode(matches); z; z = zn) {
			char *q = getdata(z);
			int namlen = strlen(q);
			int	slen = strlen(origstr);
			char **pt = fignore;
	
			zn = nextnode(z);
			for (; *pt; pt++) {
				/* We try to be smart here and override the
				   fignore variable if the user has explicity
				   used the ignored prefix, pem, 7 May 1991 */
				if (!addedstar && strcmp(origstr+slen-strlen(*pt), *pt) == 0)
					continue;
				if (strlen(*pt) < namlen && !strcmp(q+namlen-strlen(*pt),*pt)) {
					uremnode(matches,z);
					break;
				}
			}
		}
	}
}

void do_ambiguous(s) /**/
char *s;
{
	lastambig = 1;
	if (usemenu) { do_ambig_menu(s); return; }
	if (useglob) {
		feep();
		if (isset(AUTOLIST))
			listmatches(matches,
				unset(LISTTYPES) ? NULL : (haspath) ? pathprefix : "./");
		return;
	}
	cs = wb;
	foredel(we-wb);
	if (*s == '~' || *s == '=' || *s == '$') {
		spaceinline(1);
		line[cs++] = *s++;
	}
	if (haspath) {
		inststr(s);
		spaceinline(1);
		line[cs++] = typechar;
	}
	if (isset(RECEXACT) && exact) {
		lastambig = 0;
		if ((*pat == '~' || *pat == '=' || *pat == '$') && !haspath) {
			spaceinline(1);
			line[cs++] = *s++;
		}
		inststr(pat);
		spaceinline(1);
		switch (slashflag) {
			case SLASH_YES: line[cs++] = '/'; break;
			case SLASH_NO : line[cs++] = ' '; break;
			case SLASH_MAYBE: line[cs++] = isdir(pat,pathprefix)?'/':' '; break;
		}
		return;
	}
	s = peekfirst(matches);
	if ((*s == '~' || *s == '=' || *s == '$') && !haspath) {
		spaceinline(1);
		line[cs++] = *s++;
		ambig--;
	}
	inststrlen(s,ambig);
	refresh();
	if (isset(AUTOLIST)) {
		if (unset(NOLISTBEEP)) feep();
		listmatches(matches,
			unset(LISTTYPES) ? NULL : (haspath) ? pathprefix : "./");
	} else feep();
}

void do_single(s) /**/
char *s;
{
	cs = wb;
	foredel(we-wb);
	if (*s == '~' || *s == '=' || *s == '$') {
		spaceinline(1);
		line[cs++] = *s++;
	}
	if (haspath) {
		inststr(s);
		spaceinline(1);
		line[cs++] = typechar;
	}
	s = peekfirst(matches);
	if ((*s == '~' || *s == '=' || *s == '$') && !haspath) {
		spaceinline(1);
		line[cs++] = *s++;
	}
	inststr(s);
	spaceinline(1);
	switch (slashflag) {
		case SLASH_YES: line[cs++] = '/'; break;
		case SLASH_NO : line[cs++] = ' '; break;
		case SLASH_MAYBE: line[cs++] = isdir(s,pathprefix) ? '/' : ' '; break;
	}
}

void do_ambig_menu(s) /**/
char *s;
{
	menucmp = 1;
	if (isset(MENUCOMPLETEBEEP)) feep();
	cs = wb;
	menuw = cs;
	foredel(we-wb);
	if (*s == '~' || *s == '=' || *s == '$') {
		spaceinline(1);
		line[cs++] = *s++;
	}
	if (haspath) {
		inststr(s);
		spaceinline(1);
		line[cs++] = typechar;
	}
	menub = cs;
	s = peekfirst(matches);
	if ((*s == '~' || *s == '=' || *s == '$') && !haspath) {
		spaceinline(1);
		line[cs++] = *s++;
	}
	inststr(s);
	menue = cs;
	permalloc();
	menulist = duplist(matches,(VFunc)ztrdup);
	heapalloc();
	menunode = firstnode(menulist);
	permalloc();
	if (isset(LASTMENU)) {
		if (lastmenu)
			free(lastmenu);
		lastmenu = ztrdup(line);
		lastmenupos = cs;
	}
}

int strpfx(s,t) /**/
char *s;char *t;
{
	while (*s && *s == *t) s++,t++;
	return !*s;
}

int pfxlen(s,t) /**/
char *s;char *t;
{
int i = 0;

	while (*s && *s == *t) s++,t++,i++;
	return i;
}

void listmatches(l,apps) /**/
Lklist l;char *apps;
{
int longest = 1,fct,fw = 0,colsz,t0,t1,ct;
Lknode n;
char **arr,**ap;

	trashzle();
	ct = countnodes(l);
	if (listmax && ct > listmax)
		{
		fprintf(stdout,"zsh: do you wish to see all %d possibilities? ",ct);
		fflush(stdout);
		if (getquery() != 'y')
			return;
		}
	ap = arr = alloc((countnodes(l)+1)*sizeof(char **));
	for (n = firstnode(l); n; incnode(n))
		*ap++ = getdata(n);
	*ap = NULL;
	for (ap = arr; *ap; ap++)
		if (strlen(*ap) > longest)
			longest = strlen(*ap);
	if (apps)
		{
		apps = strdup(apps);
		if (*apps == '~')
			*apps = Tilde;
		else if (*apps == '=')
			*apps = Equals;
		else if (*apps == '$')
			*apps = String;
		singsub(&apps);
		longest++;
		}
	qsort(arr,ct,sizeof(char *),forstrcmp);
	fct = (columns-1)/(longest+2);
	if (fct == 0)
		fct = 1;
	else
		fw = (columns-1)/fct;
	colsz = (ct+fct-1)/fct;
	for (t1 = 0; t1 != colsz; t1++)
		{
		ap = arr+t1;
		if (apps)
			{
			do
				{
				int t2 = strlen(*ap)+1;
				char pbuf[MAXPATHLEN];
				struct stat buf;

				printf("%s",*ap);
				sprintf(pbuf,"%s/%s",apps,*ap);
				if (lstat(pbuf,&buf))
					putchar(' ');
				else switch (buf.st_mode & S_IFMT) /* screw POSIX */
					{
					case S_IFDIR: putchar('/'); break;
#ifdef S_IFIFO
					case S_IFIFO: putchar('|'); break;
#endif
					case S_IFCHR: putchar('%'); break;
					case S_IFBLK: putchar('#'); break;
#ifdef S_IFLNK
					case S_IFLNK: putchar(
						(access(pbuf,F_OK) == -1) ? '&' : '@'); break;
#endif
#ifdef S_IFSOCK
					case S_IFSOCK: putchar('='); break;
#endif
					default:
						if (buf.st_mode & 0111)
							putchar('*');
						else
							putchar(' ');
						break;
					}
				for (; t2 < fw; t2++) putchar(' ');
				for (t0 = colsz; t0 && *ap; t0--,ap++);
				}
			while (*ap);
			}
		else
			do
				{
				int t2 = strlen(*ap);

				printf("%s",*ap);
				for (; t2 < fw; t2++) putchar(' ');
				for (t0 = colsz; t0 && *ap; t0--,ap++);
				}
			while (*ap);
		putchar('\n');
		}
	resetneeded = 1;
	fflush(stdout);
}

void selectlist(l) /**/
Lklist l;
{
int longest = 1,fct,fw = 0,colsz,t0,t1,ct;
Lknode n;
char **arr,**ap;

	trashzle();
	ct = countnodes(l);
	ap = arr = alloc((countnodes(l)+1)*sizeof(char **));
	for (n = firstnode(l); n; incnode(n))
		*ap++ = getdata(n);
	*ap = NULL;
	for (ap = arr; *ap; ap++)
		if (strlen(*ap) > longest)
			longest = strlen(*ap);
	t0 = ct;
	longest++;
	while (t0)
		t0 /= 10, longest++;
	fct = (columns-1)/(longest+2);
	if (fct == 0)
		fct = 1;
	else
		fw = (columns-1)/fct;
	colsz = (ct+fct-1)/fct;
	for (t1 = 0; t1 != colsz; t1++)
		{
		ap = arr+t1;
		do
			{
			int t2 = strlen(*ap)+1,t3;

			fprintf(stderr,"%d %s",t3 = ap-arr+1,*ap);
			while (t3)
				t2++,t3 /= 10;
			for (; t2 < fw; t2++) fputc(' ',stderr);
			for (t0 = colsz; t0 && *ap; t0--,ap++);
			}
		while (*ap);
		fputc('\n',stderr);
		}
	resetneeded = 1;
	fflush(stderr);
}

int doexpandhist() /**/
{
char *cc,*ce;
int t0,oldcs,oldll;

	for (cc = line, ce = line+ll; cc < ce; cc++)
		if (*cc == '\\' && cc[1])
			cc++;
		else if (*cc == bangchar)
			break;
	if (*cc == bangchar && cc[1] == '"') return 0;
	if (cc == ce && *line != hatchar)
		return 0;
	oldcs = cs;
	oldll = ll;
	zleparse = 1;
	lexsave();
	hungets(line);
	strinbeg();
	pushheap();
	ll = cs = 0;
	for(;;)
		{
		t0 = hgetc();
		if (lexstop)
			break;
		spaceinline(1);
		line[cs++] = t0;
		}
	hflush();
	popheap();
	strinend();
	errflag = zleparse = 0;
	t0 = histdone;
	lexrestore();
	line[ll = cs] = '\0';
	if (ll == oldll) cs = oldcs;
	return t0;
}

void magicspace() /**/
{
	doexpandhist();
	c = ' ';
	selfinsert();
}

void expandhistory() /**/
{
	if (!doexpandhist())
		feep();
}

static int cmdwb,cmdwe;

char *getcurcmd() /**/
{
int lincmd = incmdpos;
char *s = NULL;

	zleparse = 1;
	lexsave();
	hungets(" "); /* KLUDGE! */
	hungets(line);
	strinbeg();
	pushheap();
	do {
		lincmd = incmdpos;
		ctxtlex();
		if (tok == ENDINPUT) break;
		if (tok == STRING && lincmd) {
			if (s) free(s);
			s = ztrdup(tokstr);
			cmdwb = ll-wordbeg; cmdwe = ll+1-inbufct;
		}
		lincmd = incmdpos;
	} while (tok != ENDINPUT && zleparse);
	hflush();
	popheap();
	strinend();
	errflag = zleparse = 0;
	lexrestore();
	return s;
}

void processcmd() /**/
{
char *s,*t;

	s = getcurcmd();
	if (!s) { feep(); return; }
	t = zlecmds[bindk].name;
	mult = 1;
	pushline();
	sizeline(strlen(s)+strlen(t)+1);
	strcpy(line,t);
	strcat(line," ");
	cs = ll = strlen(line);
	inststr(s);
	free(s);
	done = 1;
}

void expandcmdpath() /**/
{
int oldcs = cs;
char *s,*str;

	s = getcurcmd();
	if (!s) { feep(); return; }
	str = findcmd(s);
	free(s);
	if (!str) { feep(); return; }
	cs = cmdwb;
	foredel(cmdwe-cmdwb);
	spaceinline(strlen(str));
	strncpy(line+cs,str,strlen(str));
	cs = oldcs;
	if (cs >= cmdwe) cs += cmdwe-cmdwb+strlen(str);
	if (cs > ll) cs = ll;
	free(str);
}

void freemenu() /**/
{
	if (menucmp && (unset(LASTMENU) || lastmenu == NULL)) {
		menucmp = 0;
		freetable(menulist,freestr);
	}
}

int inarray(s,a) /**/
char *s; char **a;
{
	for (; *a; a++) if (!strcmp(*a,s)) return 1;
	return 0;
}

