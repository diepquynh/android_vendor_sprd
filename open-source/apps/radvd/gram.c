#ifndef lint
static const char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20101229

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)

#define YYPREFIX "yy"

#define YYPURE 0

#line 16 "gram.y"
#include "config.h"
#include "includes.h"
#include "radvd.h"
#include "defaults.h"

extern struct Interface *IfaceList;
struct Interface *iface = NULL;
struct AdvPrefix *prefix = NULL;
struct AdvRoute *route = NULL;
struct AdvRDNSS *rdnss = NULL;
struct AdvDNSSL *dnssl = NULL;

extern char *conf_file;
extern int num_lines;
extern char *yytext;

static void cleanup(void);
static void yyerror(char *msg);
static int countbits(int b);
static int count_mask(struct sockaddr_in6 *m);
static struct in6_addr get_prefix6(struct in6_addr const *addr, struct in6_addr const *mask);

#if 0 /* no longer necessary? */
#ifndef HAVE_IN6_ADDR_S6_ADDR
# ifdef __FreeBSD__
#  define s6_addr32 __u6_addr.__u6_addr32
#  define s6_addr16 __u6_addr.__u6_addr16
# endif
#endif
#endif

#define ABORT	do { cleanup(); YYABORT; } while (0);
#define ADD_TO_LL(type, list, value) \
	do { \
		if (iface->list == NULL) \
			iface->list = value; \
		else { \
			type *current = iface->list; \
			while (current->next != NULL) \
				current = current->next; \
			current->next = value; \
		} \
	} while (0)


