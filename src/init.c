/*

	init.c - main loop and initialization routines

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

#define GLOBALS
#include "zsh.h"
#include <pwd.h>

extern int yydebug;

void main(argc,argv,envp) /**/
int argc; char **argv; char **envp;
{
int notect = 0;

	environ = envp;
	pathsuppress = 1;
	meminit();
	setflags();
	parseargs(argv);
	setmoreflags();
	setupvals();
	initialize();
	heapalloc();
	runscripts();
	if (interact)
		{
		pathsuppress = 0;
		newcmdnamtab();
		}
	for(;;)
		{
		do
			loop();
		while (tok != ENDINPUT);
		if (!(isset(IGNOREEOF) && interact))
			{
#if 0
			if (interact)
				fputs(islogin ? "logout\n" : "exit\n",stderr);
#endif
			zexit(NULL);
			continue;
			}
		zerrnam("zsh",(!islogin) ? "use 'exit' to exit."
			: "use 'logout' to logout.",NULL,0);
		notect++;
		if (notect == 10)
			zexit(NULL);
		}
}

/* keep executing lists until EOF found */

void loop() /**/
{
List list;

	pushheap();
	for(;;)
		{
		freeheap();
		if (interact && isset(SHINSTDIN))
			preprompt();
		hbegin();		/* init history mech */
		intr();			/* interrupts on */
		ainit();			/* init alias mech */
		lexinit();
		errflag = 0;
		if (!(list = parse_event()))
			{				/* if we couldn't parse a list */
			hend();
			if (tok == ENDINPUT && !errflag)
				break;
			continue;
			}
		if (hend())
			{
			if (stopmsg)		/* unset 'you have stopped jobs' flag */
				stopmsg--;
			execlist(list);
			}
		if (ferror(stderr))
			{
			zerr("write error",NULL,0);
			clearerr(stderr);
			}
		if (subsh)				/* how'd we get this far in a subshell? */
			exit(lastval);
		if ((!interact && errflag) || retflag)
			break;
		if ((opts['t'] == OPT_SET) || (lastval && opts[ERREXIT] == OPT_SET))
			{
			if (sigtrapped[SIGEXIT])
				dotrap(SIGEXIT);
			exit(lastval);
			}
		}
	popheap();
}

void setflags() /**/
{
int c;

	for (c = 0; c != 32; c++)
		opts[c] = OPT_UNSET;
	for (c = 32; c != 128; c++)
		opts[c] = OPT_INVALID;
	for (c = 'a'; c <= 'z'; c++)
		opts[c] = opts[c-'a'+'A'] = OPT_UNSET;
	for (c = '0'; c <= '9'; c++)
		opts[c] = OPT_UNSET;
	opts['A'] = OPT_INVALID;
	opts['i'] = (isatty(0)) ? OPT_SET : OPT_UNSET;
	opts[BGNICE] = opts[NOTIFY] = OPT_SET;
	opts[USEZLE] = (interact && SHTTY != -1) ? OPT_SET : OPT_UNSET;
}

static char *cmd;

void parseargs(argv) /**/
char **argv;
{
char **x;
int bk = 0,action;
Lklist paramlist;

	hackzero = argzero = *argv;
	opts[LOGINSHELL] = (**(argv++) == '-') ? OPT_SET : OPT_UNSET;
	SHIN = 0;
	while (!bk && *argv && (**argv == '-' || **argv == '+'))
		{
		action = (**argv == '-') ? OPT_SET : OPT_UNSET;
		while (*++*argv)
			{
			if (opts[**argv] == OPT_INVALID)
				{
				zerr("bad option: -%c",NULL,**argv);
				exit(1);
				}
			opts[**argv] = action;
			if (bk = **argv == 'b')
				break;
			if (**argv == 'c') /* -c command */
				{
				argv++;
				if (!*argv)
					{
					zerr("string expected after -c",NULL,0);
					exit(1);
					}
				cmd = *argv;
				opts[INTERACTIVE] = OPT_UNSET;
				break;
				}
			else if (**argv == 'o')
				{
				int c;

				if (!*++*argv)
					argv++;
				if (!*argv)
					{
					zerr("string expected after -o",NULL,0);
					exit(1);
					}
				c = optlookup(*argv);
				if (c == -1)
					zerr("no such option: %s",*argv,0);
				else
					opts[c] = action;
				break;
				}
			}
		argv++;
		}
	paramlist = newlist();
	if (*argv)
		{
		if (opts[SHINSTDIN] == OPT_UNSET)
			{
			SHIN = movefd(open(argzero = *argv,O_RDONLY));
			if (SHIN == -1)
				{
				zerr("can't open input file: %s",*argv,0);
				exit(1);
				}
			opts[INTERACTIVE] = OPT_UNSET;
			argv++;
			}
		while (*argv)
			addnode(paramlist,ztrdup(*argv++));
		}
	else
		opts[SHINSTDIN] = OPT_SET;
	pparams = x = zcalloc((countnodes(paramlist)+1)*sizeof(char *));
	while (*x++ = getnode(paramlist));
	free(paramlist);
	argzero = ztrdup(argzero);
}

