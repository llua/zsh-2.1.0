/*

	zsh.h - standard header file

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

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef SYSV
#include <sys/bsdtypes.h>
#define _POSIX_SOURCE
#include <sys/limits.h>
#include <sys/sioctl.h>
#define MAXPATHLEN PATH_MAX
#define lstat stat
extern int gethostname();
#define sigmask(m) m
#include <sys/dirent.h>
struct rusage { int foo; }
#else
#include <sys/types.h>		/* this is the key to the whole thing */
#endif

#include <sys/wait.h>
#include <sys/time.h>
#ifndef SYSV
#include <sys/resource.h>
#endif
#include <sys/file.h>
#include <signal.h>
#ifdef TERMIO
#include <sys/termio.h>
#else
#ifdef TERMIOS
#include <sys/termios.h>
#else
#include <sgtty.h>
#endif
#endif

#ifdef SYSV
#undef TIOCGWINSZ
#endif

#include <sys/param.h>

#ifdef SYSV
#undef _POSIX_SOURCE
#endif

#ifdef __hp9000s800
#include <sys/bsdtty.h>
#endif

#define VERSIONSTR "zsh v2.1.0"

#if 0 /* __STDC__ */
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stat.h>
#define DCLPROTO(X) X
#undef NULL
#define NULL ((void *)0)
#else /* not __STDC__ */
#include <sys/stat.h>
#define DCLPROTO(X) ()
#ifndef NULL
#define NULL 0
#endif
#endif /* __STDC__ */

#define DEFWORDCHARS "*?_-.[]~=/&;!#$%^(){}<>"
#define DEFTIMEFMT "%E real  %U user  %S system  %P"
#ifdef UTMP_HOST
#define DEFWATCHFMT "%n has %a %l from %m."
#else
#define DEFWATCHFMT "%n has %a %l."
#endif

#ifdef GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

#ifndef F_OK
#define F_OK 00
#define R_OK 04
#define W_OK 02
#define X_OK 01
#endif

#include "zle.h"

/* size of job list */

#define MAXJOB 80

/* memory allocation routines - changed with permalloc()/heapalloc() */

vptr (*alloc)DCLPROTO((int));
vptr (*ncalloc)DCLPROTO((int));

#define addhnode(A,B,C,D) Addhnode(A,B,C,D,1)
#define addhperm(A,B,C,D) Addhnode(A,B,C,D,0)

/* character tokens */

#define ALPOP			((char) 0x81)
#define HISTSPACE		((char) 0x83)
#define Pound			((char) 0x84)
#define String			((char) 0x85)
#define Hat				((char) 0x86)
#define Star			((char) 0x87)
#define Inpar			((char) 0x88)
#define Outpar			((char) 0x89)
#define Qstring		((char) 0x8a)
#define Equals			((char) 0x8b)
#define Bar				((char) 0x8c)
#define Inbrace		((char) 0x8d)
#define Outbrace		((char) 0x8e)
#define Inbrack		((char) 0x8f)
#define Outbrack		((char) 0x90)
#define Tick			((char) 0x91)
#define Inang			((char) 0x92)
#define Outang			((char) 0x93)
#define Quest			((char) 0x94)
#define Tilde			((char) 0x95)
#define Qtick			((char) 0x96)
#define Comma			((char) 0x97)
#define Nularg			((char) 0x98)

/* chars that need to be quoted if meant literally */

#define SPECCHARS "#$^*()$=|{}[]`<>?~;&!\n\t \\\'\""

/* ALPOP in the form of a string */

#define ALPOPS " \201"
#define HISTMARK "\201"

#define SEPER 1
#define NEWLIN 2
#define LEXERR 3
#define SEMI 4
#define DSEMI 5
#define AMPER 6
#define INPAR 7
#define INBRACE 8
#define OUTPAR 9
#define DBAR 10
#define DAMPER 11
#define BANG 12
#define OUTBRACE 13
#define OUTANG 14
#define OUTANGBANG 15
#define DOUTANG 16
#define DOUTANGBANG 17
#define INANG 18
#define DINANG 19
#define DINANGDASH 20
#define INANGAMP 21
#define OUTANGAMP 22
#define OUTANGAMPBANG 23
#define DOUTANGAMP 24
#define DOUTANGAMPBANG 25
#define TRINANG 26
#define BAR 27
#define BARAMP 28
#define DINBRACK 29
#define DOUTBRACK 30
#define STRING 31
#define ENVSTRING 32
#define ENVARRAY 33
#define ENDINPUT 34
#define INOUTPAR 35
#define DO 36
#define DONE 37
#define ESAC 38
#define THEN 39
#define ELIF 40
#define ELSE 41
#define FI 42
#define FOR 43
#define CASE 44
#define IF 45
#define WHILE 46
#define FUNC 47
#define REPEAT 48
#define TIME 49
#define UNTIL 50
#define EXEC 51
#define COMMAND 52
#define SELECT 53
#define COPROC 54
#define NOGLOB 55
#define DASH 56
#define NOCORRECT 57
#define FOREACH 58
#define ZEND 59