#line 136 "gram.y"
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union {
	unsigned int		num;
	int			snum;
	double			dec;
	struct in6_addr		*addr;
	char			*str;
	struct AdvPrefix	*pinfo;
	struct AdvRoute		*rinfo;
	struct AdvRDNSS		*rdnssinfo;
	struct AdvDNSSL		*dnsslinfo;
	struct Clients		*ainfo;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 85 "gram.tab.c"
/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

/* Parameters sent to yyerror. */
#define YYERROR_DECL() yyerror(const char *s)
#define YYERROR_CALL(msg) yyerror(msg)

extern int YYPARSE_DECL();

#define T_INTERFACE 257
#define T_PREFIX 258
#define T_ROUTE 259
#define T_RDNSS 260
#define T_DNSSL 261
#define T_CLIENTS 262
#define STRING 263
#define NUMBER 264
#define SIGNEDNUMBER 265
#define DECIMAL 266
#define SWITCH 267
#define IPV6ADDR 268
#define INFINITY 269
#define T_IgnoreIfMissing 270
#define T_AdvSendAdvert 271
#define T_MaxRtrAdvInterval 272
#define T_MinRtrAdvInterval 273
#define T_MinDelayBetweenRAs 274
#define T_AdvManagedFlag 275
#define T_AdvOtherConfigFlag 276
#define T_AdvLinkMTU 277
#define T_AdvReachableTime 278
#define T_AdvRetransTimer 279
#define T_AdvCurHopLimit 280
#define T_AdvDefaultLifetime 281
#define T_AdvDefaultPreference 282
#define T_AdvSourceLLAddress 283
#define T_AdvOnLink 284
#define T_AdvAutonomous 285
#define T_AdvValidLifetime 286
#define T_AdvPreferredLifetime 287
#define T_DeprecatePrefix 288
#define T_DecrementLifetimes 289
#define T_AdvRouterAddr 290
#define T_AdvHomeAgentFlag 291
#define T_AdvIntervalOpt 292
#define T_AdvHomeAgentInfo 293
#define T_Base6Interface 294
#define T_Base6to4Interface 295
#define T_UnicastOnly 296
#define T_HomeAgentPreference 297
#define T_HomeAgentLifetime 298
#define T_AdvRoutePreference 299
#define T_AdvRouteLifetime 300
#define T_RemoveRoute 301
#define T_AdvRDNSSPreference 302
#define T_AdvRDNSSOpenFlag 303
#define T_AdvRDNSSLifetime 304
#define T_FlushRDNSS 305
#define T_AdvDNSSLLifetime 306
#define T_FlushDNSSL 307
#define T_AdvMobRtrSupportFlag 308
#define T_BAD_TOKEN 309
#define YYERRCODE 256
static const short yylhs[] = {                           -1,
    0,    0,    9,   10,    1,   11,   11,   12,   12,   12,
   12,   12,   12,   13,   13,   13,   13,   13,   13,   13,
   13,   13,   13,   13,   13,   13,   13,   13,   13,   13,
   13,   13,   13,   13,   13,   13,   13,    3,    4,    4,
    2,   14,   15,   15,   15,   16,   16,   17,   17,   17,
   17,   17,   17,   17,   17,   17,    5,   18,   19,   19,
   20,   20,   21,   21,   21,    6,   24,   24,   25,   22,
   23,   23,   26,   26,   27,   27,   27,   27,    7,   30,
   30,   31,   28,   29,   29,   32,   32,   33,   33,    8,
    8,
};
static const short yylen[] = {                            2,
    2,    1,    5,    2,    1,    0,    2,    1,    1,    1,
    1,    1,    1,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    5,    2,    3,
    3,    4,    0,    2,    3,    2,    1,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    5,    4,    0,    1,
    2,    1,    3,    3,    3,    5,    2,    1,    1,    2,
    0,    1,    2,    1,    3,    3,    3,    3,    5,    2,
    1,    1,    2,    0,    1,    2,    1,    3,    3,    1,
    1,
};
static const short yydefred[] = {                         0,
    0,    0,    2,    0,    5,    4,    1,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    9,   10,   11,   12,   13,    0,
    0,    8,    0,    0,    0,    0,    0,    0,   69,    0,
   68,   82,    0,   81,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    7,    0,    0,    0,    0,    0,    0,    0,   67,   80,
    0,    0,   20,   21,   15,   18,   14,   17,   16,   19,
   22,   23,   24,   25,   26,   29,   27,   28,   30,   33,
   31,   32,   36,   34,   35,   37,    3,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   44,    0,   47,   41,
    0,    0,    0,    0,    0,   62,    0,    0,    0,    0,
    0,    0,   74,    0,    0,    0,    0,   87,   42,   58,
   39,    0,    0,    0,    0,   90,   91,    0,    0,    0,
    0,    0,    0,    0,   45,   46,    0,    0,    0,    0,
   61,    0,    0,    0,    0,    0,   73,    0,    0,    0,
   86,   40,   38,   48,   49,   51,   52,   53,   54,   50,
   55,   56,   63,   64,   65,   57,   75,   76,   77,   78,
   66,   88,   89,   79,
};
static const short yydgoto[] = {                          2,
    6,   35,   36,   92,   37,   38,   39,  158,    3,    4,
   40,   41,   42,   43,   83,  128,  129,   44,  134,  135,
  136,   45,  141,   50,   51,  142,  143,   46,  146,   53,
   54,  147,  148,
};
static const short yysindex[] = {                      -224,
 -225, -224,    0,  -79,    0,    0,    0, -256, -220, -218,
 -215, -205,  -64, -207, -206, -253, -221, -217, -204, -203,
 -202, -199, -198, -197, -196, -195, -194, -193, -192, -191,
 -190, -186, -185, -187,    0,    0,    0,    0,    0,  -56,
 -256,    0,  -52,  -51,  -42,  -41,   36,   37,    0, -215,
    0,    0, -205,    0, -183,   27,   28,   29,   30,   31,
   32,   33,   34,   35,   38,   39,   40,   41,   42,   43,
   44,   45,   46,   47,   48,   49,   50,   51,   52,   53,
    0, -125,   54, -269, -295, -252, -169, -168,    0,    0,
   55, -124,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -152, -151, -235,
 -235, -150, -149, -148, -225, -225,    0, -113,    0,    0,
 -145, -235, -146,   -3, -269,    0, -141, -143, -235, -142,
    1, -295,    0, -235, -140,    3, -252,    0,    0,    0,
    0,   70,   71,   72,   73,    0,    0,   74,   75,   76,
   77,   78,   79,   80,    0,    0,   81,   82,   83,   84,
    0,   86,   87,   88,   89,   90,    0,   91,   92,   93,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,
};
static const short yyrindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,   58,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   58,    0,   94,    0,    0,    0,    0,    0,    0,   56,
    0,    0,   57,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   59,   60,   61,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   62,    0,    0,    0,    0,    0,
    0,   63,    0,    0,    0,    0,   64,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,
};
static const short yygindex[] = {                         0,
  -69,    0,    0,    0,    0,    0,    0,  -93,  152,    0,
  114,    0,    0,    0,    0,    0,   65,    0,    0,    0,
   21,    0,    0,    0,  107,    0,   16,    0,    0,    0,
  113,    0,   20,
};
#define YYTABLESIZE 193
static const short yytable[] = {                        127,
  153,    9,   10,   11,   12,   13,  137,  138,  139,  140,
   58,  165,   59,   14,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,   26,   27,  159,  156,  131,
  132,  133,    1,  157,   28,   29,   30,    5,  168,   31,
   32,   33,   60,    8,   61,  174,   62,   47,   63,   48,
  178,   34,   49,  144,  145,  163,  164,   52,   55,   56,
   57,   66,   64,   65,   67,   68,   69,   70,   80,   71,
   82,   84,   72,   73,   74,   75,   76,   77,   78,   79,
   85,   86,   87,   88,   91,   93,   94,   95,   96,   97,
   98,   99,  100,  101,  149,  150,  102,  103,  104,  105,
  106,  107,  108,  109,  110,  111,  112,  113,  114,  115,
  116,  117,  130,  151,  154,  155,  160,  161,  162,  167,
  169,  170,  172,  173,  175,  176,  179,  180,  182,  183,
  184,  185,  186,  187,  188,  189,  190,  191,  192,  193,
  194,  195,  196,  152,  197,  198,  199,  200,  201,  202,
  203,  204,   43,    7,   81,  171,   89,  177,  118,  119,
  120,  121,  122,  123,  124,   90,  181,    0,  125,  126,
  118,  119,  120,  121,  122,  123,  124,    0,   70,   83,
  125,  126,    6,   59,   71,   84,   60,   72,   85,    0,
    0,    0,  166,
};
static const short yycheck[] = {                        125,
  125,  258,  259,  260,  261,  262,  302,  303,  304,  305,
  264,  125,  266,  270,  271,  272,  273,  274,  275,  276,
  277,  278,  279,  280,  281,  282,  283,  121,  264,  299,
  300,  301,  257,  269,  291,  292,  293,  263,  132,  296,
  297,  298,  264,  123,  266,  139,  264,  268,  266,  268,
  144,  308,  268,  306,  307,  125,  126,  263,  123,  267,
  267,  264,  267,  267,  264,  264,  264,  264,  125,  265,
  123,  123,  267,  267,  267,  267,  267,  264,  264,  267,
  123,  123,   47,   47,  268,   59,   59,   59,   59,   59,
   59,   59,   59,   59,  264,  264,   59,   59,   59,   59,
   59,   59,   59,   59,   59,   59,   59,   59,   59,   59,
   59,   59,   59,   59,  267,  267,  267,  267,  267,  265,
  267,  125,  264,  267,  267,  125,  267,  125,   59,   59,
   59,   59,   59,   59,   59,   59,   59,   59,   59,   59,
   59,   59,   59,  268,   59,   59,   59,   59,   59,   59,
   59,   59,   59,    2,   41,  135,   50,  142,  284,  285,
  286,  287,  288,  289,  290,   53,  147,   -1,  294,  295,
  284,  285,  286,  287,  288,  289,  290,   -1,  123,  123,
  294,  295,  125,  125,  125,  125,  125,  125,  125,   -1,
   -1,   -1,  128,
};
#define YYFINAL 2
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 309
#if YYDEBUG
static const char *yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,"'/'",0,0,0,0,0,0,0,0,0,0,0,"';'",0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"T_INTERFACE",
"T_PREFIX","T_ROUTE","T_RDNSS","T_DNSSL","T_CLIENTS","STRING","NUMBER",
"SIGNEDNUMBER","DECIMAL","SWITCH","IPV6ADDR","INFINITY","T_IgnoreIfMissing",
"T_AdvSendAdvert","T_MaxRtrAdvInterval","T_MinRtrAdvInterval",
"T_MinDelayBetweenRAs","T_AdvManagedFlag","T_AdvOtherConfigFlag","T_AdvLinkMTU",
"T_AdvReachableTime","T_AdvRetransTimer","T_AdvCurHopLimit",
"T_AdvDefaultLifetime","T_AdvDefaultPreference","T_AdvSourceLLAddress",
"T_AdvOnLink","T_AdvAutonomous","T_AdvValidLifetime","T_AdvPreferredLifetime",
"T_DeprecatePrefix","T_DecrementLifetimes","T_AdvRouterAddr",
"T_AdvHomeAgentFlag","T_AdvIntervalOpt","T_AdvHomeAgentInfo","T_Base6Interface",
"T_Base6to4Interface","T_UnicastOnly","T_HomeAgentPreference",
"T_HomeAgentLifetime","T_AdvRoutePreference","T_AdvRouteLifetime",
"T_RemoveRoute","T_AdvRDNSSPreference","T_AdvRDNSSOpenFlag",
"T_AdvRDNSSLifetime","T_FlushRDNSS","T_AdvDNSSLLifetime","T_FlushDNSSL",
"T_AdvMobRtrSupportFlag","T_BAD_TOKEN",
};
static const char *yyrule[] = {
"$accept : grammar",
"grammar : grammar ifacedef",
"grammar : ifacedef",
"ifacedef : ifacehead '{' ifaceparams '}' ';'",
"ifacehead : T_INTERFACE name",
"name : STRING",
"ifaceparams :",
"ifaceparams : ifaceparam ifaceparams",
"ifaceparam : ifaceval",
"ifaceparam : prefixdef",
"ifaceparam : clientslist",
"ifaceparam : routedef",
"ifaceparam : rdnssdef",
"ifaceparam : dnssldef",
"ifaceval : T_MinRtrAdvInterval NUMBER ';'",
"ifaceval : T_MaxRtrAdvInterval NUMBER ';'",
"ifaceval : T_MinDelayBetweenRAs NUMBER ';'",
"ifaceval : T_MinRtrAdvInterval DECIMAL ';'",
"ifaceval : T_MaxRtrAdvInterval DECIMAL ';'",
"ifaceval : T_MinDelayBetweenRAs DECIMAL ';'",
"ifaceval : T_IgnoreIfMissing SWITCH ';'",
"ifaceval : T_AdvSendAdvert SWITCH ';'",
"ifaceval : T_AdvManagedFlag SWITCH ';'",
"ifaceval : T_AdvOtherConfigFlag SWITCH ';'",
"ifaceval : T_AdvLinkMTU NUMBER ';'",
"ifaceval : T_AdvReachableTime NUMBER ';'",
"ifaceval : T_AdvRetransTimer NUMBER ';'",
"ifaceval : T_AdvDefaultLifetime NUMBER ';'",
"ifaceval : T_AdvDefaultPreference SIGNEDNUMBER ';'",
"ifaceval : T_AdvCurHopLimit NUMBER ';'",
"ifaceval : T_AdvSourceLLAddress SWITCH ';'",
"ifaceval : T_AdvIntervalOpt SWITCH ';'",
"ifaceval : T_AdvHomeAgentInfo SWITCH ';'",
"ifaceval : T_AdvHomeAgentFlag SWITCH ';'",
"ifaceval : T_HomeAgentPreference NUMBER ';'",
"ifaceval : T_HomeAgentLifetime NUMBER ';'",
"ifaceval : T_UnicastOnly SWITCH ';'",
"ifaceval : T_AdvMobRtrSupportFlag SWITCH ';'",
"clientslist : T_CLIENTS '{' v6addrlist '}' ';'",
"v6addrlist : IPV6ADDR ';'",
"v6addrlist : v6addrlist IPV6ADDR ';'",
"prefixdef : prefixhead optional_prefixplist ';'",
"prefixhead : T_PREFIX IPV6ADDR '/' NUMBER",
"optional_prefixplist :",
"optional_prefixplist : '{' '}'",
"optional_prefixplist : '{' prefixplist '}'",
"prefixplist : prefixplist prefixparms",
"prefixplist : prefixparms",
"prefixparms : T_AdvOnLink SWITCH ';'",
"prefixparms : T_AdvAutonomous SWITCH ';'",
"prefixparms : T_AdvRouterAddr SWITCH ';'",
"prefixparms : T_AdvValidLifetime number_or_infinity ';'",
"prefixparms : T_AdvPreferredLifetime number_or_infinity ';'",
"prefixparms : T_DeprecatePrefix SWITCH ';'",
"prefixparms : T_DecrementLifetimes SWITCH ';'",
"prefixparms : T_Base6Interface name ';'",
"prefixparms : T_Base6to4Interface name ';'",
"routedef : routehead '{' optional_routeplist '}' ';'",
"routehead : T_ROUTE IPV6ADDR '/' NUMBER",
"optional_routeplist :",
"optional_routeplist : routeplist",
"routeplist : routeplist routeparms",
"routeplist : routeparms",
"routeparms : T_AdvRoutePreference SIGNEDNUMBER ';'",
"routeparms : T_AdvRouteLifetime number_or_infinity ';'",
"routeparms : T_RemoveRoute SWITCH ';'",
"rdnssdef : rdnsshead '{' optional_rdnssplist '}' ';'",
"rdnssaddrs : rdnssaddrs rdnssaddr",
"rdnssaddrs : rdnssaddr",
"rdnssaddr : IPV6ADDR",
"rdnsshead : T_RDNSS rdnssaddrs",
"optional_rdnssplist :",
"optional_rdnssplist : rdnssplist",
"rdnssplist : rdnssplist rdnssparms",
"rdnssplist : rdnssparms",
"rdnssparms : T_AdvRDNSSPreference NUMBER ';'",
"rdnssparms : T_AdvRDNSSOpenFlag SWITCH ';'",
"rdnssparms : T_AdvRDNSSLifetime number_or_infinity ';'",
"rdnssparms : T_FlushRDNSS SWITCH ';'",
"dnssldef : dnsslhead '{' optional_dnsslplist '}' ';'",
"dnsslsuffixes : dnsslsuffixes dnsslsuffix",
"dnsslsuffixes : dnsslsuffix",
"dnsslsuffix : STRING",
"dnsslhead : T_DNSSL dnsslsuffixes",
"optional_dnsslplist :",
"optional_dnsslplist : dnsslplist",
"dnsslplist : dnsslplist dnsslparms",
"dnsslplist : dnsslparms",
"dnsslparms : T_AdvDNSSLLifetime number_or_infinity ';'",
"dnsslparms : T_FlushDNSSL SWITCH ';'",
"number_or_infinity : NUMBER",
"number_or_infinity : INFINITY",

};
#endif
/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH  500
#endif
#endif

