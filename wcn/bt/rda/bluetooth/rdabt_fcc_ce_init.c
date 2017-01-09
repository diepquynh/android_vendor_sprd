#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/param.h>

#include "bt_cfg.h"
#include "bt_rda.h"
extern unsigned int rdabt_chip_version;
//extern int setup_uart_param(int fd,int iBaudrate,int iFlowControl);
extern int rda_setup_flow_ctl(int fd);
static int     bt_fd = -1;


static const __u32 RDA5876_PSKEY_RF_FCC_CE[][2] =
{
{0x40240000,0x0004f39c},
{0x800000C0,0x00000020},
{0x800000C4,0x003F0000},
{0x800000C8,0x00414003},
{0x800000CC,0x004225BD},
{0x800000D0,0x004908E4},
{0x800000D4,0x0043B074},
{0x800000D8,0x0044D01A},
{0x800000DC,0x004A0800},
{0x800000E0,0x0054A020},
{0x800000E4,0x0055A020},
{0x800000E8,0x0056A542},
{0x800000EC,0x00574C18},
{0x800000F0,0x003F0001},
{0x800000F4,0x00410900},
{0x800000F8,0x0046033F},
{0x800000FC,0x004C0000},
{0x80000100,0x004D0015},
{0x80000104,0x004E002B},
{0x80000108,0x004F0042},
{0x8000010C,0x0050005A},
{0x80000110,0x00510073},
{0x80000114,0x0052008D},
{0x80000118,0x005300A7},
{0x8000011C,0x005400C4},
{0x80000120,0x005500E3},
{0x80000124,0x00560103},
{0x80000128,0x00570127},
{0x8000012C,0x0058014E},
{0x80000130,0x00590178},
{0x80000134,0x005A01A1},
{0x80000138,0x005B01CE},
{0x8000013C,0x005C01FF},
{0x80000140,0x003F0000},
{0x80000040,0x10000000},
};

