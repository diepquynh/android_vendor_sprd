#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/kernel.h>
#include <linux/magic.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/vfs.h>
#include <sys/mman.h>

/* #define DEBUG */
#ifdef DEBUG
#define DEBUGSEE fprintf
#else
#define DEBUGSEE(...)
#endif



/* option flags */
#define LOOKAT_F_GET      (1<<0)
#define LOOKAT_F_GET_MUL  (1<<1)
#define LOOKAT_F_SET      (1<<2)

#define LOOKAT_F_GET_V      (1<<3)
#define LOOKAT_F_GET_MUL_V  (1<<4)
#define LOOKAT_F_SET_V      (1<<5)


static int debugfs_found = 0;
char debugfs_mountpoint[4096 + 1] = "/sys/kernel/debug";
static int debugfs_premounted;

static const char *debugfs_known_mountpoints[] = {
	"/sys/kernel/debug/",
	"/debug/",
	"/d/",
	0,
};

/* verify that a mountpoint is actually a debugfs instance */
static int debugfs_valid_mountpoint(const char *debugfs)
{
	struct statfs st_fs;

	if (statfs(debugfs, &st_fs) < 0)
		return -2;
	else if (st_fs.f_type != (long) DEBUGFS_MAGIC)
		return -2;

	return 0;
}

/* find the path to the mounted debugfs */
static const char *debugfs_find_mountpoint(void)
{
	const char **ptr;
	char type[100];
	FILE *fp;

	if (debugfs_found)
		return (const char *) debugfs_mountpoint;

	ptr = debugfs_known_mountpoints;
	while (*ptr) {
		if (debugfs_valid_mountpoint(*ptr) == 0) {
			debugfs_found = 1;
			strcpy(debugfs_mountpoint, *ptr);
			return debugfs_mountpoint;
		}
		ptr++;
	}

	/* give up and parse /proc/mounts */
	fp = fopen("/proc/mounts", "r");
	if (fp == NULL)
		return NULL;

	while (fscanf(fp, "%*s %4096s %99s %*s %*d %*d\n",
		      debugfs_mountpoint, type) == 2) {
		if (strcmp(type, "debugfs") == 0)
			break;
	}
	fclose(fp);

	if (strcmp(type, "debugfs") != 0)
		return NULL;

	debugfs_found = 1;

	return debugfs_mountpoint;
}



/* mount the debugfs somewhere if it's not mounted */
static char *debugfs_mount(const char *mountpoint)
{
	struct statfs st_fs;
	/* see if it's already mounted */
	if (debugfs_find_mountpoint()) {
		debugfs_premounted = 1;
		goto out;
	}
	/* if not mounted and no argument */
	if (mountpoint == NULL) {
		/* see if environment variable set */
		/* if no environment variable, use default */
		mountpoint = "/sys/kernel/debug/";
	}

	if (statfs("/debug_12345", &st_fs) < 0){
		if(mkdir("/debug_12345/", 0644) < 0)
			return NULL;
	}
	if (mount(NULL, mountpoint, "debugfs", 0, NULL) < 0)
		return NULL;

	/* save the mountpoint */
	debugfs_found = 1;
	strncpy(debugfs_mountpoint, mountpoint, sizeof(debugfs_mountpoint));
out:

	return debugfs_mountpoint;
}