#define WRITE 0
#define WRITENOW 1
#define APP 2
#define APPNOW 3
#define MERGEOUT 4
#define MERGEOUTNOW 5
#define ERRAPP 6
#define ERRAPPNOW 7
#define READ 8
#define HEREDOC 9
#define MERGE 10
#define CLOSE 11
#define INPIPE 12
#define OUTPIPE 13
#define HERESTR 14
#define NONE 15
#define HEREDOCDASH 16

#ifdef GLOBALS
int redirtab[TRINANG-OUTANG+1] = {
	WRITE,
	WRITENOW,
	APP,
	APPNOW,
	READ,
	HEREDOC,
	HEREDOCDASH,
	MERGE,
	MERGEOUT,
	MERGEOUTNOW,
	ERRAPP,
	ERRAPPNOW,
	HERESTR,
};
#else
int redirtab[TRINANG-OUTANG+1];
#endif

#define IS_READFD(X) (((X)>=READ && (X)<=MERGE)||(X)==HEREDOCDASH||(X)==HERESTR)
#define IS_REDIROP(X) ((X)>=OUTANG && (X)<=TRINANG)
#define IS_ERROR_REDIR(X) ((X)>=MERGEOUT && (X)<=ERRAPPNOW)
#define UN_ERROR_REDIR(X) ((X)-MERGEOUT+WRITE)

EXTERN char **environ;

typedef struct hashtab *Hashtab;
typedef struct hashnode *Hashnode;
typedef struct schedcmd *Schedcmd;
typedef struct alias *Alias;
typedef struct process *Process;
typedef struct job *Job;
typedef struct value *Value;
typedef struct arrind *Arrind;
typedef struct varasg *Varasg;
typedef struct param *Param;
typedef struct cmdnam *Cmdnam;
typedef struct cond *Cond;
typedef struct cmd *Cmd;
typedef struct pline *Pline;
typedef struct sublist *Sublist;
typedef struct list *List;
typedef struct lklist *Lklist;
typedef struct lknode *Lknode;
typedef struct comp *Comp;
typedef struct redir *Redir;
typedef struct complist *Complist;
typedef struct heap *Heap;
typedef void (*FFunc)DCLPROTO((vptr));
typedef vptr (*VFunc)DCLPROTO((vptr));
typedef void (*HFunc)DCLPROTO((char *,char *));

/* linked list abstract data type */

struct lknode;
struct lklist;

struct lknode {
   Lknode next,last;
   vptr dat;
   };
struct lklist {
   Lknode first,last;
   };

#define addnode(X,Y) insnode(X,(X)->last,Y)
#define full(X) ((X)->first != NULL)
#define firstnode(X) ((X)->first)
#define getaddrdata(X) (&((X)->dat))
#define getdata(X) ((X)->dat)
#define setdata(X,Y) ((X)->dat = (Y))
#define lastnode(X) ((X)->last)
#define nextnode(X) ((X)->next)
#define prevnode(X) ((X)->last)
#define peekfirst(X) ((X)->first->dat)
#define pushnode(X,Y) insnode(X,(Lknode) X,Y)
#define incnode(X) (X = nextnode(X))

/* node structure for syntax trees */

/* struct list, struct sublist, struct pline, etc.  all fit the form
	of this structure and are used interchangably
*/

struct node {
	int data[4];			/* arbitrary integer data */
	vptr ptrs[4];			/* arbitrary pointer data */
	int types[4];			/* what ptrs[] are pointing to */
	int type;				/* node type */
	};

#define N_LIST 0
#define N_SUBLIST 1
#define N_PLINE 2
#define N_CMD 3
#define N_REDIR 4
#define N_COND 5
#define N_FOR 6
#define N_CASE 7
#define N_IF 8
#define N_WHILE 9
#define N_VARASG 10
#define N_COUNT 11

/* values for types[4] */

#define NT_EMPTY 0
#define NT_NODE  1
#define NT_STR   2
#define NT_LIST  4
#define NT_MALLOC 8

/* tree element for lists */

struct list {
	int type;
	int ifil[3];		/* to fit struct node */
   Sublist left;
   List right;
   };

#define SYNC 0		/* ; */
#define ASYNC 1	/* & */
#define TIMED 2

/* tree element for sublists */

struct sublist {
	int type;
	int flags;			/* see PFLAGs below */
	int ifil[2];
	Pline left;
	Sublist right;
	};

#define ORNEXT 10		/* || */
#define ANDNEXT 11	/* && */