static const __u32 RDA5876_PSKEY_MISC_FCC_CE[][2] =
{
{0x70000008,0x331a3ae2},//
{0x7000000c,0x4e7a2cce},//
{0x80000000,0xea00238A},//   b  0x80008e30
{0x40180004,0x0001c26c},//
{0x40180024,0x00032d14},//

{0x80000004,0xea00003f},//    b 0x80000108  //  b 0x800090dc
{0x80000100,0xe51ff004},//    ....    LDR      pc,0x80008e04
{0x80000104,0x800090dc},//    4...    DCD    1076
{0x80000108,0xebfffffc},//   bl lslc_acc_bbtest_pka
{0x8000010c,0xe8bd8038},//    8@..    LDMFD  r13!,{r3-r5,r14}

{0x40180008,0x0001fbdc},//
{0x40180028,0x00032d18},//


{0x80008e00,0xe51ff004},//    ....    LDR      pc,0x80008e04
{0x80008e04,0x00000434},//    4...    DCD    1076
{0x80008e08,0xe51ff004},//    ....    LDR      pc,0x80008e0c
{0x80008e0c,0x0000529c},//    .R..    DCD    21164
{0x80008e10,0xe51ff004},//    ....    LDR      pc,0x80008e14
{0x80008e14,0x0001c6e4},//    @...    DCD    116032
{0x80008e18,0xe51ff004},//    ....    LDR      pc,0x80008e1c
{0x80008e1c,0x0002c88c},//    ....    DCD    181972
{0x80008e20,0xe51ff004},//    ....    LDR      pc,0x80008e24
{0x80008e24,0x00004ce8},//    .L..    DCD    19704
{0x80008e28,0xe51ff004},//    ....    LDR      pc,0x80008e2c
{0x80008e2c,0x000052c8},//    .R..    DCD    21208
{0x80008e30,0xe92d4038},//    8@-.    STMFD    r13!,{r3-r5,r14}
{0x80008e34,0xe59f5064},//    dP..    LDR      r5,0x80008ea0
{0x80008e38,0xe1a04000},//    .@..    MOV      r4,r0
{0x80008e3c,0xe3a00000},//    ....    MOV      r0,#0
{0x80008e40,0xe5850000},//    ....    STR      r0,[r5,#0]
{0x80008e44,0xe5d40000},//    ....    LDRB     r0,[r4,#0]
{0x80008e48,0xe350004f},//    O.P.    CMP      r0,#0x4f
{0x80008e4c,0x11a00004},//    ....    MOVNE    r0,r4
{0x80008e50,0x18bd4038},//    8@..    LDMNEFD  r13!,{r3-r5,r14}
{0x80008e54,0x1a000012},//    ....    BNE      Lslcacc_Tx_test_start_test  ; 0x80008ea4
{0x80008e58,0xe5950000},//    ....    LDR      r0,[r5,#0]
{0x80008e5c,0xe3500000},//    ..P.    CMP      r0,#0
{0x80008e60,0x1a00000d},//    ....    BNE      0x80008e9c
{0x80008e64,0xe5d41000},//    ....    LDRB     r1,[r4,#0]
{0x80008e68,0xe3a0004f},//    O...    MOV      r0,#0x4f
{0x80008e6c,0xebffffe3},//    ....    BL       $Ven$AA$L$$__rt_udiv  ; 0x80008e00
{0x80008e70,0xe5c41000},//    ....    STRB     r1,[r4,#0]
{0x80008e74,0xe1a00004},//    ....    MOV      r0,r4
{0x80008e78,0xeb000009},//    ....    BL       Lslcacc_Tx_test_start_test  ; 0x80008ea4
{0x80008e7c,0xe3a01000},//    ....    MOV      r1,#0
{0x80008e80,0xe3a00064},//    d...    MOV      r0,#0x64
{0x80008e84,0xebffffdf},//    ....    BL       $Ven$AA$L$$HWdelay_Wait_For_ms  ; 0x80008e08
{0x80008e88,0xebffffe0},//    ....    BL       $Ven$AA$L$$LSLCacc_Exit_Test  ; 0x80008e10
{0x80008e8c,0xe5d40000},//    ....    LDRB     r0,[r4,#0]
{0x80008e90,0xe2800001},//    ....    ADD      r0,r0,#1
{0x80008e94,0xe5c40000},//    ....    STRB     r0,[r4,#0]
{0x80008e98,0xeaffffee},//    ....    B        0x80008e58
{0x80008e9c,0xe8bd8038},//    8...    LDMFD    r13!,{r3-r5,pc}
{0x80008ea0,0x80009158},//    X...    DCD    2147520856
{0x80008ea4,0xe5d01002},//    ....    LDRB     r1,[r0,#2]
{0x80008ea8,0xe3510000},//    ..Q.    CMP      r1,#0
{0x80008eac,0x0a000002},//    ....    BEQ      Lslcacc_build_Tx_Test_burst  ; 0x80008ebc
{0x80008eb0,0xe3510001},//    ..Q.    CMP      r1,#1
{0x80008eb4,0x0a00004d},//    M...    BEQ      Lslcacc_build_Tx_Test_continues  ; 0x80008ff0
{0x80008eb8,0xe1a0f00e},//    ....    MOV      pc,r14
{0x80008ebc,0xe92d4038},//    8@-.    STMFD    r13!,{r3-r5,r14}
{0x80008ec0,0xe3a05470},//    pT..    MOV      r5,#0x70000000
{0x80008ec4,0xe1a04000},//    .@..    MOV      r4,r0
{0x80008ec8,0xe5950020},//     ...    LDR      r0,[r5,#0x20]
{0x80008ecc,0xe5d41014},//    ....    LDRB     r1,[r4,#0x14]
{0x80008ed0,0xe3c00007},//    ....    BIC      r0,r0,#7
{0x80008ed4,0xe2011007},//    ....    AND      r1,r1,#7
{0x80008ed8,0xe1810000},//    ....    ORR      r0,r1,r0
{0x80008edc,0xe5850020},//     ...    STR      r0,[r5,#0x20]
{0x80008ee0,0xe5950024},//    $...    LDR      r0,[r5,#0x24]
{0x80008ee4,0xe3c00480},//    ....    BIC      r0,r0,#0x80000000
{0x80008ee8,0xe5850024},//    $...    STR      r0,[r5,#0x24]
{0x80008eec,0xe5d40014},//    ....    LDRB     r0,[r4,#0x14]
{0x80008ef0,0xe59f10f0},//    ....    LDR      r1,0x80008fe8
{0x80008ef4,0xe5c10000},//    ....    STRB     r0,[r1,#0]
{0x80008ef8,0xe5d40006},//    ....    LDRB     r0,[r4,#6]
{0x80008efc,0xe3a01003},//    ....    MOV      r1,#3
{0x80008f00,0xe0810280},//    ....    ADD      r0,r1,r0,LSL #5
{0x80008f04,0xe5d41000},//    ....    LDRB     r1,[r4,#0]
{0x80008f08,0xe1a00b80},//    ....    MOV      r0,r0,LSL #23
{0x80008f0c,0xe0810820},//     ...    ADD      r0,r1,r0,LSR #16
{0x80008f10,0xe3800b80},//    ....    ORR      r0,r0,#0x20000
{0x80008f14,0xebffffbf},//    ....    BL       $Ven$AA$L$$_HWradio_ProgNow  ; 0x80008e18
{0x80008f18,0xe5950024},//    $...    LDR      r0,[r5,#0x24]
{0x80008f1c,0xe3800e80},//    ....    ORR      r0,r0,#0x800
{0x80008f20,0xe5850024},//    $...    STR      r0,[r5,#0x24]
{0x80008f24,0xe5d40001},//    ....    LDRB     r0,[r4,#1]
{0x80008f28,0xe5d41002},//    ....    LDRB     r1,[r4,#2]
{0x80008f2c,0xe1800081},//    ....    ORR      r0,r0,r1,LSL #1
{0x80008f30,0xe5d41003},//    ....    LDRB     r1,[r4,#3]
{0x80008f34,0xe1800181},//    ....    ORR      r0,r0,r1,LSL #3
{0x80008f38,0xe1d411b0},//    ....    LDRH     r1,[r4,#0x10]
{0x80008f3c,0xe1800381},//    ....    ORR      r0,r0,r1,LSL #7
{0x80008f40,0xe1d411b2},//    ....    LDRH     r1,[r4,#0x12]
{0x80008f44,0xe1800801},//    ....    ORR      r0,r0,r1,LSL #16
{0x80008f48,0xe5d41005},//    ....    LDRB     r1,[r4,#5]
{0x80008f4c,0xe3510002},//    ..Q.    CMP      r1,#2
{0x80008f50,0x03800004},//    ....    ORREQ    r0,r0,#4
{0x80008f54,0xe58500f4},//    ....    STR      r0,[r5,#0xf4]
{0x80008f58,0xe5950030},//    0...    LDR      r0,[r5,#0x30]
{0x80008f5c,0xe5d41005},//    ....    LDRB     r1,[r4,#5]
{0x80008f60,0xe1a00720},//     ...    MOV      r0,r0,LSR #14
{0x80008f64,0xe1a00700},//    ....    MOV      r0,r0,LSL #14
{0x80008f68,0xe3a02000},//    . ..    MOV      r2,#0
{0x80008f6c,0xe3510000},//    ..Q.    CMP      r1,#0
{0x80008f70,0x05852034},//    4 ..    STREQ    r2,[r5,#0x34]
{0x80008f74,0xe3800b80},//    ....    ORR      r0,r0,#0x20000
{0x80008f78,0x13a01480},//    ....    MOVNE    r1,#0x80000000
{0x80008f7c,0x15851034},//    4...    STRNE    r1,[r5,#0x34]
{0x80008f80,0xe1d411b6},//    ....    LDRH     r1,[r4,#0x16]
{0x80008f84,0xe0810000},//    ....    ADD      r0,r1,r0
{0x80008f88,0xe5d41004},//    ....    LDRB     r1,[r4,#4]
{0x80008f8c,0xe0800501},//    ....    ADD      r0,r0,r1,LSL #10
{0x80008f90,0xe5850030},//    0...    STR      r0,[r5,#0x30]
{0x80008f94,0xe59f0050},//    P...    LDR      r0,0x80008fec
{0x80008f98,0xe5d01001},//    ....    LDRB     r1,[r0,#1]
{0x80008f9c,0xe5d03008},//    .0..    LDRB     r3,[r0,#8]
{0x80008fa0,0xe26110dd},//    ..a.    RSB      r1,r1,#0xdd
{0x80008fa4,0xe2811e40},//    @...    ADD      r1,r1,#0x400
{0x80008fa8,0xe2633f9b},//    .?c.    RSB      r3,r3,#0x26c
{0x80008fac,0xe1811803},//    ....    ORR      r1,r1,r3,LSL #16
{0x80008fb0,0xe5851140},//    @...    STR      r1,[r5,#0x140]
{0x80008fb4,0xe5d01002},//    ....    LDRB     r1,[r0,#2]
{0x80008fb8,0xe5d00009},//    ....    LDRB     r0,[r0,#9]
{0x80008fbc,0xe2611f90},//    ..a.    RSB      r1,r1,#0x240
{0x80008fc0,0xe26000be},//    ..`.    RSB      r0,r0,#0xbe
{0x80008fc4,0xe2800e40},//    @...    ADD      r0,r0,#0x400
{0x80008fc8,0xe3800c80},//    ....    ORR      r0,r0,#0x8000
{0x80008fcc,0xe1810800},//    ....    ORR      r0,r1,r0,LSL #16
{0x80008fd0,0xe5850158},//    X...    STR      r0,[r5,#0x158]
{0x80008fd4,0xe5852170},//    p!..    STR      r2,[r5,#0x170]
{0x80008fd8,0xe5950028},//    (...    LDR      r0,[r5,#0x28]
{0x80008fdc,0xe3c00040},//    @...    BIC      r0,r0,#0x40
{0x80008fe0,0xe5850028},//    (...    STR      r0,[r5,#0x28]
{0x80008fe4,0xe8bd8038},//    8...    LDMFD    r13!,{r3-r5,pc}
{0x80008fe8,0x80003c78},//    ....    DCD    2147535116                     ///RDA_TEST_MODE_AM_ADDR
{0x80008fec,0x80002e34},//    ....    DCD    2147531464                     ///drive_data
{0x80008ff0,0xe92d4010},//    .@-.    STMFD    r13!,{r4,r14}
{0x80008ff4,0xe1a04000},//    .@..    MOV      r4,r0
{0x80008ff8,0xe5d00006},//    ....    LDRB     r0,[r0,#6]
{0x80008ffc,0xe3a01003},//    ....    MOV      r1,#3
{0x80009000,0xe0810280},//    ....    ADD      r0,r1,r0,LSL #5
{0x80009004,0xe5d41000},//    ....    LDRB     r1,[r4,#0]
{0x80009008,0xe1a00b80},//    ....    MOV      r0,r0,LSL #23
{0x8000900c,0xe0810820},//     ...    ADD      r0,r1,r0,LSR #16
{0x80009010,0xe3800b80},//    ....    ORR      r0,r0,#0x20000
{0x80009014,0xebffff7f},//    ....    BL       $Ven$AA$L$$_HWradio_ProgNow  ; 0x80008e18
{0x80009018,0xe3a02470},//    p$..    MOV      r2,#0x70000000
{0x8000901c,0xe5920024},//    $...    LDR      r0,[r2,#0x24]
{0x80009020,0xe3800e80},//    ....    ORR      r0,r0,#0x800
{0x80009024,0xe5820024},//    $...    STR      r0,[r2,#0x24]
{0x80009028,0xe5d40001},//    ....    LDRB     r0,[r4,#1]
{0x8000902c,0xe5d41002},//    ....    LDRB     r1,[r4,#2]
{0x80009030,0xe1800081},//    ....    ORR      r0,r0,r1,LSL #1
{0x80009034,0xe5d41003},//    ....    LDRB     r1,[r4,#3]
{0x80009038,0xe1800181},//    ....    ORR      r0,r0,r1,LSL #3
{0x8000903c,0xe1d411b0},//    ....    LDRH     r1,[r4,#0x10]
{0x80009040,0xe1800381},//    ....    ORR      r0,r0,r1,LSL #7
{0x80009044,0xe1d411b2},//    ....    LDRH     r1,[r4,#0x12]
{0x80009048,0xe5d43005},//    .0..    LDRB     r3,[r4,#5]
{0x8000904c,0xe1801801},//    ....    ORR      r1,r0,r1,LSL #16
{0x80009050,0xe3a00000},//    ....    MOV      r0,#0
{0x80009054,0xe3530000},//    ..S.    CMP      r3,#0
{0x80009058,0x05820034},//    4...    STREQ    r0,[r2,#0x34]
{0x8000905c,0x13a03480},//    .4..    MOVNE    r3,#0x80000000
{0x80009060,0x15823034},//    40..    STRNE    r3,[r2,#0x34]
{0x80009064,0xe5d43005},//    .0..    LDRB     r3,[r4,#5]
{0x80009068,0xe1a04002},//    .@..    MOV      r4,r2
{0x8000906c,0xe3530002},//    ..S.    CMP      r3,#2
{0x80009070,0x03811004},//    ....    ORREQ    r1,r1,#4
{0x80009074,0xe58210f4},//    ....    STR      r1,[r2,#0xf4]
{0x80009078,0xe59f1058},//    X...    LDR      r1,0x800090d8
{0x8000907c,0xe5d12001},//    . ..    LDRB     r2,[r1,#1]
{0x80009080,0xe5d13008},//    .0..    LDRB     r3,[r1,#8]
{0x80009084,0xe26220dd},//    . b.    RSB      r2,r2,#0xdd
{0x80009088,0xe2822e40},//    @...    ADD      r2,r2,#0x400
{0x8000908c,0xe2633f9b},//    .?c.    RSB      r3,r3,#0x26c
{0x80009090,0xe1822803},//    .(..    ORR      r2,r2,r3,LSL #16
{0x80009094,0xe3822c80},//    .,..    ORR      r2,r2,#0x8000
{0x80009098,0xe5842140},//    @!..    STR      r2,[r4,#0x140]
{0x8000909c,0xe5d12002},//    . ..    LDRB     r2,[r1,#2]
{0x800090a0,0xe5d11009},//    ....    LDRB     r1,[r1,#9]
{0x800090a4,0xe2622f90},//    ./b.    RSB      r2,r2,#0x240
{0x800090a8,0xe26110be},//    ..a.    RSB      r1,r1,#0xbe
{0x800090ac,0xe2811e40},//    @...    ADD      r1,r1,#0x400
{0x800090b0,0xe3811c80},//    ....    ORR      r1,r1,#0x8000
{0x800090b4,0xe1821801},//    ....    ORR      r1,r2,r1,LSL #16
{0x800090b8,0xe5841158},//    X...    STR      r1,[r4,#0x158]
{0x800090bc,0xe5840170},//    p...    STR      r0,[r4,#0x170]
{0x800090c0,0xe3a00003},//    ....    MOV      r0,#3
{0x800090c4,0xebffff55},//    U...    BL       $Ven$AA$L$$HW_set_tx_mode  ; 0x80008e20
{0x800090c8,0xe5940028},//    (...    LDR      r0,[r4,#0x28]
{0x800090cc,0xe3800040},//    @...    ORR      r0,r0,#0x40
{0x800090d0,0xe5840028},//    (...    STR      r0,[r4,#0x28]
{0x800090d4,0xe8bd8010},//    ....    LDMFD    r13!,{r4,pc}
{0x800090d8,0x80002e34},//    ....    DCD    2147531464
{0x800090dc,0xe59f006c},//    l...    LDR      r0,0x80009150
{0x800090e0,0xe5d00000},//    ....    LDRB     r0,[r0,#0]
{0x800090e4,0xe35000ff},//    ..P.    CMP      r0,#0xff
{0x800090e8,0x01a0f00e},//    ....    MOVEQ    pc,r14
{0x800090ec,0xe92d4008},//    .@-.    STMFD    r13!,{r3,r14}
{0x800090f0,0xe3a00014},//    ....    MOV      r0,#0x14
{0x800090f4,0xebffff4b},//    K...    BL       $Ven$AA$L$$HWdelay_Wait_For_us  ; 0x80008e28
{0x800090f8,0xe3a00001},//    ....    MOV      r0,#1
{0x800090fc,0xebffff47},//    G...    BL       $Ven$AA$L$$HW_set_tx_mode  ; 0x80008e20
{0x80009100,0xe59f004c},//    L...    LDR      r0,0x80009154
{0x80009104,0xe5d01001},//    ....    LDRB     r1,[r0,#1]
{0x80009108,0xe5d02008},//    . ..    LDRB     r2,[r0,#8]
{0x8000910c,0xe26110dd},//    ..a.    RSB      r1,r1,#0xdd
{0x80009110,0xe2811e40},//    @...    ADD      r1,r1,#0x400
{0x80009114,0xe2622f9b},//    ./b.    RSB      r2,r2,#0x26c
{0x80009118,0xe1812802},//    .(..    ORR      r2,r1,r2,LSL #16
{0x8000911c,0xe3a01470},//    p...    MOV      r1,#0x70000000
{0x80009120,0xe5812140},//    @!..    STR      r2,[r1,#0x140]
{0x80009124,0xe5d02002},//    . ..    LDRB     r2,[r0,#2]
{0x80009128,0xe5d00009},//    ....    LDRB     r0,[r0,#9]
{0x8000912c,0xe2622f90},//    ./b.    RSB      r2,r2,#0x240
{0x80009130,0xe26000be},//    ..`.    RSB      r0,r0,#0xbe
{0x80009134,0xe2800e40},//    @...    ADD      r0,r0,#0x400
{0x80009138,0xe3800c80},//    ....    ORR      r0,r0,#0x8000
{0x8000913c,0xe1820800},//    ....    ORR      r0,r2,r0,LSL #16
{0x80009140,0xe5810158},//    X...    STR      r0,[r1,#0x158]
{0x80009144,0xe3a00000},//    ....    MOV      r0,#0
{0x80009148,0xe5810170},//    p...    STR      r0,[r1,#0x170]
{0x8000914c,0xe8bd8008},//    ....    LDMFD    r13!,{r3,pc}
{0x80009150,0x80003c78},//    ....    DCD    2147535116
{0x80009154,0x80002e34},//    ....    DCD    2147531464
{0x80009158,0x00000000},//
{0x40180000,0x00000003},//
};