#define YYINITSTACKSIZE 500

int      yydebug;
int      yynerrs;

typedef struct {
    unsigned stacksize;
    short    *s_base;
    short    *s_mark;
    short    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;

/* variables for the parser stack */
static YYSTACKDATA yystack;
#line 917 "gram.y"

static
int countbits(int b)
{
	int count;

	for (count = 0; b != 0; count++) {
		b &= b - 1; // this clears the LSB-most set bit
	}

	return (count);
}

static
int count_mask(struct sockaddr_in6 *m)
{
	struct in6_addr *in6 = &m->sin6_addr;
	int i;
	int count = 0;

	for (i = 0; i < 16; ++i) {
		count += countbits(in6->s6_addr[i]);
	}
	return count;
}

static
struct in6_addr get_prefix6(struct in6_addr const *addr, struct in6_addr const *mask)
{
	struct in6_addr prefix = *addr;
	int i = 0;

	for (; i < 16; ++i) {
		prefix.s6_addr[i] &= mask->s6_addr[i];
	}

	return prefix;
}

static
void cleanup(void)
{
	if (iface)
		free(iface);

	if (prefix)
		free(prefix);

	if (route)
		free(route);

	if (rdnss)
		free(rdnss);

	if (dnssl) {
		int i;
		for (i = 0;i < dnssl->AdvDNSSLNumber;i++)
			free(dnssl->AdvDNSSLSuffixes[i]);
		free(dnssl->AdvDNSSLSuffixes);
		free(dnssl);
	}
}

static void
yyerror(char *msg)
{
	cleanup();
	flog(LOG_ERR, "%s in %s, line %d: %s", msg, conf_file, num_lines, yytext);
}
#line 546 "gram.tab.c"

#if YYDEBUG
#include <stdio.h>		/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = data->s_mark - data->s_base;
    newss = (short *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return -1;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return -1;

    data->l_base = newvs;
    data->l_mark = newvs + i;

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab

int
YYPARSE_DECL()
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack)) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    yyerror("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yystack.s_mark]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
                {
                    goto yyoverflow;
                }
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
                --yystack.s_mark;
                --yystack.l_mark;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 3:
#line 156 "gram.y"
	{
			struct Interface *iface2;

			iface2 = IfaceList;
			while (iface2)
			{
				if (!strcmp(iface2->Name, iface->Name))
				{
					flog(LOG_ERR, "duplicate interface "
						"definition for %s", iface->Name);
					ABORT;
				}
				iface2 = iface2->next;
			}

			if (check_device(iface) < 0) {
				if (iface->IgnoreIfMissing) {
					dlog(LOG_DEBUG, 4, "interface %s did not exist, ignoring the interface", iface->Name);
				}
				else {
					flog(LOG_ERR, "interface %s does not exist", iface->Name);
					ABORT;
				}
			}
			if (setup_deviceinfo(iface) < 0)
				if (!iface->IgnoreIfMissing)
				ABORT;
			if (check_iface(iface) < 0)
				if (!iface->IgnoreIfMissing)
				ABORT;
			if (setup_linklocal_addr(iface) < 0)
				if (!iface->IgnoreIfMissing)
				ABORT;
			if (setup_allrouters_membership(iface) < 0)
				if (!iface->IgnoreIfMissing)
				ABORT;

			dlog(LOG_DEBUG, 4, "interface definition for %s is ok", iface->Name);

			iface->next = IfaceList;
			IfaceList = iface;

			iface = NULL;
		}
break;
case 4:
#line 202 "gram.y"
	{
			iface = malloc(sizeof(struct Interface));

			if (iface == NULL) {
				flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
				ABORT;
			}

			iface_init_defaults(iface);
			strncpy(iface->Name, yystack.l_mark[0].str, IFNAMSIZ-1);
			iface->Name[IFNAMSIZ-1] = '\0';
		}
break;
case 5:
#line 217 "gram.y"
	{
			/* check vality */
			yyval.str = yystack.l_mark[0].str;
		}
break;
case 9:
#line 229 "gram.y"
	{ ADD_TO_LL(struct AdvPrefix, AdvPrefixList, yystack.l_mark[0].pinfo); }
break;
case 10:
#line 230 "gram.y"
	{ ADD_TO_LL(struct Clients, ClientList, yystack.l_mark[0].ainfo); }
break;
case 11:
#line 231 "gram.y"
	{ ADD_TO_LL(struct AdvRoute, AdvRouteList, yystack.l_mark[0].rinfo); }
break;
case 12:
#line 232 "gram.y"
	{ ADD_TO_LL(struct AdvRDNSS, AdvRDNSSList, yystack.l_mark[0].rdnssinfo); }
break;
case 13:
#line 233 "gram.y"
	{ ADD_TO_LL(struct AdvDNSSL, AdvDNSSLList, yystack.l_mark[0].dnsslinfo); }
break;
case 14:
#line 237 "gram.y"
	{
			iface->MinRtrAdvInterval = yystack.l_mark[-1].num;
		}
break;
case 15:
#line 241 "gram.y"
	{
			iface->MaxRtrAdvInterval = yystack.l_mark[-1].num;
		}
break;
case 16:
#line 245 "gram.y"
	{
			iface->MinDelayBetweenRAs = yystack.l_mark[-1].num;
		}
break;
case 17:
#line 249 "gram.y"
	{
			iface->MinRtrAdvInterval = yystack.l_mark[-1].dec;
		}
break;
case 18:
#line 253 "gram.y"
	{
			iface->MaxRtrAdvInterval = yystack.l_mark[-1].dec;
		}
break;
case 19:
#line 257 "gram.y"
	{
			iface->MinDelayBetweenRAs = yystack.l_mark[-1].dec;
		}
break;
case 20:
#line 261 "gram.y"
	{
			iface->IgnoreIfMissing = yystack.l_mark[-1].num;
		}
break;
case 21:
#line 265 "gram.y"
	{
			iface->AdvSendAdvert = yystack.l_mark[-1].num;
		}
break;
case 22:
#line 269 "gram.y"
	{
			iface->AdvManagedFlag = yystack.l_mark[-1].num;
		}
break;
case 23:
#line 273 "gram.y"
	{
			iface->AdvOtherConfigFlag = yystack.l_mark[-1].num;
		}
break;
case 24:
#line 277 "gram.y"
	{
			iface->AdvLinkMTU = yystack.l_mark[-1].num;
		}
break;
case 25:
#line 281 "gram.y"
	{
			iface->AdvReachableTime = yystack.l_mark[-1].num;
		}
break;
case 26:
#line 285 "gram.y"
	{
			iface->AdvRetransTimer = yystack.l_mark[-1].num;
		}
break;
case 27:
#line 289 "gram.y"
	{
			iface->AdvDefaultLifetime = yystack.l_mark[-1].num;
		}
break;
case 28:
#line 293 "gram.y"
	{
			iface->AdvDefaultPreference = yystack.l_mark[-1].snum;
		}
break;
case 29:
#line 297 "gram.y"
	{
			iface->AdvCurHopLimit = yystack.l_mark[-1].num;
		}
break;
case 30:
#line 301 "gram.y"
	{
			iface->AdvSourceLLAddress = yystack.l_mark[-1].num;
		}
break;
case 31:
#line 305 "gram.y"
	{
			iface->AdvIntervalOpt = yystack.l_mark[-1].num;
		}
break;
case 32:
#line 309 "gram.y"
	{
			iface->AdvHomeAgentInfo = yystack.l_mark[-1].num;
		}
break;
case 33:
#line 313 "gram.y"
	{
			iface->AdvHomeAgentFlag = yystack.l_mark[-1].num;
		}
break;
case 34:
#line 317 "gram.y"
	{
			iface->HomeAgentPreference = yystack.l_mark[-1].num;
		}
break;
case 35:
#line 321 "gram.y"
	{
			iface->HomeAgentLifetime = yystack.l_mark[-1].num;
		}
break;
case 36:
#line 325 "gram.y"
	{
			iface->UnicastOnly = yystack.l_mark[-1].num;
		}
break;
case 37:
#line 329 "gram.y"
	{
			iface->AdvMobRtrSupportFlag = yystack.l_mark[-1].num;
		}
break;
case 38:
#line 335 "gram.y"
	{
			yyval.ainfo = yystack.l_mark[-2].ainfo;
		}
break;
case 39:
#line 341 "gram.y"
	{
			struct Clients *new = calloc(1, sizeof(struct Clients));
			if (new == NULL) {
				flog(LOG_CRIT, "calloc failed: %s", strerror(errno));
				ABORT;
			}

			memcpy(&(new->Address), yystack.l_mark[-1].addr, sizeof(struct in6_addr));
			yyval.ainfo = new;
		}
break;
case 40:
#line 352 "gram.y"
	{
			struct Clients *new = calloc(1, sizeof(struct Clients));
			if (new == NULL) {
				flog(LOG_CRIT, "calloc failed: %s", strerror(errno));
				ABORT;
			}

			memcpy(&(new->Address), yystack.l_mark[-1].addr, sizeof(struct in6_addr));
			new->next = yystack.l_mark[-2].ainfo;
			yyval.ainfo = new;
		}
break;
case 41:
#line 367 "gram.y"
	{
			if (prefix) {
				unsigned int dst;

				if (prefix->AdvPreferredLifetime > prefix->AdvValidLifetime)
				{
					flog(LOG_ERR, "AdvValidLifeTime must be "
						"greater than AdvPreferredLifetime in %s, line %d",
						conf_file, num_lines);
					ABORT;
				}

				if ( prefix->if6[0] && prefix->if6to4[0]) {
					flog(LOG_ERR, "Base6Interface and Base6to4Interface are mutually exclusive at this time.");
					ABORT;
				}

				if ( prefix->if6to4[0] )
				{
					if (get_v4addr(prefix->if6to4, &dst) < 0)
					{
						flog(LOG_ERR, "interface %s has no IPv4 addresses, disabling 6to4 prefix", prefix->if6to4 );
						prefix->enabled = 0;
					}
					else
					{
						*((uint16_t *)(prefix->Prefix.s6_addr)) = htons(0x2002);
						memcpy( prefix->Prefix.s6_addr + 2, &dst, sizeof( dst ) );
					}
				}

				if ( prefix->if6[0] )
				{
#ifndef HAVE_IFADDRS_H
					flog(LOG_ERR, "Base6Interface not supported in %s, line %d", conf_file, num_lines);
					ABORT;
#else
					struct ifaddrs *ifap = 0, *ifa = 0;
					struct AdvPrefix *next = prefix->next;

					if (prefix->PrefixLen != 64) {
						flog(LOG_ERR, "Only /64 is allowed with Base6Interface.  %s:%d", conf_file, num_lines);
						ABORT;
					}

					if (getifaddrs(&ifap) != 0)
						flog(LOG_ERR, "getifaddrs failed: %s", strerror(errno));

					for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
						struct sockaddr_in6 *s6 = 0;
						struct sockaddr_in6 *mask = (struct sockaddr_in6 *)ifa->ifa_netmask;
						struct in6_addr base6prefix;
						char buf[INET6_ADDRSTRLEN];
						int i;

						if (strncmp(ifa->ifa_name, prefix->if6, IFNAMSIZ))
							continue;

						if (ifa->ifa_addr->sa_family != AF_INET6)
							continue;

						s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);

						if (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr))
							continue;

						base6prefix = get_prefix6(&s6->sin6_addr, &mask->sin6_addr);
						for (i = 0; i < 8; ++i) {
							prefix->Prefix.s6_addr[i] &= ~mask->sin6_addr.s6_addr[i];
							prefix->Prefix.s6_addr[i] |= base6prefix.s6_addr[i];
						}
						memset(&prefix->Prefix.s6_addr[8], 0, 8);
						prefix->AdvRouterAddr = 1;
						prefix->AutoSelected = 1;
						prefix->next = next;

						if (inet_ntop(ifa->ifa_addr->sa_family, (void *)&(prefix->Prefix), buf, sizeof(buf)) == NULL)
							flog(LOG_ERR, "%s: inet_ntop failed in %s, line %d!", ifa->ifa_name, conf_file, num_lines);
						else
							dlog(LOG_DEBUG, 3, "auto-selected prefix %s/%d on interface %s from interface %s",
								buf, prefix->PrefixLen, iface->Name, ifa->ifa_name);

						/* Taking only one prefix from the Base6Interface.  Taking more than one would require allocating new
						   prefixes and building a list.  I'm not sure how to do that from here. So for now, break. */
						break;
					}

					if (ifap)
						freeifaddrs(ifap);
#endif /* ifndef HAVE_IFADDRS_H */
				}
			}
			yyval.pinfo = prefix;
			prefix = NULL;
		}