#define PFLAG_NOT 1			/* ! ... */
#define PFLAG_COPROC 32		/* coproc ... */

/* tree element for pipes */

struct pline {
	int type;
	int ifil[3];
   Cmd left;
   Pline right;
   };

#define END		0	/* pnode *right is null */
#define PIPE	1	/* pnode *right is the rest of the pipeline */

/* tree element for commands */

struct cmd {
	int type;
	int flags;				/* see CFLAGs below */
	int ifil[2];
   Lklist args;			/* command & argmument List (char *'s) */
	union {
   	List list;			/* for SUBSH/CURSH/SHFUNC */
		struct forcmd *forcmd;
		struct casecmd *casecmd;
		struct ifcmd *ifcmd;
		struct whilecmd *whilecmd;
		Sublist pline;
		Cond cond;
		} u;
   Lklist redir;			/* i/o redirections (struct redir *'s) */
	Lklist vars;			/* param assignments (struct varasg *'s) */
   };

#define SIMPLE 0
#define SUBSH 1
#define ZCTIME 2
#define CURSH 3
#define FUNCDEF 4
#define CFOR 5
#define CWHILE 6
#define CREPEAT 7
#define CIF 8
#define CCASE 9
#define CSELECT 10
#define COND 11

#define CFLAG_EXEC 1			/* exec ... */
#define CFLAG_COMMAND 2		/* command ... */
#define CFLAG_NOGLOB 4     /* noglob ... */
#define CFLAG_DASH 8			/* - ... */

/* tree element for redirection lists */

struct redir {
	int type,fd1,fd2,ifil;
	char *name;
   };

/* tree element for conditionals */

struct cond {
	int type;		/* can be cond_type, or a single letter (-a, -b, ...) */
	int ifil[3];
	vptr left,right,vfil[2];
	int types[4],typ;	/* from struct node.  DO NOT REMOVE */
	};

#define COND_NOT 0
#define COND_AND 1
#define COND_OR 2
#define COND_STREQ 3
#define COND_STRNEQ 4
#define COND_STRLT 5
#define COND_STRGTR 6
#define COND_NT 7
#define COND_OT 8
#define COND_EF 9
#define COND_EQ 10
#define COND_NE 11
#define COND_LT 12
#define COND_GT 13
#define COND_LE 14
#define COND_GE 15

struct forcmd {		/* for/select */
							/* Cmd->args contains list of words to loop thru */
	int inflag;			/* if there is an in ... clause */
	int ifil[3];
	char *name;			/* parameter to assign values to */
	List list;			/* list to look through for each name */
	};
struct casecmd {
							/* Cmd->args contains word to test */
	int ifil[4];
	struct casecmd *next;
	char *pat;
	List list;					/* list to execute */
	};

/*

	a command like "if foo then bar elif baz then fubar else fooble"
	generates a tree like:

	struct ifcmd a = { next =  &b,  ifl = "foo", thenl = "bar" }
	struct ifcmd b = { next =  &c,  ifl = "baz", thenl = "fubar" }
	struct ifcmd c = { next = NULL, ifl = NULL, thenl = "fooble" }

*/

struct ifcmd {
	int ifil[4];
	struct ifcmd *next;
	List ifl;
	List thenl;
	};

struct whilecmd {
	int cond;		/* 0 for while, 1 for until */
	int ifil[3];
	List cont;		/* condition */
	List loop;		/* list to execute until condition met */
	};

/* structure used for multiple i/o redirection */
/* one for each fd open */

struct multio {
	int ct;				/* # of redirections on this fd */
	int rflag;			/* 0 if open for reading, 1 if open for writing */
	int pipe;			/* fd of pipe if ct > 1 */
	int fds[NOFILE];	/* list of src/dests redirected to/from this fd */
   }; 

/* node used in command path hash table (cmdnamtab) */

struct cmdnam 
{
	int type,flags;
	int ifil[2];
	union {
		char *nam;		/* full pathname if type != BUILTIN */
		int binnum;		/* func to exec if type == BUILTIN */
		List list;		/* list to exec if type == SHFUNC */
		} u;
	};

#define EXCMD_PREDOT 0
#define EXCMD_POSTDOT 1
#define BUILTIN 2
#define SHFUNC 3
#define DISABLED 4
#define ISEXCMD(X) ((X)==EXCMD_PREDOT||(X)==EXCMD_POSTDOT)

/* node used in parameter hash table (paramtab) */