void setmoreflags() /**/
{
int t0;

	/* stdout,stderr fully buffered */
#ifdef _IOFBF
	setvbuf(stdout,malloc(BUFSIZ),_IOFBF,BUFSIZ);
	setvbuf(stderr,malloc(BUFSIZ),_IOFBF,BUFSIZ);
#else
	setbuffer(stdout,malloc(BUFSIZ),BUFSIZ);
	setbuffer(stderr,malloc(BUFSIZ),BUFSIZ);
#endif
	subsh = 0;
#ifndef NOCLOSEFUNNYFDS
	/* this works around a bug in some versions of in.rshd */
	for (t0 = 3; t0 != 10; t0++)
		close(t0);
#endif
#ifdef JOB_CONTROL
	opts[MONITOR] = (interact) ? OPT_SET : OPT_UNSET;
	if (jobbing)
		{
		SHTTY = movefd((isatty(0)) ? dup(0) : open("/dev/tty",O_RDWR));
		if (SHTTY == -1)
			opts[MONITOR] = OPT_UNSET;
		else
			{
#ifdef TIOCSETD
			int ldisc = NTTYDISC;
			ioctl(SHTTY, TIOCSETD, &ldisc);
#endif
			gettyinfo(&shttyinfo);	/* get tty state */
			savedttyinfo = shttyinfo;
			}
#ifdef sgi
		setpgrp(0,getpgrp(0));
#endif
		if ((mypgrp = getpgrp(0)) <= 0)
			opts[MONITOR] = OPT_UNSET;
		}
	else
		SHTTY = -1;
#else
	opts[MONITOR] = OPT_UNSET;
	SHTTY = movefd((isatty(0)) ? dup(0) : open("/dev/tty",O_RDWR));
	if (SHTTY != -1)
		{
		gettyinfo(&shttyinfo);
		savedttyinfo = shttyinfo;
		}
#endif
}