break;
case 42:
#line 465 "gram.y"
	{
			struct in6_addr zeroaddr;
			memset(&zeroaddr, 0, sizeof(zeroaddr));

			if (!memcmp(yystack.l_mark[-2].addr, &zeroaddr, sizeof(struct in6_addr))) {
#ifndef HAVE_IFADDRS_H
				flog(LOG_ERR, "invalid all-zeros prefix in %s, line %d", conf_file, num_lines);
				ABORT;
#else
				struct ifaddrs *ifap = 0, *ifa = 0;
				struct AdvPrefix *next = iface->AdvPrefixList;

				while (next) {
					if (next->AutoSelected) {
						flog(LOG_ERR, "auto selecting prefixes works only once per interface.  See %s, line %d", conf_file, num_lines);
						ABORT;
					}
					next = next->next;
				}
				next = 0;

				dlog(LOG_DEBUG, 5, "all-zeros prefix in %s, line %d, parsing..", conf_file, num_lines);

				if (getifaddrs(&ifap) != 0)
					flog(LOG_ERR, "getifaddrs failed: %s", strerror(errno));

				for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
					struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
					struct sockaddr_in6 *mask = (struct sockaddr_in6 *)ifa->ifa_netmask;
					char buf[INET6_ADDRSTRLEN];

					if (strncmp(ifa->ifa_name, iface->Name, IFNAMSIZ))
						continue;

					if (ifa->ifa_addr->sa_family != AF_INET6)
						continue;

					s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);

					if (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr))
						continue;

					prefix = malloc(sizeof(struct AdvPrefix));

					if (prefix == NULL) {
						flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
						ABORT;
					}

					prefix_init_defaults(prefix);
					prefix->Prefix = get_prefix6(&s6->sin6_addr, &mask->sin6_addr);
					prefix->AdvRouterAddr = 1;
					prefix->AutoSelected = 1;
					prefix->next = next;
					next = prefix;

					if (prefix->PrefixLen == 0)
						prefix->PrefixLen = count_mask(mask);

					if (inet_ntop(ifa->ifa_addr->sa_family, (void *)&(prefix->Prefix), buf, sizeof(buf)) == NULL)
						flog(LOG_ERR, "%s: inet_ntop failed in %s, line %d!", ifa->ifa_name, conf_file, num_lines);
					else
						dlog(LOG_DEBUG, 3, "auto-selected prefix %s/%d on interface %s", buf, prefix->PrefixLen, ifa->ifa_name);
				}

				if (!prefix) {
					flog(LOG_WARNING, "no auto-selected prefix on interface %s, disabling advertisements",  iface->Name);
				}

				if (ifap)
					freeifaddrs(ifap);