struct param {
	union {
		char **arr;		/* value if declared array */
		char *str;		/* value if declared string (scalar) */
		long val;		/* value if declared integer */
		} u;
	union {				/* functions to call to set value */
		void (*cfn)DCLPROTO((Param,char *));
		void (*ifn)DCLPROTO((Param,long));
		void (*afn)DCLPROTO((Param,char **));
		} sets;
	union {				/* functions to call to get value */
		char *(*cfn)DCLPROTO((Param));
		long (*ifn)DCLPROTO((Param));
		char **(*afn)DCLPROTO((Param));
		} gets;
	int ct;				/* output base or field width */
	int flags;
	vptr data;			/* used by getfns */
	char *env;			/* location in environment, if exported */
	char *ename;		/* name of corresponding environment var */
	};

#define PMFLAG_s 0		/* scalar */
#define PMFLAG_L 1		/* left justify and remove leading blanks */
#define PMFLAG_R 2		/* right justify and fill with leading blanks */
#define PMFLAG_Z 4		/* right justify and fill with leading zeros */
#define PMFLAG_i 8		/* integer */
#define PMFLAG_l 16		/* all lower case */
#define PMFLAG_u 32		/* all upper case */
#define PMFLAG_r 64		/* readonly */
#define PMFLAG_t 128		/* tagged */
#define PMFLAG_x 256		/* exported */
#define PMFLAG_A 512		/* array */
#define PMFLAG_SPECIAL	1024
#define PMTYPE (PMFLAG_i|PMFLAG_A)
#define pmtype(X) ((X)->flags & PMTYPE)

/* variable assignment tree element */

struct varasg {
	int type;			/* nonzero means array */
	int ifil[3];
	char *name;
	char *str;			/* should've been a union here.  oh well */
	Lklist arr;
	};

/* lvalue for variable assignment/expansion */

struct value {
	int isarr;
	struct param *pm;		/* parameter node */
	int a;					/* first element of array slice, or -1 */
	int b;					/* last element of array slice, or -1 */
	};

struct fdpair {
	int fd1,fd2;
	};

/* tty state structure */

struct ttyinfo {
#ifdef TERMIOS
	struct termios termios;
#else
#ifdef TERMIO
	struct termio termio;
#else
	struct sgttyb sgttyb;
	int lmodes;
	struct tchars tchars;
	struct ltchars ltchars;
#endif
#endif
#ifdef TIOCGWINSZ
	struct winsize winsize;
#endif
	};

EXTERN struct ttyinfo savedttyinfo;

/* entry in job table */

struct job {
	long gleader;					/* process group leader of this job */
	int stat;
	char *cwd;						/* current working dir of shell when
											this job was spawned */
	struct process *procs;		/* list of processes */
	Lklist filelist;				/* list of files to delete when done */
	};

#define STAT_CHANGED 1		/* status changed and not reported */
#define STAT_STOPPED 2		/* all procs stopped or exited */
#define STAT_TIMED 4			/* job is being timed */
#define STAT_DONE 8
#define STAT_LOCKED 16		/* shell is finished creating this job,
										may be deleted from job table */
#define STAT_INUSE 64		/* this job entry is in use */

#define SP_RUNNING -1		/* fake statusp for running jobs */

/* node in job process lists */

struct process {
	struct process *next;
	long pid;
	char *text;						/* text to print when 'jobs' is run */
	int statusp;					/* return code from wait3() */
	int lastfg;						/* set if this process represents a
											fragment of a pipeline run in a subshell
											for commands like:

											foo | bar | ble

											where foo is a current shell function
											or control structure.  The command
											actually executed is:

											foo | (bar | ble)

											That's two procnodes in the parent
											shell, the latter having this flag set. */
	struct rusage ru;
	time_t bgtime;					/* time job was spawned */
	time_t endtime;				/* time job exited */
	};

/* node in alias hash table */

struct alias {
	char *text;			/* expansion of alias */
	int cmd;				/* one for regular aliases,
								zero for global aliases,
								negative for reserved words */
	int inuse;			/* alias is being expanded */
	};

/* node in sched list */

struct schedcmd {
	struct schedcmd *next;
	char *cmd;		/* command to run */
	time_t time;	/* when to run it */
	};

#define MAXAL 20	/* maximum number of aliases expanded at once */

/* hash table node */

struct hashnode {
	struct hashnode *next;
	char *nam;
	vptr dat;
	int canfree;		/* nam is free()able */
	};

/* hash table */

struct hashtab {
	int hsize;							/* size of nodes[] */
	int ct;								/* # of elements */
	struct hashnode **nodes;		/* array of size hsize */
	};

extern char *sys_errlist[];
extern int errno;

/* values in opts[] array */

#define OPT_INVALID 1	/* opt is invalid, like -$ */
#define OPT_UNSET 0
#define OPT_SET 2

/* the options */

struct option {
	char *name;
	char id;			/* corresponding letter */
	};