void setupvals() /**/
{
struct passwd *pwd;
char *ptr;
static long bauds[] = {
	0,50,75,110,134,150,200,300,600,1200,1800,2400,4800,9600,19200,38400
	};

	curhist = 0;
	histsiz = 20;
	lithistsiz = 5;
	logcheck = 60;
	dirstacksize = -1;
	listmax = 100;
	bangchar = '!';
	hashchar = '#';
	hatchar = '^';
	termok = 0;
	curjob = prevjob = coprocin = coprocout = -1;
	shtimer = time(NULL);	/* init $SECONDS */
	srand((unsigned int) shtimer);
	/* build various hash tables; argument to newhtable is table size */
	aliastab = newhtable(37);
	addreswords();
	paramtab = newhtable(151);
	cmdnamtab = newhtable(13);
	initxbindtab();
	nullcmd = ztrdup("cat");
	prompt = ztrdup("%m%# ");
	prompt2 = ztrdup("> ");
	prompt3 = ztrdup("?# ");
	prompt4 = ztrdup("+ ");
	sprompt = ztrdup("zsh: correct `%s' to `%r' [nyae]? ");
	ppid = getppid();
#ifdef TERMIOS
	baud = bauds[shttyinfo.termios.c_cflag & CBAUD];
#else
#ifdef TERMIO
	baud = bauds[shttyinfo.termio.c_cflag & CBAUD];
#else
	baud = bauds[shttyinfo.sgttyb.sg_ospeed];
#endif
#endif
#ifdef TIOCGWINSZ
	if (!(columns = shttyinfo.winsize.ws_col))
		columns = 80;
	if (!(lines = shttyinfo.winsize.ws_row))
		lines = 24;
#else
	columns = 80;
	lines = 24;
#endif
	ifs = ztrdup(" \t\n");
	if (pwd = getpwuid(getuid())) {
		username = ztrdup(pwd->pw_name);
		home = ztrdup(pwd->pw_dir);
	} else {
		username = ztrdup("");
		home = ztrdup("/");
	}
	timefmt = ztrdup(DEFTIMEFMT);
	watchfmt = ztrdup(DEFWATCHFMT);
	if (!(ttystrname = ztrdup(ttyname(SHTTY))))
		ttystrname = ztrdup("");
	wordchars = ztrdup(DEFWORDCHARS);
	fceditparam = ztrdup(DEFFCEDIT);
	tmpprefix = ztrdup(DEFTMPPREFIX);
	if (ispwd(home))
		cwd = ztrdup(home);
	else if ((ptr = zgetenv("PWD")) && ispwd(ptr))
		cwd = ztrdup(ptr);
	else
		cwd = zgetwd();
	oldpwd = ztrdup(cwd);
	hostnam = zalloc(256);
	underscore = ztrdup("");
	gethostname(hostnam,256);
	mypid = getpid();
	cdpath = mkarray(NULL);
	fignore = mkarray(NULL);
	fpath = mkarray(NULL);
	mailpath = mkarray(NULL);
	watch = mkarray(NULL);
	hosts = mkarray(NULL);
	hostcmds = (char **) zcalloc(sizeof(char *)*7);
	hostcmds[0] = ztrdup("telnet"); hostcmds[1] = ztrdup("rlogin");
	hostcmds[2] = ztrdup("ftp"); hostcmds[3] = ztrdup("rup");
	hostcmds[4] = ztrdup("rusers"); hostcmds[5] = ztrdup("rsh");
	optcmds = (char **) zcalloc(sizeof(char *)*3);
	optcmds[0] = ztrdup("setopt"); optcmds[1] = ztrdup("unsetopt");
	bindcmds = (char **) zcalloc(sizeof(char *)*2);
	bindcmds[0] = ztrdup("bindkey");
	varcmds = (char **) zcalloc(sizeof(char *)*5);
	varcmds[0] = ztrdup("export"); varcmds[1] = ztrdup("typeset");
	varcmds[2] = ztrdup("vared"); varcmds[3] = ztrdup("unset");
	userdirs = (char **) zcalloc(sizeof(char *)*2);
	usernames = (char **) zcalloc(sizeof(char *)*2);
	userdirsz = 2;
	userdirct = 0;
	optarg = ztrdup("");
	optind = 0;
	schedcmds = NULL;
	path = (char **) zalloc(4*sizeof *path);
	path[0] = ztrdup("/bin"); path[1] = ztrdup("/usr/bin");
	path[2] = ztrdup("/usr/ucb"); path[3] = NULL;
	inittyptab();
	initlextabs();
	setupparams();
	setparams();
	inittyptab();
}

void initialize() /**/
{
int t0;

	breaks = loops = 0;
	lastmailcheck = time(NULL);
	firsthist = firstlithist = 1;
	histsiz = DEFAULT_HISTSIZE;
	histlist = newlist();
	lithistlist = newlist();
	locallist = NULL;
	dirstack = newlist();
	bufstack = newlist();
	newcmdnamtab();
	inbuf = zalloc(inbufsz = 256);
	inbufptr = inbuf+inbufsz;
	inbufct = 0;
#ifdef QDEBUG
	signal(SIGQUIT,SIG_IGN);
#endif
#ifdef RLIM_INFINITY
	for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
		getrlimit(t0,limits+t0);
#endif
	hsubl = hsubr = NULL;
	lastpid = 0;
	bshin = fdopen(SHIN,"r");
	signal(SIGCHLD,handler);
	if (jobbing)
		{
		signal(SIGTTOU,SIG_IGN);
		signal(SIGTSTP,SIG_IGN);
		signal(SIGTTIN,SIG_IGN);
		signal(SIGPIPE,SIG_IGN);
		attachtty(mypgrp);
		}
	if (interact)
		{
		signal(SIGTERM,SIG_IGN);
#ifdef SIGWINCH
		signal(SIGWINCH,handler);
#endif
		signal(SIGALRM,handler);
		intr();
		}
}