static int find_create_debugfs(char **p)
{
	char *_p = 0;
	char *debugfs_dir = debugfs_mount("/debug_12345/");
	if (!debugfs_dir)
		return -1;
	_p = debugfs_dir;
	while (*_p)
		_p++;
	if (*(_p-1) != '/') {
		*(_p) = '/';
		*(_p+1) = 0;
	}

	DEBUGSEE("debugfs_dir = %s\n",debugfs_dir);
	*p = debugfs_dir;

	return 0;
}
int sub_main(int flags, int addr, int value, int nword)
{
	int fd1, fd2;
	char *dir = 0;
	char strPath1[256] = {0};
	char strPath2[256] = {0};
	char c[12 + 12] = {0};
	int n = 0;
	ssize_t cnt = 0;

	if (find_create_debugfs(&dir))
		return 0;

	 sprintf(strPath1, "%s%s", dir, "lookat/addr_rwpv");
	 sprintf(strPath2, "%s%s", dir, "lookat/data");

	 if((fd1 = open(strPath1, O_RDWR | O_SYNC)) < 0)
	 {
			perror("open addr_rwpv");
			 exit(EXIT_FAILURE);
			 return -1;
	 }
	 if((fd2 = open(strPath2, O_RDWR | O_SYNC)) < 0)
	 {
			 perror("open data");
			 exit(EXIT_FAILURE);
			 return -1;
	 }

	if ((flags & LOOKAT_F_SET) || (flags & LOOKAT_F_SET_V) ) {
		if (flags & LOOKAT_F_SET)
			addr |= 2;
		else
			addr |= 3;
		n = snprintf(c, 11,"0x%x", addr);
		write(fd1, c, n);
		n = snprintf(c, 11,"0x%x", value);
		write(fd2, c, n);
	} else if ( (flags & LOOKAT_F_GET) || (flags & LOOKAT_F_GET_V) ) {
		if (flags & LOOKAT_F_GET_V)
			addr |= 1;
		n = snprintf(c, 11,"0x%x", addr);
		write(fd1, c, n);
		memset(c, 0, 11);
		cnt = read(fd2, c, 10);
		if((cnt < 0) || (cnt > 10)){
			perror("read data");
			exit(EXIT_FAILURE);
			return -1;
		}
		fprintf(stdout, "%s\n",c);
	} else if ( (flags & LOOKAT_F_GET_MUL) || (flags & LOOKAT_F_GET_MUL_V) ){
        int i;
        /* print a serial of reg in a formated way */
        fprintf(stdout, "  ADDRESS  |   VALUE\n"
                        "-----------+-----------\n");
        for (i = 0; i < nword; ++i) {
				if (flags & LOOKAT_F_GET_MUL_V)
					addr |= 1;
				n = snprintf(c, 11,"0x%0x", addr);
				write(fd1, c, n);
				memset(c, 0, 11);
				cnt = read(fd2, c, 10);/*It is  simple file system don't support seek method
									 just open again it is quickly.*/
				if((cnt < 0) || (cnt > 10)){
					perror("read data");
					exit(EXIT_FAILURE);
					return -1;
				}
				close(fd2);
				if((fd2 = open(strPath2, O_RDWR | O_SYNC)) < 0) {
					fprintf(stderr,"open %s error\n", strPath2);
					goto Exit;/*sad*/
				}
			    fprintf(stdout, "0x%08x | %s\n", addr & ~0x3, c);
                addr += 4;
        }

	}
	close(fd2);
Exit:
	close(fd1);
	return 0;
}




//===============================================================
/*
 * phy address simple read, word only, return to stdout
 *
 * 2010.11.16 v01 fool2
 *          init version
 * 2010.11.17 v02 fool2
 *          add ADI reg read support
 * 2010.11.17 v03 fool2
 *          add continous area read support
 * 2010.11.17 v04 fool2
 *          add set support, polish the help info
 * 2012.11.16 v05 fool2
 *          add sc8825 (tiger) a-die regs access
 * 2013.4.1 v06 fool2
 *          add sc7710g /sc8830(shark) a-die regs access
 */

#define LOG_NDEBUG 0
#define LOG_TAG "lookat"

static unsigned int chip_is_known = 0;

#define MAPSIZE (4096)
unsigned int ana_max_size = 4096;
unsigned int ana_reg_addr_start = 0x42000000, ana_reg_addr_end = 0;
#define IS_ANA_ADDR(_a) (_a>=ana_reg_addr_start && _a<=ana_reg_addr_end)

#define ADI_ARM_RD_CMD(b) (b+0x24)
#define ADI_RD_DATA(b)    (b+0x28)
#define ADI_FIFO_STS(b)   (b+0x2C)


void usage0(void)
{
        fprintf(stderr,
"\n"
"Usage: lookat [-l nword] [-s value] [-h] phy_addr_in_hex\n"
"get/set the value of IO reg at phy_addr_in_hex\n"
"  -l nword     print values of nword continous regs start from \n"
"               phy_addr_in_hex in a formated way\n"
"  -s value     set a reg at phy_addr_in_hex to value\n"
"  -h           print this message\n\n"
" NOTE:\n"
" * the phy_addr_in_hex should be 4-byte aligned in hex form\n"
" * the value should be in hex form too\n"
" * currently, there are two reg areas:\n"
"  1) 0x%08x-0x%08x - ADI REG area\n"
"  2) other addresses       - Other IO areas\n"
"\n"
                ,ana_reg_addr_start, ana_reg_addr_end);
}