void RDA5876_Pskey_RfInit_fcc_ce(int fd)
{
    RDA_uart_write_array(fd,RDA5876_PSKEY_RF_FCC_CE,sizeof(RDA5876_PSKEY_RF_FCC_CE)/sizeof(RDA5876_PSKEY_RF_FCC_CE[0]),0);
}



void RDA5876_Pskey_Misc_fcc_ce(int fd)
{
    RDA_uart_write_array(fd,RDA5876_PSKEY_MISC_FCC_CE,sizeof(RDA5876_PSKEY_MISC_FCC_CE)/sizeof(RDA5876_PSKEY_MISC_FCC_CE[0]),0);

}


int RDABT_core_Intialization_fcc_ce(int fd)
{
	int bt_fd = open(RDA_BT_DEVICE_PATH, O_RDWR);
		  DBG_PRINT("RDABT_core_Intialization_fcc_ce");


	if( bt_fd < 0 )
	{
		ERR_PRINT("[###### TCC BT #######] open error.\n");

		return -1;
	}


    rdabt_chip_version = RDABT_5876;
    RDA_pin_to_high(bt_fd);

    RDA5876_RfInit(fd);

    RDA_pin_to_low(bt_fd);

    RDA_pin_to_high(bt_fd);

    usleep(50000);

    RDA5876_RfInit(fd);

    RDA5876_Pskey_RfInit_fcc_ce(fd);

    RDA5876_Dccal(fd);
    RDA5876_Pskey_Misc_fcc_ce(fd);


    rda_change_baudrate(fd);
    if(bt_fd>0)
    {
        close(bt_fd);
    }

    return 0;

}


