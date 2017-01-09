/*
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the functions for the actual tests, called from the
 * main routine in memtester.c.  See other comments in that file.
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
//#include <sys/cachectl.h>
#include <unistd.h>
#include <pthread.h>

#include "types.h"
#include "sizes.h"
#include "memtester.h"
#include "tests.h"
#include "log.h"

char progress[] = "-\\|/";
#define PROGRESSLEN 4
#define PROGRESSOFTEN 2500
#define ONE 0x00000001L

extern int compare_20_times;
/* Function definitions. */

#define fprintf my_fprint_f
#define printf my_printf
#define fprintfTag my_fprint_f_tag
#define printfTag my_printf_tag

#ifdef SUPPORT_X86
#define  cacheflush(a,b,c)   asm("wbinvd\n")
#endif

int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    size_t i,j;
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    off_t physaddr;

    for (j = 0; j < 20; j++)
    {
        p1 = bufa;
        p2 = bufb;

        for (i = 0; i < count; i++, p1++, p2++) {
            if (*p1 != *p2) {
#undef DEBUG_COMPARE
#           ifdef DEBUG_COMPARE
                printf("0x%08x: 0x%08lx  |  0x%08lx\n",i,*p1,*p2);
                fflush(stdout);
#           else
                if (use_phys) {
                    physaddr = physaddrbase + (i * sizeof(ul));
                    fprintf(stdout,
                            "Compare times: %d.\n",
                            (j+1));
                    fprintf(stdout, 
                            "FAILURE: 0x%08lx != 0x%08lx at physical address "
                            "0x%08lx.\n", 
                            (ul) *p1, (ul) *p2, physaddr);
                } else {
                    fprintf(stdout,
                            "Compare times: %d.\n",
                            (j+1));
                    fprintf(stdout, 
                            "FAILURE: 0x%08lx != 0x%08lx at offset 0x%08lx.\n", 
                            (ul) *p1, (ul) *p2, (ul) (i * sizeof(ul)));
                }
                
                readRepeat(p1, p2, bufa, bufb, count, 20);

                /* printf("Skipping to next test..."); */
                if (compare_20_times == 0){
                    r = -1;
                }else {
                    r = 0; /* complete the last comparison.*/
                }
#           endif
            }
        }

        if (compare_20_times == 0){ 
            j = 20; /* do not need compare 20 times; */
        }
    }
    return r;
}

void readRepeat(ulv *p1, ulv *p2, ulv *bufa, ulv *bufb, size_t count, int n_repeat) {
	int i = 0;
	char *pchar = NULL;

	for(i = 0; i < n_repeat; i++) {
		cacheflush(bufa, bufa+count, 0);
		cacheflush(bufb, bufb+count, 0);
		pchar = (*p1 == *p2)?"=":"!";
        fprintf(stdout,
                "ReadRepeat(%d): 0x%08lx %s= 0x%08lx \n",
                i, (ul) *p1, pchar, (ul) *p2);
	}
}