void addreswords() /**/
{
static char *reswds[] = {
	"do", "done", "esac", "then", "elif", "else", "fi", "for", "case",
	"if", "while", "function", "repeat", "time", "until", "exec", "command",
	"select", "coproc", "noglob", "-", "nocorrect", "foreach", "end", NULL
	};
int t0;

	for (t0 = 0; reswds[t0]; t0++)
		addhperm(reswds[t0],mkanode(NULL,-1-t0),aliastab,NULL);
}

void runscripts() /**/
{
	if (opts[NORCS] == OPT_SET) {
#ifdef GLOBALZPROFILE
		source(GLOBALZPROFILE);
#endif
#ifdef GLOBALZSHRC
		source(GLOBALZSHRC);
#endif
#ifdef GLOBALZLOGIN
		if (islogin) source(GLOBALZLOGIN);
#endif
		return;
	}
	sourcehome(".zshenv");
	if (opts[NORCS] == OPT_SET)
		return;
	if (interact) {
		if (islogin) {
			sourcehome(".zprofile");
#ifdef GLOBALZPROFILE
			source(GLOBALZPROFILE);
#endif
		}
#ifdef GLOBALZSHRC
		source(GLOBALZSHRC);
#endif
		sourcehome(".zshrc");
		if (islogin) {
#ifdef GLOBALZLOGIN
			source(GLOBALZLOGIN);
#endif
			sourcehome(".zlogin");
		}
	}
	if (interact)
		readhistfile(getsparam("HISTFILE"),0);
	if (opts['c'] == OPT_SET)
		{
		if (SHIN >= 10)
			close(SHIN);
		SHIN = movefd(open("/dev/null",O_RDONLY));
		hungets(cmd);
		strinbeg();
		}
#ifdef TIOCSWINSZ
	if (!(columns = shttyinfo.winsize.ws_col))
		columns = 80;
	if (!(lines = shttyinfo.winsize.ws_row))
		lines = 24;
#endif
}

void ainit() /**/
{
	alstackind = 0;		/* reset alias stack */
	alstat = 0;
	isfirstln = 1;
}

void readhistfile(s,err) /**/
char *s;int err;
{
char buf[1024];
FILE *in;

	if (!s)
		return;
	if (in = fopen(s,"r"))
		{
		permalloc();
		while (fgets(buf,1024,in))
			{
			int l = strlen(buf);
			char *pt = buf;

			if (l && buf[l-1] == '\n')
				buf[l-1] = '\0';
			for (;*pt;pt++)
			    if (*pt == ' ')
				*pt = HISTSPACE;
			addnode(histlist,ztrdup(buf));
			addnode(lithistlist,ztrdup(buf));
			curhist++;
			}
		fclose(in);
		lastalloc();
		}
	else if (err)
		zerr("can't read history file",s,0);
}

void savehistfile(s,err) /**/
char *s;int err;
{
char *t;
Lknode n;
Lklist l;
FILE *out;

	if (!s || !interact)
		return;
	if (out = fdopen(open(s,O_CREAT|O_WRONLY|O_TRUNC,0600),"w"))
		{
		n = lastnode(l = (isset(HISTLIT) ? lithistlist : histlist));
		if (n == (Lknode) l)
			{
			fclose(out);
			return;
			}
		while (--savehist && prevnode(n) != (Lknode) l)
			n = prevnode(n);
		for (; n; incnode(n))
			{
			for (s = t = getdata(n); *s; s++)
				if (*s == HISTSPACE)
					*s = ' ';
			fputs(t,out);
			fputc('\n',out);
			}
		fclose(out);
		}
	else if (err)
		zerr("can't write history file: %s",s,0);
}