int rda_init_fcc_ce(int fd, int iBaudrate)
{
    int ret = RDABT_core_Intialization_fcc_ce(fd);
	  DBG_PRINT("rda_init_fcc_ce");

	if(ret < 0)
	{
    	ERR_PRINT("[###### TCC BT #######]rda_init:fails: %s(%d)\n",strerror(errno), errno);
	}

	return ret;
}


#if 1
static int uart_speed(int s)
{
    switch (s)
    {
    case 9600:
         return B9600;
    case 19200:
         return B19200;
    case 38400:
         return B38400;
    case 57600:
         return B57600;
    case 115200:
         return B115200;
    case 230400:
         return B230400;
    case 460800:
         return B460800;
    case 500000:
         return B500000;
    case 576000:
         return B576000;
    case 921600:
         return B921600;
    case 1000000:
         return B1000000;
    case 1152000:
         return B1152000;
    case 1500000:
         return B1500000;
    case 2000000:
         return B2000000;
    case 3000000:
         return B3000000;
    case 4000000:
         return B4000000;
    default:
         return B57600;
    }
}


static int setup_uart_param(int fd,int iBaudrate,int iFlowControl)
{
    struct termios ti;

#if 1
    tcflush(fd, TCIOFLUSH);

    if (tcgetattr(fd, &ti) < 0)
    {
        ERR_PRINT("Can't get UART port settings\n");
        return -1;
    }

    cfmakeraw(&ti);

    ti.c_cflag |= CLOCAL;
    ti.c_cflag &= ~CRTSCTS;
    ti.c_iflag &= ~(IXON | IXOFF | IXANY | 0x80000000);

    if (iFlowControl == 1)
    {
        /* HW flow control */
        ti.c_cflag |= CRTSCTS;
    }
    else if (iFlowControl == 2)
    {
        /* MTK SW flow control */
        //ti.c_iflag |= (IXON | IXOFF | IXANY);
        ti.c_iflag |= 0x80000000;
    }

    if (tcsetattr(fd, TCSANOW, &ti) < 0)
    {
        ERR_PRINT("Can't set UART port settings\n");
        return -1;
    }

    /* Set initial baudrate */
    cfsetospeed(&ti, uart_speed(iBaudrate));
    cfsetispeed(&ti, uart_speed(iBaudrate));

    if (tcsetattr(fd, TCSANOW, &ti) < 0)
    {
        ERR_PRINT("Can't set UART baud rate\n");
        return -1;
    }

    tcflush(fd, TCIOFLUSH);
#else
	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0)
	{
		perror("Can't get port settings");
		return -1;
	}

	cfmakeraw(&ti);

	ti.c_cflag |= CLOCAL;
	ti.c_cflag |= CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &ti) < 0)
	{
		perror("Can't set port settings");
		return -1;
	}

    /* Set initial baudrate */
    cfsetospeed(&ti, uart_speed(iBaudrate));
    cfsetispeed(&ti, uart_speed(iBaudrate));

    if (tcsetattr(fd, TCSANOW, &ti) < 0)
    {
        ERR_PRINT("Can't set UART baud rate\n");
        return -1;
    }

	tcflush(fd, TCIOFLUSH);