#define CORRECT '0'
#define NOCLOBBER '1'
#define NOBADPATTERN '2'
#define NONOMATCH '3'
#define GLOBDOTS '4'
#define NOTIFY '5'
#define BGNICE '6'
#define IGNOREEOF '7'
#define MARKDIRS '8'
#define AUTOLIST '9'
#define NOBEEP 'B'
#define PRINTEXITVALUE 'C'
#define PUSHDTOHOME 'D'
#define PUSHDSILENT 'E'
#define NOGLOBOPT 'F'
#define NULLGLOB 'G'
#define RMSTARSILENT 'H'
#define IGNOREBRACES 'I'
#define AUTOCD 'J'
#define NOBANGHIST 'K'
#define SUNKEYBOARDHACK 'L'
#define SINGLELINEZLE 'M'
#define AUTOPUSHD 'N'
#define CORRECTALL 'O'
#define RCEXPANDPARAM 'P'
#define PATHDIRS 'Q'
#define LONGLISTJOBS 'R'
#define RECEXACT 'S'
#define CDABLEVARS 'T'
#define MAILWARNING 'U'
#define NOPROMPTCLOBBER 'V'
#define AUTORESUME 'W'
#define LISTTYPES 'X'
#define MENUCOMPLETE 'Y'
#define USEZLE 'Z'
#define ALLEXPORT 'a'
#define ERREXIT 'e'
#define NORCS 'f'
#define HISTIGNORESPACE 'g'
#define HISTIGNOREDUPS 'h'
#define INTERACTIVE 'i'
#define HISTLIT 'j'
#define INTERACTIVECOMMENTS 'k'
#define LOGINSHELL 'l'
#define MONITOR 'm'
#define NOEXEC 'n'
#define SHINSTDIN 's'
#define NOUNSET 'u'
#define VERBOSE 'v'
#define CHASELINKS 'w'
#define XTRACE 'x'
#define SHWORDSPLIT 'y'
#define MENUCOMPLETEBEEP '\2'
#define HISTNOSTORE '\3'
#define EXTENDEDGLOB '\5'
#define GLOBCOMPLETE '\6'
#define CSHJUNKIEQUOTES '\7'
#define PUSHDMINUS '\10'
#define CSHJUNKIELOOPS '\11'
#define RCQUOTES '\12'
#define KSHOPTIONPRINT '\13'
#define NOSHORTLOOPS '\14'
#define LASTMENU '\15'
#define AUTOMENU '\16'
#define HISTVERIFY '\17'
#define NOLISTBEEP '\20'
#define NOHUP '\21'
#define NOEQUALS '\22'
#define CSHNULLGLOB '\23'

#ifndef GLOBALS
extern struct option optns[];
#else
struct option optns[] = {
	"correct",CORRECT,
	"noclobber",NOCLOBBER,
	"nobadpattern",NOBADPATTERN,
	"nonomatch",NONOMATCH,
	"globdots",GLOBDOTS,
	"notify",NOTIFY,
	"bgnice",BGNICE,
	"ignoreeof",IGNOREEOF,
	"markdirs",MARKDIRS,
	"autolist",AUTOLIST,
	"nobeep",NOBEEP,
	"printexitvalue",PRINTEXITVALUE,
	"pushdtohome",PUSHDTOHOME,
	"pushdsilent",PUSHDSILENT,
	"noglob",NOGLOBOPT,
	"nullglob",NULLGLOB,
	"rmstarsilent",RMSTARSILENT,
	"ignorebraces",IGNOREBRACES,
	"autocd",AUTOCD,
	"nobanghist",NOBANGHIST,
	"sunkeyboardhack",SUNKEYBOARDHACK,
	"singlelinezle",SINGLELINEZLE,
	"autopushd",AUTOPUSHD,
	"correctall",CORRECTALL,
	"rcexpandparam",RCEXPANDPARAM,
	"pathdirs",PATHDIRS,
	"longlistjobs",LONGLISTJOBS,
	"recexact",RECEXACT,
	"cdablevars",CDABLEVARS,
	"mailwarning",MAILWARNING,
	"nopromptclobber",NOPROMPTCLOBBER,
	"autoresume",AUTORESUME,
	"listtypes",LISTTYPES,
	"menucomplete",MENUCOMPLETE,
	"zle",USEZLE,
	"allexport",ALLEXPORT,
	"errexit",ERREXIT,
	"norcs",NORCS,
	"histignorespace",HISTIGNORESPACE,
	"histignoredups",HISTIGNOREDUPS,
	"interactive",INTERACTIVE,
	"histlit",HISTLIT,
	"interactivecomments",INTERACTIVECOMMENTS,
	"login",LOGINSHELL,
	"monitor",MONITOR,
	"noexec",NOEXEC,
	"shinstdin",SHINSTDIN,
	"nounset",NOUNSET,
	"verbose",VERBOSE,
	"chaselinks",CHASELINKS,
	"xtrace",XTRACE,
	"shwordsplit",SHWORDSPLIT,
	"menucompletebeep",MENUCOMPLETEBEEP,
	"histnostore",HISTNOSTORE,
	"extendedglob",EXTENDEDGLOB,
	"globcomplete",GLOBCOMPLETE,
	"cshjunkiequotes",CSHJUNKIEQUOTES,
	"pushdminus",PUSHDMINUS,
	"cshjunkieloops",CSHJUNKIELOOPS,
	"rcquotes",RCQUOTES,
	"noshortloops",NOSHORTLOOPS,
	"lastmenu",LASTMENU,
	"automenu",AUTOMENU,
	"histverify",HISTVERIFY,
	"nolistbeep",NOLISTBEEP,
	"nohup",NOHUP,
	"noequals",NOEQUALS,
	"kshoptionprint",KSHOPTIONPRINT,
	"cshnullglob",CSHNULLGLOB,
	NULL,0
};
#endif