void usage1(void)
{
        fprintf(stderr,
"\n"
"Usage: lookat [-l nword] [-s value] [-h] addr_in_hex\n"
"get/set the value of IO reg at addr_in_hex\n"
"  -l nword     print values of nword continous regs start from \n"
"               addr_in_hex in a formated way\n"
"  -s value     set a reg at addr_in_hex to value\n"
"  -h           print this message\n\n"
" NOTE:\n"
" * the addr_in_hex should be 4-byte aligned in hex form\n"
" * Default is physical address, Using Virtual address If the \n"
" * parameters are capital letter: Only the [-L],[-S],(Read/Write) \n"
" * will use virtual address\n");
}

unsigned short ADI_Analogdie_reg_read (unsigned int vbase, unsigned int paddr)
{
        unsigned int data;

        *(volatile unsigned int*)((unsigned long)ADI_ARM_RD_CMD(vbase)) = paddr;

        do {

                 data = *(volatile unsigned int*)((unsigned long)ADI_RD_DATA(vbase));
        } while (data & 0x80000000);

        if ((data & 0xffff0000) != (paddr << 16)) {
                fprintf(stderr, "ADI read error [0x%x]\n", data);
                exit(EXIT_FAILURE);
        }

        return ((unsigned short)(data & 0xffff));
}

void ADI_Analogdie_reg_write (unsigned int vbase, unsigned int vaddr,
                                        unsigned int data)
{

        do {

                 if ((*(volatile unsigned int *)((unsigned long)ADI_FIFO_STS(vbase))&(1<<10))
                          != 0)
                         break;
        } while (1);

        *(volatile unsigned int*)((unsigned long)vaddr) = (data & 0xffff);

}

void set_value(unsigned int paddr, unsigned int vbase,
                unsigned int offset, unsigned int value)
{
        unsigned int old_value, new_value;

        if (IS_ANA_ADDR(paddr)) {
                old_value = ADI_Analogdie_reg_read(vbase,paddr);
                ADI_Analogdie_reg_write(vbase,(vbase+offset),value);
                new_value = ADI_Analogdie_reg_read(vbase,paddr);
                DEBUGSEE(stdout, "ADie REG\n"
                                "before: 0x%04x\n"
                                "after:  0x%04x\n",
                                old_value, new_value);

        }
        else {
                old_value = *((unsigned int *)((unsigned long)(vbase+offset)));
                *(volatile unsigned int*)((unsigned long)(vbase+offset)) = value;
                new_value = *((unsigned int *)((unsigned long)(vbase+offset)));
                DEBUGSEE(stdout, "before: 0x%08x\n"
                                "after:  0x%08x\n",
                                old_value, new_value);
        }

        return;
}

void print_result(unsigned int paddr, unsigned int vbase,
                unsigned int offset)
{
        if (IS_ANA_ADDR(paddr))
                fprintf(stdout, "0x%04x\n",
                                ADI_Analogdie_reg_read(vbase,paddr));
        else 
                fprintf(stdout, "0x%08x\n", 
                                *((unsigned int *)((unsigned long)(vbase+offset))));
        return;
}

void print_result_mul(unsigned int paddr, unsigned int vbase,
                unsigned int offset, int nword)
{
        int i;

        /* print a serial of reg in a formated way */
        fprintf(stdout, "  ADDRESS  |   VALUE\n"
                        "-----------+-----------\n");
        for (i=0;i<nword;i++) {
                if (IS_ANA_ADDR(paddr))
                        fprintf(stdout, "0x%08x |     0x%04x\n", paddr,
                                        ADI_Analogdie_reg_read(vbase,paddr));
                else 
                        fprintf(stdout, "0x%08x | 0x%08x\n", paddr,
                                        *((unsigned int *)((unsigned long)(vbase+offset))));
                paddr+=4;
                offset+=4;
        }

        return;
}