#endif /* ifndef HAVE_IFADDRS_H */
			}
			else {
				prefix = malloc(sizeof(struct AdvPrefix));

				if (prefix == NULL) {
					flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
					ABORT;
				}

				prefix_init_defaults(prefix);

				if (yystack.l_mark[0].num > MAX_PrefixLen)
				{
					flog(LOG_ERR, "invalid prefix length in %s, line %d", conf_file, num_lines);
					ABORT;
				}

				prefix->PrefixLen = yystack.l_mark[0].num;

				memcpy(&prefix->Prefix, yystack.l_mark[-2].addr, sizeof(struct in6_addr));
			}
		}
break;
case 48:
#line 571 "gram.y"
	{
			if (prefix) {
				if (prefix->AutoSelected) {
					struct AdvPrefix *p = prefix;
					do {
						p->AdvOnLinkFlag = yystack.l_mark[-1].num;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					prefix->AdvOnLinkFlag = yystack.l_mark[-1].num;
			}
		}
break;
case 49:
#line 585 "gram.y"
	{
			if (prefix) {
				if (prefix->AutoSelected) {
					struct AdvPrefix *p = prefix;
					do {
						p->AdvAutonomousFlag = yystack.l_mark[-1].num;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					prefix->AdvAutonomousFlag = yystack.l_mark[-1].num;
			}
		}
break;
case 50:
#line 599 "gram.y"
	{
			if (prefix) {
				if (prefix->AutoSelected && yystack.l_mark[-1].num == 0)
					flog(LOG_WARNING, "prefix automatically selected, AdvRouterAddr always enabled, ignoring config line %d", num_lines);
				else
					prefix->AdvRouterAddr = yystack.l_mark[-1].num;
			}
		}
break;
case 51:
#line 608 "gram.y"
	{
			if (prefix) {
				if (prefix->AutoSelected) {
					struct AdvPrefix *p = prefix;
					do {
						p->AdvValidLifetime = yystack.l_mark[-1].num;
						p->curr_validlft = yystack.l_mark[-1].num;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					prefix->AdvValidLifetime = yystack.l_mark[-1].num;
					prefix->curr_validlft = yystack.l_mark[-1].num;
			}
		}
break;
case 52:
#line 624 "gram.y"
	{
			if (prefix) {
				if (prefix->AutoSelected) {
					struct AdvPrefix *p = prefix;
					do {
						p->AdvPreferredLifetime = yystack.l_mark[-1].num;
						p->curr_preferredlft = yystack.l_mark[-1].num;
						p = p->next;
					} while (p && p->AutoSelected);
				}
				else
					prefix->AdvPreferredLifetime = yystack.l_mark[-1].num;
					prefix->curr_preferredlft = yystack.l_mark[-1].num;
			}
		}
break;
case 53:
#line 640 "gram.y"
	{
			prefix->DeprecatePrefixFlag = yystack.l_mark[-1].num;
		}
break;
case 54:
#line 644 "gram.y"
	{
			prefix->DecrementLifetimesFlag = yystack.l_mark[-1].num;
		}
break;
case 55:
#line 648 "gram.y"
	{
			if (prefix) {
				if (prefix->AutoSelected) {
					flog(LOG_ERR, "automatically selecting the prefix and Base6to4Interface are mutually exclusive");
					ABORT;
				} /* fallthrough */
				dlog(LOG_DEBUG, 4, "using prefixes on interface %s for prefixes on interface %s", yystack.l_mark[-1].str, iface->Name);
				strncpy(prefix->if6, yystack.l_mark[-1].str, IFNAMSIZ-1);
				prefix->if6[IFNAMSIZ-1] = '\0';
			}
		}
break;
case 56:
#line 661 "gram.y"
	{
			if (prefix) {
				if (prefix->AutoSelected) {
					flog(LOG_ERR, "automatically selecting the prefix and Base6to4Interface are mutually exclusive");
					ABORT;
				} /* fallthrough */
				dlog(LOG_DEBUG, 4, "using interface %s for 6to4 prefixes on interface %s", yystack.l_mark[-1].str, iface->Name);
				strncpy(prefix->if6to4, yystack.l_mark[-1].str, IFNAMSIZ-1);
				prefix->if6to4[IFNAMSIZ-1] = '\0';
			}
		}
break;
case 57:
#line 675 "gram.y"
	{
			yyval.rinfo = route;
			route = NULL;
		}
break;
case 58:
#line 683 "gram.y"
	{
			route = malloc(sizeof(struct AdvRoute));

			if (route == NULL) {
				flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
				ABORT;
			}

			route_init_defaults(route, iface);

			if (yystack.l_mark[0].num > MAX_PrefixLen)
			{
				flog(LOG_ERR, "invalid route prefix length in %s, line %d", conf_file, num_lines);
				ABORT;
			}

			route->PrefixLen = yystack.l_mark[0].num;

			memcpy(&route->Prefix, yystack.l_mark[-2].addr, sizeof(struct in6_addr));
		}
break;
case 63:
#line 716 "gram.y"
	{
			route->AdvRoutePreference = yystack.l_mark[-1].snum;
		}
break;
case 64:
#line 720 "gram.y"
	{
			route->AdvRouteLifetime = yystack.l_mark[-1].num;
		}
break;
case 65:
#line 724 "gram.y"
	{
			route->RemoveRouteFlag = yystack.l_mark[-1].num;
		}
break;
case 66:
#line 730 "gram.y"
	{
			yyval.rdnssinfo = rdnss;
			rdnss = NULL;
		}
break;
case 69:
#line 741 "gram.y"
	{
			if (!rdnss) {
				/* first IP found */
				rdnss = malloc(sizeof(struct AdvRDNSS));

				if (rdnss == NULL) {
					flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
					ABORT;
				}

				rdnss_init_defaults(rdnss, iface);
			}

			switch (rdnss->AdvRDNSSNumber) {
				case 0:
					memcpy(&rdnss->AdvRDNSSAddr1, yystack.l_mark[0].addr, sizeof(struct in6_addr));
					rdnss->AdvRDNSSNumber++;
					break;
				case 1:
					memcpy(&rdnss->AdvRDNSSAddr2, yystack.l_mark[0].addr, sizeof(struct in6_addr));
					rdnss->AdvRDNSSNumber++;
					break;
				case 2:
					memcpy(&rdnss->AdvRDNSSAddr3, yystack.l_mark[0].addr, sizeof(struct in6_addr));
					rdnss->AdvRDNSSNumber++;
					break;
				default:
					flog(LOG_CRIT, "Too many addresses in RDNSS section");
					ABORT;
			}

		}
break;
case 70:
#line 776 "gram.y"
	{
			if (!rdnss) {
				flog(LOG_CRIT, "No address specified in RDNSS section");
				ABORT;
			}
		}
break;
case 75:
#line 794 "gram.y"
	{
			flog(LOG_WARNING, "Ignoring deprecated RDNSS preference.");
		}
break;
case 76:
#line 798 "gram.y"
	{
			flog(LOG_WARNING, "Ignoring deprecated RDNSS open flag.");
		}
break;
case 77:
#line 802 "gram.y"
	{
			if (yystack.l_mark[-1].num < iface->MaxRtrAdvInterval && yystack.l_mark[-1].num != 0) {
				flog(LOG_ERR, "AdvRDNSSLifetime must be at least MaxRtrAdvInterval");
				ABORT;
			}
			if (yystack.l_mark[-1].num > 2*(iface->MaxRtrAdvInterval))
				flog(LOG_WARNING, "Warning: AdvRDNSSLifetime <= 2*MaxRtrAdvInterval would allow stale DNS servers to be deleted faster");

			rdnss->AdvRDNSSLifetime = yystack.l_mark[-1].num;
		}
break;
case 78:
#line 813 "gram.y"
	{
			rdnss->FlushRDNSSFlag = yystack.l_mark[-1].num;
		}
break;
case 79:
#line 819 "gram.y"
	{
			yyval.dnsslinfo = dnssl;
			dnssl = NULL;
		}
break;
case 82:
#line 830 "gram.y"
	{
			char *ch;
			for (ch = yystack.l_mark[0].str;*ch != '\0';ch++) {
				if (*ch >= 'A' && *ch <= 'Z')
					continue;
				if (*ch >= 'a' && *ch <= 'z')
					continue;
				if (*ch >= '0' && *ch <= '9')
					continue;
				if (*ch == '-' || *ch == '.')
					continue;

				flog(LOG_CRIT, "Invalid domain suffix specified");
				ABORT;
			}

			if (!dnssl) {
				/* first domain found */
				dnssl = malloc(sizeof(struct AdvDNSSL));

				if (dnssl == NULL) {
					flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
					ABORT;
				}

				dnssl_init_defaults(dnssl, iface);
			}

			dnssl->AdvDNSSLNumber++;
			dnssl->AdvDNSSLSuffixes =
				realloc(dnssl->AdvDNSSLSuffixes,
					dnssl->AdvDNSSLNumber * sizeof(char*));
			if (dnssl->AdvDNSSLSuffixes == NULL) {
				flog(LOG_CRIT, "realloc failed: %s", strerror(errno));
				ABORT;
			}

			dnssl->AdvDNSSLSuffixes[dnssl->AdvDNSSLNumber - 1] = strdup(yystack.l_mark[0].str);
		}
break;
case 83:
#line 872 "gram.y"
	{
			if (!dnssl) {
				flog(LOG_CRIT, "No domain specified in DNSSL section");
				ABORT;
			}
		}
break;
case 88:
#line 890 "gram.y"
	{
			if (yystack.l_mark[-1].num < iface->MaxRtrAdvInterval && yystack.l_mark[-1].num != 0) {
				flog(LOG_ERR, "AdvDNSSLLifetime must be at least MaxRtrAdvInterval");
				ABORT;
			}
			if (yystack.l_mark[-1].num > 2*(iface->MaxRtrAdvInterval))
				flog(LOG_WARNING, "Warning: AdvDNSSLLifetime <= 2*MaxRtrAdvInterval would allow stale DNS suffixes to be deleted faster");

			dnssl->AdvDNSSLLifetime = yystack.l_mark[-1].num;
		}
break;
case 89:
#line 901 "gram.y"
	{
			dnssl->FlushDNSSLFlag = yystack.l_mark[-1].num;
		}
break;
case 90:
#line 907 "gram.y"
	{
				yyval.num = yystack.l_mark[0].num;
			}
break;
case 91:
#line 911 "gram.y"
	{
				yyval.num = (uint32_t)~0;
			}
break;
#line 1551 "gram.tab.c"
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
        if (yychar < 0)
        {
            if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (short) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    yyerror("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