#endif

    if (iFlowControl == FLOW_CTL_HW)
        rda_setup_flow_ctl(fd);

    return 0;
}
#endif
#if 1
int init_uart_fcc_ce(void)
{
    int fd = -1;

    DBG_PRINT("init_uart fcc ce33");

    fd = open(CUST_BT_SERIAL_PORT, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        ERR_PRINT("Can't open serial port\n");

        return -1;
    }

    if(setup_uart_param(fd, 115200, FLOW_CTL_NONE) <0)
    {
        ERR_PRINT("Can't set serial port\n");
        close(fd);
        return -1;
    }
    //if (rda_init(fd, CUST_BT_SERIAL_BOURATE) < 0)
   if (rda_init_fcc_ce(fd, CUST_BT_SERIAL_BOURATE) < 0)
    {
    	close(fd);
    	return -1;
    }
    #if (CUST_BT_SERIAL_FLOW_CTRL == FLOW_CTL_HW)
    if(setup_uart_param(fd, CUST_BT_SERIAL_BOURATE, FLOW_CTL_HW) <0)
    #elif(CUST_BT_SERIAL_FLOW_CTRL == FLOW_CTL_NONE)
    if(setup_uart_param(fd, CUST_BT_SERIAL_BOURATE, FLOW_CTL_NONE) <0)
    #endif
    {
        ERR_PRINT("Can't set serial port\n");
        close(fd);
        return -1;
    }



    return fd;
}
#endif


