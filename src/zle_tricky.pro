int usetab DCLPROTO((void));
void completeword DCLPROTO((void));
void menucompleteword DCLPROTO((void));
void listchoices DCLPROTO((void));
void spellword DCLPROTO((void));
void deletecharorlist DCLPROTO((void));
void expandword DCLPROTO((void));
void expandorcomplete DCLPROTO((void));
void menuexpandorcomplete DCLPROTO((void));
void listexpand DCLPROTO((void));
void reversemenucomplete DCLPROTO((void));
void acceptandmenucomplete DCLPROTO((void));
void docomplete DCLPROTO((int lst));
void do_menucmp DCLPROTO((int lst));
char *get_comp_string DCLPROTO((void));
void doexpansion DCLPROTO((char *s,int lst,int lincmd));
void gotword DCLPROTO((char *s));
void inststrlen DCLPROTO((char *s,int l));
void addmatch DCLPROTO((char *s));
void addcmdmatch DCLPROTO((char *s,char *t));
void addcmdnodis DCLPROTO((char *s,char *t));
void maketildelist DCLPROTO((char	*s));
int Isdir DCLPROTO((char *s));
int isdir DCLPROTO((char *t,char *s));
void docompletion DCLPROTO((char *s,int lst,int incmd));
void gen_matches_glob DCLPROTO((char *s,int incmd));
void gen_matches_reg DCLPROTO((char *s,int incmd));
void do_fignore DCLPROTO((char *origstr));
void do_ambiguous DCLPROTO((char *s));
void do_single DCLPROTO((char *s));
void do_ambig_menu DCLPROTO((char *s));
int strpfx DCLPROTO((char *s,char *t));
int pfxlen DCLPROTO((char *s,char *t));
void listmatches DCLPROTO((Lklist l,char *apps));
void selectlist DCLPROTO((Lklist l));
int doexpandhist DCLPROTO((void));
void magicspace DCLPROTO((void));
void expandhistory DCLPROTO((void));
char *getcurcmd DCLPROTO((void));
void processcmd DCLPROTO((void));
void expandcmdpath DCLPROTO((void));
void freemenu DCLPROTO((void));
int inarray DCLPROTO((char *s, char **a));