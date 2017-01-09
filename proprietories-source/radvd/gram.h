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
extern YYSTYPE yylval;