#define ALSTAT_MORE 1	/* last alias ended with ' ' */
#define ALSTAT_JUNK 2	/* don't put word in history List */

#undef isset
#define isset(X) (opts[X])
#define unset(X) (!opts[X])
#define interact (isset(INTERACTIVE))
#define jobbing (isset(MONITOR))
#define nointr() signal(SIGINT,SIG_IGN)
#define islogin (isset(LOGINSHELL))

#undef WIFSTOPPED
#undef WIFSIGNALED
#undef WIFEXITED
#undef WEXITSTATUS
#undef WTERMSIG
#undef WSTOPSIG
#undef WCOREDUMPED

#define SP(X) (X)
#define WIFSTOPPED(X) (((X)&0377)==0177)
#define WIFSIGNALED(X) (((X)&0377)!=0&&((X)&0377)!=0177)
#define WIFEXITED(X) (((X)&0377)==0)
#define WEXITSTATUS(X) (((X)>>8)&0377)
#define WTERMSIG(X) ((X)&0177)
#define WSTOPSIG(X) (((X)>>8)&0377)
#define WCOREDUMPED(X) ((X)&0200)

#ifndef S_ISBLK
#define	_IFMT		0170000
#define	_IFDIR	0040000
#define	_IFCHR	0020000
#define	_IFBLK	0060000
#define	_IFREG	0100000
#define	_IFIFO	0010000
#define	S_ISBLK(m)	(((m)&_IFMT) == _IFBLK)
#define	S_ISCHR(m)	(((m)&_IFMT) == _IFCHR)
#define	S_ISDIR(m)	(((m)&_IFMT) == _IFDIR)
#define	S_ISFIFO(m)	(((m)&_IFMT) == _IFIFO)
#define	S_ISREG(m)	(((m)&_IFMT) == _IFREG)
#endif

#ifndef S_ISSOCK
#ifndef _IFMT
#define _IFMT 0170000
#endif
#define	_IFLNK	0120000
#define	_IFSOCK	0140000
#define	S_ISLNK(m)	(((m)&_IFMT) == _IFLNK)
#define	S_ISSOCK(m)	(((m)&_IFMT) == _IFSOCK)
#endif

#if S_IFIFO == S_IFSOCK
#undef S_IFIFO
#endif

/* buffered shell input for non-interactive shells */

EXTERN FILE *bshin;

/* NULL-terminated arrays containing path, cdpath, etc. */

EXTERN char **path,**cdpath,**fpath,**watch,**mailpath,**tildedirs,**fignore;
EXTERN char **hosts,**hostcmds,**optcmds,**bindcmds,**varcmds;

/* named directories */

EXTERN char **userdirs,**usernames;

/* size of userdirs[], # of userdirs */

EXTERN int userdirsz,userdirct;

EXTERN char *mailfile;

EXTERN char *yytext;

/* error/break flag */

EXTERN int errflag;

EXTERN char *tokstr;
EXTERN int tok, tokfd;

/* lexical analyzer error flag */

EXTERN int lexstop;

/* suppress error messages */

EXTERN int noerrs;

/* current history event number */

EXTERN int curhist;

/* if != 0, this is the first line of the command */

EXTERN int isfirstln;

/* if != 0, this is the first char of the command (not including
	white space */

EXTERN int isfirstch;

/* first event number in the history lists */

EXTERN int firsthist,firstlithist;

/* capacity of history lists */

EXTERN int histsiz,lithistsiz;

/* if = 1, we have performed history substitution on the current line
 	if = 2, we have used the 'p' modifier */

EXTERN int histdone;

/* default event (usually curhist-1, that is, "!!") */

EXTERN int defev;

/* != 0 if we are about to read a command word */

EXTERN int incmdpos;

/* != 0 if we are in the middle of a [[ ... ]] */

EXTERN int incond;

/* != 0 if we are about to read a case pattern */

