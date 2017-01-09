#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "elf.h"

long gbase_addr;
long gsntable_addr;


static int check_elf()
{

//TODO can check if the file is a elf file
	return 0;


}

static long get_elftextsize(int fd)
{
	int offset;
	int i = 0;
	long textsize = 0;
	Elf32_Ehdr* ehdr = mmap(0,1024,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(ehdr == MAP_FAILED){
	  return -1;
	}
	gbase_addr = (long)ehdr;

	printf(" ehdr->e_shstrndx is %d \n",ehdr->e_shstrndx);
	printf(" ehdr->e_shentsize is %d \n",ehdr->e_shentsize);
	printf(" ehdr->e_shoff is  %d \n",ehdr->e_shoff);
	printf("\n");

	munmap(ehdr,1024);
//	Elf32_Shdr* shdr;
//	offset = ehdr->e_shoff + ehdr->e_shstrndx * ehdr->e_shentsize;
//	shdr = gbase_addr + offset;
//	gsntable_addr = gbase_addr + shdr->sh_offset;
//	shdr = ehdr + offset;
//	char* gsntable_addr = ehdr + shdr->sh_offset;
/*
	shdr = (Elf32_Shdr *)(gbase_addr + ehdr->e_shoff);	
	for(i;i<ehdr->e_shnum;i++){
		char* name;
		name = (char*)(gsntable_addr+shdr->sh_name);
		printf("section name is %s",name);
		//if(shdr->)
		shdr++;
	}
*/
	return  textsize;

}

static int  useage(char *program)
{
	printf("\n %s usage: %s  VMLINUX COREFILE TEXTSIZE\n", program, program);
	printf("                    VMLINUX     ----full path of vmlinux file\n");
	printf("                    COREFILE    ----full path of core file\n");
	printf("                    TEXTSIZE    ----the size of text section\n");
	printf(" NOTE : \n");
	printf("       1.make sure that the first arg is the vmlinux file path \n");
	printf("       2.you can get textsize from vmlinux using readelf, the 'Size' field of '.text'\n");	
}

static int fillfile(int srcfd , int destfd,unsigned long size)
{

	char buff[256] = {0};
	int count = 0;
	do{
		read(srcfd,buff,sizeof(buff));
		write(destfd,buff,sizeof(buff));
		count += 256;
		}while(count < size);
	return 0;
}

int main(int argc,char* argv[])
{
	int fd1,fd2;
	int fd3,fd4;
	char* file1;
	char* file2;
	char* vmlinux_file = "vmlinux.S";
	char* core_file = "core.S";

	unsigned long  elftexts_size;

	if(argc !=  4 ){
		return useage(argv[0]);
	}
	
	file1 = argv[1];
	file2 = argv[2];
	elftexts_size = strtoul(argv[3], NULL, 16);

	fd1 = open(file1,O_RDWR|O_SYNC);
	fd2 = open(file2,O_RDWR|O_SYNC);

	fd3 = open(vmlinux_file,O_CREAT|O_RDWR|O_SYNC);
	fd4 = open(core_file,O_CREAT|O_RDWR|O_SYNC);

	system("chmod 777 *");

	if((fd1 < 0) || (fd2 < 0)){
		if(fd1 < 0 ){
			printf("\ninvalidate argument  %s\n",argv[1]);
		}else{
			printf("\ninvalidate argument  %s\n",argv[2]);
			}
		goto end;
	}

//TODO gettext size from elf	elftexts_size = get_elftextsize(fd1);

	lseek(fd1,0x9000,SEEK_SET);
	lseek(fd2,0x9000+0x1000,SEEK_SET);

	fillfile(fd1,fd3,elftexts_size);
	fillfile(fd2,fd4,elftexts_size);

	system("hexdump -c core.S > core.txt");
	system("hexdump -c vmlinux.S > vmlinux.txt");
	system("diff core.txt vmlinux.txt");
	system("rm core.S core.txt");
	system("rm vmlinux.S  vmlinux.txt");


end:
	if(fd1 >= 0)
		close(fd1);
	if(fd2 >= 0)
		close(fd2);
	if(fd3 >= 0)
		close(fd3);
	if(fd4 >= 0)
		close(fd4);
	printf(" \n the compare has finished \n");
	return 0;


}