int test_stuck_address(ulv *bufa, size_t count) {
    ulv *p1 = bufa;
    unsigned int j;
    size_t i;
    off_t physaddr;

    //printf("           ");
    fflush(stdout);
    for (j = 0; j < 16; j++) {
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        //printf("setting %3u", j);
        //fflush(stdout);
        for (i = 0; i < count; i++) {
            *p1 = ((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1);
            *p1++;
        }
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        //printf("testing %3u", j);
        //fflush(stdout);
        p1 = (ulv *) bufa;
        for (i = 0; i < count; i++, p1++) {
            if (*p1 != (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
                if (use_phys) {
                    physaddr = physaddrbase + (i * sizeof(ul));
                    fprintf(stdout,
                            "FAILURE: possible bad address line at physical "
                            "address 0x%08lx.\n", 
                            physaddr);
                } else {
                    fprintf(stdout,
                            "FAILURE: possible bad address line at offset "
                            "0x%08lx.\n", 
                            (ul) (i * sizeof(ul)));
                }
                printf("Skipping to next test...\n");
                fflush(stdout);
                return -1;
            }
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    ul j = 0;
    size_t i;

    putchar(' ');
    fflush(stdout);
    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = rand_ul();
        if (!(i % PROGRESSOFTEN)) {
            //putchar('\b');
            //putchar(progress[++j % PROGRESSLEN]);
            fflush(stdout);
        }
    }
  //  printf("\b \b");
    fflush(stdout);
    return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        if (!q) {
            q++;
        }
        *p1++ /= q;
        *p2++ /= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
    }
    return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    //printf("           ");
    fflush(stdout);
    for (j = 0; j < 64; j++) {
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? UL_ONEBITS : 0;
        //printf("setting %3u", j);
        fflush(stdout);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        // printf("\b\b\b\b\b\b\b\b\b\b\b");
        //("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    //printf("           ");
    fflush(stdout);
    for (j = 0; j < 64; j++) {
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
        //printf("setting %3u", j);
        fflush(stdout);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        //printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    //printf("           ");
    fflush(stdout);
    for (j = 0; j < 256; j++) {
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        //printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul) UL_BYTE(j);
        }
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        //printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    //printf("           ");
    fflush(stdout);
    for (j = 0; j < UL_LEN * 2; j++) {
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        //printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = ONE << j;
            } else { /* Walk it back down. */
                *p1++ = *p2++ = ONE << (UL_LEN * 2 - j - 1);
            }
        }
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        //printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    //printf("           ");
    fflush(stdout);
    for (j = 0; j < UL_LEN * 2; j++) {
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        //printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << j);
            } else { /* Walk it back down. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
            }
        }
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        //printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    //printf("           ");
    fflush(stdout);
    for (j = 0; j < UL_LEN * 2; j++) {
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        //printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (ONE << j) | (ONE << (j + 2))
                    : UL_ONEBITS ^ ((ONE << j)
                                    | (ONE << (j + 2)));
            } else { /* Walk it back down. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? (ONE << (UL_LEN * 2 - 1 - j)) | (ONE << (UL_LEN * 2 + 1 - j))
                    : UL_ONEBITS ^ (ONE << (UL_LEN * 2 - 1 - j)
                                    | (ONE << (UL_LEN * 2 + 1 - j)));
            }
        }
        //printf("\b\b\b\b\b\b\b\b\b\b\b");
        //printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j, k;
    ul q;
    size_t i;

    //printf("           ");
    fflush(stdout);
    for (k = 0; k < UL_LEN; k++) {
        q = ONE << k;
        for (j = 0; j < 8; j++) {
            //printf("\b\b\b\b\b\b\b\b\b\b\b");
            q = ~q;
            //printf("setting %3u", k * 8 + j);
            fflush(stdout);
            p1 = (ulv *) bufa;
            p2 = (ulv *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
            }
            //printf("\b\b\b\b\b\b\b\b\b\b\b");
            //printf("testing %3u", k * 8 + j);
            fflush(stdout);
            if (compare_regions(bufa, bufb, count)) {
                return -1;
            }
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

void switch_for_tiger(ulv *p){
    char *r0;
    char *r1;
    char new_r0[4],new_r1[4];
    ulv *p_r = p;
    int i; 
#define copy_byte(__x,__y) do{\
    __y = __x;}while(0)

/*Change method:
SW_original:
Word data:  0x77665544
Address 0x4:    7   6   5   4
Byte data:  0x77    0x66    0x55    0x44

Word data:  0x33221100
Address 0x0:    3   2   1   0
Byte data:  0x33    0x22    0x11    0x00

    |
    |
    V

SW_Swap:
Word data:  0x33772266
Address 0x4:    7   6   5   4
Byte data:  0x33    0x77    0x22    0x66

Word data:  0x11550044
Addrees 0x0:    3   2   1   0
Byte data:  0x11    0x55    0x00    0x44 

r0_0 -> new_r0_1
r0_1 -> new_r0_3
r0_2 -> new_r1_1
r0_3 -> new_r1_3
r1_0 -> new_r0_0
r1_1 -> new_r0_2
r1_2 -> new_r1_0
r1_3 -> new_r1_2*/

    /*switch p,p-1*/
    r1 = (char *)(p_r--);
    r0 = (char *)p_r;
/*    printf("r1=0x%08lx,*r1=0x%08lx\n",r1,*(ulv*)r1);
    printf("r0=0x%08lx,*r0=0x%08lx\n",r0,*(ulv*)r0);*/
    copy_byte(r0[0],new_r0[1]);
    copy_byte(r0[1],new_r0[3]);
    copy_byte(r0[2],new_r1[1]);
    copy_byte(r0[3],new_r1[3]);
    copy_byte(r1[0],new_r0[0]);
    copy_byte(r1[1],new_r0[2]);
    copy_byte(r1[2],new_r1[0]);
    copy_byte(r1[3],new_r1[2]);
    for(i = 0; i < 4; i++){
        r0[i] = new_r0[i]; /*write back*/
        r1[i] = new_r1[i];
    }
/*    printf("r1=0x%08lx,*r1=0x%08lx\n",r1,*(ulv*)r1);
    printf("r0=0x%08lx,*r0=0x%08lx\n",r0,*(ulv*)r0);*/
}

int test_bitflip_comparison_tiger(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    /*ulv *temp;*/
    unsigned int j, k;
    ul q;
    size_t i;
    //printf("           ");
    fflush(stdout);

    if (sizeof(ulv) != 4){
        printf("This test only support for 32bit Architecture");
        return -1;
    }

    for (k = 0; k < UL_LEN; k++) {
        q = ONE << k;
        for (j = 0; j < 8; j++) {
            //printf("\b\b\b\b\b\b\b\b\b\b\b");
            q = ~q;
            //printf("setting %3u", k * 8 + j);
            fflush(stdout);
            p1 = (ulv *) bufa;
            p2 = (ulv *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
                if ((i > 0) && ((i % 2) == 1)){  /*switch for tiger*/
                   /* *temp = 0x33221100;
                    *p1 = 0x77665544;
                    //printf("\nbefore switch: 0x%8X 0x%8X\n",*(ulv *)temp,*(ulv *)p1);
                    //printf("\nbefore switch p1 i=%d : 0x%08lX 0x%08lX\n",i,*(ulv *)(p1-1),*(ulv *)p1);
                    //printf("\nbefore switch p2 i=%d : 0x%08lX 0x%08lX\n",i,*(ulv *)(p2-1),*(ulv *)p2);*/
                    switch_for_tiger(p1-1); 
                    switch_for_tiger(p2-1); 
                /*    printf("\nafter switch p1 i=%d : 0x%08lX 0x%08lX\n",i,*(ulv *)(p1-1),*(ulv *)p1);
                    printf("\nafter switch p2 i=%d : 0x%08lX 0x%08lX\n",i,*(ulv *)(p2-1),*(ulv *)p2);
                }else{
                    temp = p1;*/
                }
            }
            //printf("\b\b\b\b\b\b\b\b\b\b\b");
            //printf("testing %3u", k * 8 + j);
            fflush(stdout);
            if (compare_regions(bufa, bufb, count)) {
                return -1;
            }
        }
    }
    //printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}


#ifdef TEST_NARROW_WRITES    
int test_8bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u8v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    putchar(' ');
    fflush(stdout);
    for (attempt = 0; attempt < 2;  attempt++) {
        if (attempt & 1) {
            p1 = (u8v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u8v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword8.bytes;
            *p2++ = mword8.val = rand_ul();
            for (b=0; b < UL_LEN/8; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
               // putchar('\b');
                //putchar(progress[++j % PROGRESSLEN]);
                fflush(stdout);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b \b");
    fflush(stdout);
    return 0;
}

int test_16bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u16v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    putchar( ' ' );
    fflush( stdout );
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u16v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u16v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword16.u16s;
            *p2++ = mword16.val = rand_ul();
            for (b = 0; b < UL_LEN/16; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                //putchar('\b');
                //putchar(progress[++j % PROGRESSLEN]);
                fflush(stdout);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    //printf("\b \b");
    fflush(stdout);
    return 0;
}
#endif

extern int volatile bfinished;
extern int mode;
extern pthread_mutex_t mutex;
int compare_regions_tag(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    size_t i,j;
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    off_t physaddr;

    for (j = 0; j < 20; j++)
    {
        p1 = bufa;
        p2 = bufb;

        for (i = 0; i < count; i++, p1++, p2++) {
            if (*p1 != *p2) {
#undef DEBUG_COMPARE
#           ifdef DEBUG_COMPARE
                printfTag("0x%08x: 0x%08lx  |  0x%08lx\n",i,*p1,*p2);
                fflush(stdout);
#           else
                if (use_phys) {
                    physaddr = physaddrbase + (i * sizeof(ul));
                    fprintfTag(stdout,
                            "Compare times: %d.\n",
                            (j+1));
                    fprintfTag(stdout,
                            "FAILURE: 0x%08lx != 0x%08lx at physical address "
                            "0x%08lx.\n",
                            (ul) *p1, (ul) *p2, physaddr);
                } else {
                    fprintfTag(stdout,
                            "Compare times: %d.\n",
                            (j+1));
                    fprintfTag(stdout,
                            "FAILURE: 0x%08lx != 0x%08lx at offset 0x%08lx.\n",
                            (ul) *p1, (ul) *p2, (ul) (i * sizeof(ul)));
                }

                readRepeat(p1, p2, bufa, bufb, count, 20);

                /* printf("Skipping to next test..."); */
                if (compare_20_times == 0){
                    r = -1;
                }else {
                    r = 0; /* complete the last comparison.*/
                }
#           endif
            }
        }

        if (compare_20_times == 0){
            j = 20; /* do not need compare 20 times; */
        }
    }
    return r;
}

void readRepeat_tag(ulv *p1, ulv *p2, ulv *bufa, ulv *bufb, size_t count, int n_repeat) {
	int i = 0;
	char *pchar = NULL;

	for(i = 0; i < n_repeat; i++) {
		cacheflush(bufa, bufa+count, 0);
		cacheflush(bufb, bufb+count, 0);
		pchar = (*p1 == *p2)?"=":"!";
        fprintfTag(stdout,
                "ReadRepeat(%d): 0x%08lx %s= 0x%08lx \n",
                i, (ul) *p1, pchar, (ul) *p2);
	}
}

int compare_regions_const(ulv *bufa, size_t count, int value) {
	ulv *p1 = bufa;
	size_t i;
	int res = 0;
	for(i = 0; i < count; i++, p1++) {
		if(*p1 != value) {
			printfTag(TAG_REFRESH, "FAILURE: 0x%08lx != 0x%08lx at offset 0x%08lx.\n",
						(ul) *p1, value, (ul) (i * sizeof(ul)));
			res = -1;
		}
	}

	return res;
}

int test_bitRefresh_comparison(lprefresh_t lprefresh) {
    ulv *bufConst = lprefresh->bufa;
    ulv *bufRandom = lprefresh->bufb;
    size_t const_count = lprefresh->count/2;
    size_t random_count = lprefresh->count/2;
    ulv *p1 = bufConst;
    ulv *p2 = bufConst+const_count;
    ulv *p3 = bufRandom;
    ulv *p4 = bufRandom+random_count;
    size_t i;
    ul loop = 0;
    int result = 0;

    for(i = 0; i < const_count; i++) {
    	*p1++ = 0;
    	*p2++ = 1;
    }

    for(i = 0; i < random_count; i++) {
    	*p3++ = *p4++ = rand_ul();
    }

    do {
    	pthread_mutex_lock(&mutex);

        printfTag(TAG_REFRESH, "Loop %lu :\n", loop++);
    	printfTag(TAG_REFRESH, "\n");
    	printfTag(TAG_REFRESH, "  %-20s: \n", "refresh");

    	p1 = bufConst;
    	p2 = bufConst+const_count;
        p3 = bufRandom;
        p4 = bufRandom+random_count;
    	result = compare_regions_const(p1, const_count, 0);
    	result |= compare_regions_const(p2, const_count, 1);
    	result |= compare_regions_tag(p3, p4, random_count);
    	if (!result) {
    		printfTag(TAG_REFRESH, "  %-20s: ok\n", " ");
    	}

    	pthread_mutex_unlock(&mutex);

    	sleep(lprefresh->time);
    }while(!bfinished);

    return 0;
}