EXTERN int incasepat;

/* != 0 if we just read FUNCTION */

EXTERN int infunc;

/* != 0 if we just read a newline */

EXTERN int isnewlin;

/* the lists of history events */

EXTERN Lklist histlist,lithistlist;

/* the directory stack */

EXTERN Lklist dirstack;

/* the zle buffer stack */

EXTERN Lklist bufstack;

/* the input queue (stack?)

	inbuf    = start of buffer
	inbufptr = location in buffer (= inbuf for a FULL buffer)
											(= inbuf+inbufsz for an EMPTY buffer)
	inbufct  = # of chars in buffer (inbufptr+inbufct == inbuf+inbufsz)
	inbufsz  = max size of buffer
*/

EXTERN char *inbuf,*inbufptr;
EXTERN int inbufct,inbufsz;

EXTERN char *ifs;		/* $IFS */

EXTERN char *oldpwd;	/* $OLDPWD */

EXTERN char *underscore; /* $_ */

/* != 0 if this is a subshell */

EXTERN int subsh;

/* # of break levels */

EXTERN int breaks;

/* != 0 if we have a return pending */

EXTERN int retflag;

/* != 0 means don't hash the PATH */

EXTERN int pathsuppress;

/* # of nested loops we are in */

EXTERN int loops;

/* # of continue levels */

EXTERN int contflag;

/* the job we are working on */

EXTERN int thisjob;

/* the current job (+) */

EXTERN int curjob;

/* the previous job (-) */

EXTERN int prevjob;

/* hash table containing the aliases and reserved words */

EXTERN Hashtab aliastab;

/* hash table containing the parameters */

EXTERN Hashtab paramtab;

/* hash table containing the builtins/shfuncs/hashed commands */

EXTERN Hashtab cmdnamtab;

/* hash table containing the zle multi-character bindings */

EXTERN Hashtab xbindtab;

/* the job table */

EXTERN struct job jobtab[MAXJOB];

/* the list of sched jobs pending */

EXTERN struct schedcmd *schedcmds;

/* the last l for s/l/r/ history substitution */

EXTERN char *hsubl;

/* the last r for s/l/r/ history substitution */

EXTERN char *hsubr;

EXTERN char *username;	/* $USERNAME */
EXTERN long lastval;		/* $? */
EXTERN long baud;			/* $BAUD */
EXTERN long columns;		/* $COLUMNS */
EXTERN long lines;		/* $LINES */

/* input fd from the coprocess */

EXTERN int coprocin;

/* output fd from the coprocess */

EXTERN int coprocout;

EXTERN long mailcheck;	/* $MAILCHECK */
EXTERN long logcheck;	/* $LOGCHECK */

/* the last time we checked mail */

EXTERN time_t lastmailcheck;

/* the last time we checked the people in the WATCH variable */

EXTERN time_t lastwatch;

/* the last time we did the periodic() shell function */

EXTERN time_t lastperiod;

/* $SECONDS = time(NULL) - shtimer */

EXTERN time_t shtimer;

EXTERN long mypid;		/* $$ */
EXTERN long lastpid;		/* $! */
EXTERN long ppid;			/* $PPID */

/* the process group of the shell */

EXTERN long mypgrp;

EXTERN char *cwd;			/* $PWD */
EXTERN char *optarg;		/* $OPTARG */
EXTERN long optind;		/* $OPTIND */
EXTERN char *prompt;		/* $PROMPT */
EXTERN char *rprompt;	/* $RPROMPT */
EXTERN char *prompt2;	/* etc. */
EXTERN char *prompt3;
EXTERN char *prompt4;
EXTERN char *sprompt;
EXTERN char *timefmt;
EXTERN char *watchfmt;
EXTERN char *wordchars;
EXTERN char *fceditparam;
EXTERN char *tmpprefix;

EXTERN char *argzero;	/* $0 */

EXTERN char *hackzero;

/* the hostname */

EXTERN char *hostnam;

EXTERN char *home;		/* $HOME */
EXTERN char **pparams;	/* $argv */

/* the default command for null commands */

EXTERN char *nullcmd;

/* the List of local variables we have to destroy */

EXTERN Lklist locallist;

/* the shell input fd */

EXTERN int SHIN;

/* the shell tty fd */

EXTERN int SHTTY;

/* the stack of aliases we are expanding */

EXTERN struct alias *alstack[MAXAL];

/* the alias stack pointer; also, the number of aliases currently
 	being expanded */

EXTERN int alstackind;

/* != 0 means we are reading input from a string */

EXTERN int strin;

/* period between periodic() commands, in seconds */

EXTERN long period;

/* != 0 means history substitution is turned off */

EXTERN int stophist;

EXTERN int lithist;

/* this line began with a space, so junk it if HISTIGNORESPACE is on */