int chip_probe(void)
{
    int fd;
    char *vbase;
    unsigned int chip_id;
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        return -1; /* try others method */
    }

    vbase = (char*)mmap(0, MAPSIZE, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0x20900000);
    if (vbase == (char *)-1) {
        perror("mmap failed!");
        exit(EXIT_FAILURE);
    }

    chip_id = *((unsigned int *)(vbase+0x3fc));
    chip_id >>= 16;

    if (chip_id == 0x8810 || chip_id == 0x7710) {
        ana_reg_addr_start = 0x82000040;
		chip_is_known = 1;
    }
    else if (chip_id == 0x8820) {/* tiger, shark...*/
        ana_reg_addr_start = 0x42000040;
		chip_is_known = 1;
    }

    /* each adi frame only contain 9~10 bits address */
    ana_reg_addr_end = (ana_reg_addr_start & ~(MAPSIZE - 1)) + ana_max_size;

    if (munmap(vbase, 4096) == -1) {
        perror("munmap failed!");
        exit(EXIT_FAILURE);
    }

    close(fd);
    return 0;
}

int main (int argc, char** argv)
{
        int fd, opt;
        int flags = 0; /* option indicator */
        unsigned int paddr, pbase, offset;
        unsigned int value = 0, nword = 0;
        char *vbase;
		char *op = "s:l:h";
		char *op_v = "s:l:S:L:h";
		void (*usage)(void);

		usage = usage0;
//        chip_probe();

		if (!chip_is_known) {
			op = op_v;
			usage = usage1;
		}
        while ((opt = getopt(argc, argv, op)) != -1) {
                switch (opt) {
                case 's':
                        flags |= LOOKAT_F_SET;
                        value = strtoul(optarg, NULL, 16);
                        DEBUGSEE(stdout, "[I] value = 0x%x\n", value);
                        break;
                case 'l':
                        flags |= LOOKAT_F_GET_MUL;
                        nword = strtoul(optarg, NULL, 10);
                        DEBUGSEE(stdout, "[I] nword = 0x%d\n", nword);
                        if (nword <= 0) nword = 1;
                        break;
                case 'h':
                        usage();
                        exit(0);
                default:
						if (!chip_is_known) {
							if (opt == 'S') {
								flags |= LOOKAT_F_SET_V;
								value = strtoul(optarg, NULL, 16);
								DEBUGSEE(stdout, "[I] value = 0x%x\n", value);
							} else if (opt == 'L') {
								flags |= LOOKAT_F_GET_MUL_V;
								nword = strtoul(optarg, NULL, 10);
								DEBUGSEE(stdout, "[I] nword = 0x%d\n", nword);
								if (nword <= 0)
									nword = 1;
							} else {
								usage();
								exit(EXIT_FAILURE);
							}
						} else {
							usage();
							exit(EXIT_FAILURE);
						}
                }
        }

        if (optind == 1) 
                flags |= LOOKAT_F_GET;

        if (optind >= argc) {
                fprintf(stderr, "Expected argument after options\n");
                exit(EXIT_FAILURE);
        }

        paddr = strtoul(argv[optind], NULL, 16);
        DEBUGSEE(stdout, "[I] paddr = 0x%x\n", paddr);

        if (paddr&0x3) {
                fprintf(stderr, "address should be 4-byte aligned\n");
                exit(EXIT_FAILURE);
        }

        offset = paddr & (MAPSIZE - 1);
        pbase = paddr - offset;

        if ((nword<<2) >= (MAPSIZE-offset)) {
                fprintf(stderr, "length should not cross the page boundary!\n");
                exit(EXIT_FAILURE);
        }

	if (!chip_is_known) {
		sub_main(flags, paddr, value, nword);
		return 0;
	}

        /* ok, we've done with the options/arguments, start to work */
        if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
                perror("could not open /dev/mem/");
                exit(EXIT_FAILURE);
        }

        vbase = (char*)mmap(0, MAPSIZE, PROT_READ | PROT_WRITE, 
                        MAP_SHARED, fd, pbase);
        if (vbase == (char *)-1) {
                perror("mmap failed!");
                exit(EXIT_FAILURE);
        }

        if (flags & LOOKAT_F_SET )
                set_value(paddr, (unsigned long)vbase, offset, value);

        if ( flags & LOOKAT_F_GET )
                print_result(paddr, (unsigned long)vbase, offset);

        if ( flags & LOOKAT_F_GET_MUL )
                print_result_mul(paddr, (unsigned long)vbase, offset, nword);

        if (munmap(vbase, 4096) == -1) {
                perror("munmap failed!");
                exit(EXIT_FAILURE);
        }

        close(fd);

        return 0;
}

