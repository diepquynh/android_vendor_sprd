#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


/* 32-bit ELF base types. */
typedef __uint32_t	Elf32_Addr;
typedef __uint16_t	Elf32_Half;
typedef __uint32_t	Elf32_Off;
typedef __int32_t	Elf32_Sword;
typedef __uint32_t	Elf32_Word;

#define EI_NIDENT	16
struct elf32_hdr {
  unsigned char	e_ident[EI_NIDENT];
  Elf32_Half	e_type;
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;  /* Entry point */
  Elf32_Off	e_phoff;
  Elf32_Off	e_shoff;
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;
  Elf32_Half	e_phentsize;
  Elf32_Half	e_phnum;
  Elf32_Half	e_shentsize;
  Elf32_Half	e_shnum;
  Elf32_Half	e_shstrndx;
};

struct elf32_phdr{
  Elf32_Word	p_type;
  Elf32_Off	p_offset;
  Elf32_Addr	p_vaddr;
  Elf32_Addr	p_paddr;
  Elf32_Word	p_filesz;
  Elf32_Word	p_memsz;
  Elf32_Word	p_flags;
  Elf32_Word	p_align;
};


/* Note header in a PT_NOTE section */
struct elf32_note {
  Elf32_Word	n_namesz;	/* Name size */
  Elf32_Word	n_descsz;	/* Content size */
  Elf32_Word	n_type;		/* Content type */
};

typedef __uint32_t	__u32;
struct reg_context {
	__u32	r4;
	__u32	r5;
	__u32	r6;
	__u32	r7;
	__u32	r8;
	__u32	r9;
	__u32	sl;
	__u32	fp;
	__u32	sp;
	__u32	pc;
	__u32	extra[2];		/* Xscale 'acc' register, etc */
};

struct reg_all {
	__u32	r0;
	__u32	r1;
	__u32	r2;
	__u32	r3;
	__u32	r4;
	__u32	r5;
	__u32	r6;
	__u32	r7;
	__u32	r8;
	__u32	r9;
	__u32	r10;
	__u32	fp;
	__u32	ip;
	__u32	sp;
	__u32	lr;
	__u32	pc;
	__u32	extra[2];		/* Xscale 'acc' register, etc */
};


__uint32_t context_new[18];
__uint32_t context_old[18];

int fd = -1;

char *mmap_open(char *file, unsigned long offset, unsigned long size)
{
	char *maddr = MAP_FAILED;

	fd = open(file, O_RDWR|O_SYNC);
	if (fd < 0) {
		printf("open error.\n");
		return MAP_FAILED;
	}

	maddr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
	if (maddr == MAP_FAILED) {
		printf("mmap error.\n");
		return MAP_FAILED;
	}

	return maddr;
}

int mmap_close(void *addr, unsigned long size)
{
	if (addr != NULL)
		munmap(addr, size);

	if (fd >= 0)
		close(fd);
}

__uint32_t *get_regs_addr(char *start_addr)
{
	struct elf32_phdr *elfhdr;
	struct elf32_note *note;
	char *prstatus;
	__uint32_t *regs;

	elfhdr = (struct elf32_phdr *)((char *)start_addr + sizeof(struct elf32_hdr));
	note = (struct elf32_note *)((char *)start_addr + elfhdr->p_offset);
	prstatus = (char *)((char *)note + sizeof(struct elf32_note) + note->n_namesz + 3);
	regs = (__uint32_t *)((char *)prstatus + 0x48);

	return regs;
}

void help(char *exec)
{
	printf ("\nUsage: %s [c|p] COREFILE CPU_CONTEXT_ADDR\n\n", exec);
	printf ("          [c|p] --- c for cpu backtrace, p for process backtrace\n");
	printf ("          COREFILE --- full path of core file\n");
	printf ("          CPU_CONTEXT_ADDR --- process context is shown by 'ps' command\n");
	printf ("                               in gdb, and cpu context can is shown by\n");
	printf ("                               'cc' command in gdb. In hex format\n\n\n");
	return;
}

int main(int argc, char *argv[])
{

	char *corefile, *mapaddr;
	int context_addr, fileoff, bufoff, ret;
	int fd;
	int context_type;

	if (argc != 4) {
		help(argv[0]);
		return -1;
	}


	if (strcmp(argv[1], "c") && strcmp(argv[1], "p")) {
		help(argv[0]);
		return -1;
	}

	corefile = argv[2];
	context_addr = strtoul(argv[3], NULL, 16);
	if (context_addr == 0) {
		printf("wrong context addr.\n");
		return -1;
	}

	fileoff = (context_addr - 0xc0000000 + 0x00001000) & 0xfffff000;
	bufoff =  (context_addr - 0xc0000000 + 0x00001000) & 0x00000fff;
	mapaddr = mmap_open(corefile, fileoff, 0x1000);
	if (mapaddr == MAP_FAILED)
		return -1;
	memcpy(context_new, mapaddr + bufoff, sizeof(context_new));
	mmap_close(mapaddr, 0x1000);



	mapaddr = mmap_open(corefile, 0, 0x1000);
	if (mapaddr == MAP_FAILED)
		return -1;
	__uint32_t *gdb_context = get_regs_addr(mapaddr);	
	memcpy(context_old, gdb_context, sizeof(context_old));

	struct reg_all *ra = (struct reg_all *)gdb_context;

	if (!strcmp(argv[1], "p")) {
		struct reg_context *rc = (struct reg_context *)context_new;
		ra->r4 = rc->r4;
		ra->r5 = rc->r5;
		ra->r6 = rc->r6;
		ra->r7 = rc->r7;
		ra->r8 = rc->r8;
		ra->r9 = rc->r9;
		ra->fp = rc->fp;
		ra->sp = rc->sp;
		ra->pc = rc->pc;
	} else {
		struct reg_all *rc = (struct reg_all *)context_new;
		ra->r4 = rc->r4;
		ra->r5 = rc->r5;
		ra->r6 = rc->r6;
		ra->r7 = rc->r7;
		ra->r8 = rc->r8;
		ra->r9 = rc->r9;
		ra->fp = rc->fp;
		ra->sp = rc->sp;
		ra->pc = rc->pc;
	}
	sync();

	char gdbcmd[1024];
	snprintf(gdbcmd, 1024, 
		"(arm-eabi-gdb vmlinux --core %s)<< GDBEOF\nset print null-stop on\nbt\nGDBEOF\n", 
		corefile);
	system(gdbcmd);

	memcpy(gdb_context, context_old, sizeof(context_old));
	sync();

	mmap_close(mapaddr, 0x1000);

	return 0;
}