EXTERN int spaceflag;

/* don't do spelling correction */

EXTERN int nocorrect;

/* != 0 means we have removed the current event from the history List */

EXTERN int histremmed;

/* the options; e.g. if opts['a'] == OPT_SET, -a is turned on */

EXTERN int opts[128];

EXTERN long lineno;		/* LINENO */
EXTERN long listmax;		/* LISTMAX */
EXTERN long savehist;	/* SAVEHIST */
EXTERN long shlvl;		/* SHLVL */
EXTERN long tmout;		/* TMOUT */
EXTERN long dirstacksize;	/* DIRSTACKSIZE */

/* != 0 means we have called execlist() and then intend to exit(),
 	so don't fork if not necessary */

EXTERN int exiting;

EXTERN int lastbase;		/* last input base we used */

/* the limits for child processes */

#ifdef RLIM_INFINITY
EXTERN struct rlimit limits[RLIM_NLIMITS];
#endif

/* the current word in the history List */

EXTERN char *hlastw;

/* pointer into the history line */

EXTERN char *hptr;

/* the current history line */

EXTERN char *hline;

/* the termcap buffer */

EXTERN char termbuf[1024];

/* $TERM */

EXTERN char *term;

/* != 0 if this $TERM setup is usable */

EXTERN int termok;

/* flag for CSHNULLGLOB */

EXTERN int badcshglob;

/* max size of hline */

EXTERN int hlinesz;

/* the alias expansion status - if == ALSTAT_MORE, we just finished
	expanding an alias ending with a space */

EXTERN int alstat;

/* we have printed a 'you have stopped (running) jobs.' message */

EXTERN int stopmsg;

/* the default tty state */

EXTERN struct ttyinfo shttyinfo;

/* $TTY */

EXTERN char *ttystrname;

/* list of memory heaps */

EXTERN Lklist heaplist;

/* != 0 if we are allocating in the heaplist */

EXTERN int useheap;

#include "signals.h"

#ifdef GLOBALS

/* signal names */
char **sigptr = sigs;

/* tokens */
char *ztokens = "#$^*()$=|{}[]`<>?~`,";

#else
extern char *ztokens,**sigptr;
#endif

#define SIGERR (SIGCOUNT+1)
#define SIGDEBUG (SIGCOUNT+2)
#define VSIGCOUNT (SIGCOUNT+3)
#define SIGEXIT 0

/* signals that are trapped = 1, signals ignored =2 */

EXTERN int sigtrapped[VSIGCOUNT];

/* trap functions for each signal */

EXTERN List sigfuncs[VSIGCOUNT];

/* $HISTCHARS */

EXTERN char bangchar,hatchar,hashchar;

EXTERN int eofseen;

/* we are parsing a line sent to use by the editor */

EXTERN int zleparse;

EXTERN int wordbeg;

EXTERN int parbegin;

/* interesting termcap strings */

#define TCCLEARSCREEN 0
#define TCLEFT 1
#define TCMULTLEFT 2
#define TCRIGHT 3
#define TCMULTRIGHT 4
#define TCUP 5
#define TCMULTUP 6
#define TCDOWN 7
#define TCMULTDOWN 8
#define TCDEL 9
#define TCMULTDEL 10
#define TCINS 11
#define TCMULTINS 12
#define TCCLEAREOD 13
#define TCCLEAREOL 14
#define TCINSLINE 15
#define TCDELLINE 16
#define TC_COUNT 17

/* lengths of each string */

EXTERN int tclen[TC_COUNT];

EXTERN char *tcstr[TC_COUNT];

#ifdef GLOBALS

/* names of the strings we want */

char *tccapnams[TC_COUNT] = {
	"cl","le","LE","nd","RI","up","UP","do",
	"DO","dc","DC","ic","IC","cd","ce","al","dl"
	};

#else
extern char *tccapnams[TC_COUNT];
#endif

#define tccan(X) (!!tclen[X])

#define HISTFLAG_DONE   1
#define HISTFLAG_NOEXEC 2

#include "ztype.h"
#include "funcs.h"

#ifdef __hpux
#define setpgrp setpgid
#endif

#define _INCLUDE_POSIX_SOURCE
#define _INCLUDE_XOPEN_SOURCE
#define _INCLUDE_HPUX_SOURCE

#ifdef SV_BSDSIG
#define SV_INTERRUPT SV_BSDSIG
#endif

#ifdef SYSV
#define blockchld() sighold(SIGCHLD)
#define unblockchld() sigrelse(SIGCHLD)
#define chldsuspend() sigpause(SIGCHLD)
#else
#define blockchld() sigblock(sigmask(SIGCHLD))
#define unblockchld() sigsetmask(0)
#define chldsuspend() sigpause(0)
#endif
