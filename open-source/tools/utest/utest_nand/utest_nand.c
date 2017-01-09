/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <time.h>

#define BUF_SIZE	       (50*1024)         /*least malloc size*/
#define FILE_PATH_LEN          1024              /*max lenght of file path*/
#define DEFAULT_FILE_SIZE      (20*1024*1024)    /*default file_size to test*/

static double erase_duration = 0;    /*record total erase time(us)*/
static double write_duration = 0;    /*record total write time(us)*/
static double read_duration  = 0;    /*record total read time(us)*/
static int    file_size      = 0;    /*file_size to test*/
static int    test_count     = 5;    /*times to repeat the testing*/
static char * file_name      = NULL;

static void usage(char *prog_name)
{
	(void)fprintf(stderr,
		"Usage: \n"
		"  utest_nand info mtd_device\n"
		"  utest_nand raw  mtd_device [-n count]\n"
		"  utest_nand fs [-f filepath] [-s size] [-n count]\n"
		"  utest_nand erase mtd_device\n"
		"info \n"
		"	show the mtd_device info, including page size, block size, partition info, bad block stats, dirty block stats, etc.\n"
		"raw\n"
		"	test the read/erase/write bandwidth of the raw block device and output the result as MB/s.\n"
		"fs\n"
		"	test the read/write bandwidth of the file system and output the result as MB/s.The file size should be big enough to decrease the effect of page cache.\n"
		"erase \n"
		"	erase the mtd_device entirely.\n"
		"mtd_device \n"
		"	specify the mtd device which to be tested.\n"
		"-f filepath \n"
		"	specify the file path. it's current directory by default.\n"
		"-s size \n"
		"	specify the file size to test. it's 20MB by default.\n"
		"-n count\n"
		"	specify the count to repeat the testing. it's 5 by default.\n"
		);
}

void process_options (int argc, char **argv)
{
	int opt = 0;
	while ((opt = getopt (argc, argv, "f:s:n:")) != -1) {
		switch (opt) {
		case 'f':
			if (!(file_name = strdup(optarg)))
			{
			printf("error ");
			}
			break;
		case 's':
			file_size = atoi(optarg);
			break;
		case 'n':
			test_count = atoi(optarg);
			break;
		default:
			break;
		}
	}

}

/*
 * MEMGETREGIONCOUNT
 * MEMGETREGIONINFO
 */
static int getregions (int fd,struct region_info_user *regions,int *n)
{
	int i,err;
	err = ioctl (fd,MEMGETREGIONCOUNT,n);
	if (err) return (err);
	for (i = 0; i < *n; i++){
		regions[i].regionindex = i;
		err = ioctl (fd,MEMGETREGIONINFO,&regions[i]);
		if (err) return err;
	}
	return 0;
}

/*get current path name*/
static char * get_exe_path( char * buf)
{
	int i;
	int rslt = readlink("/proc/self/exe", buf, FILE_PATH_LEN);
	if (rslt < 0 ){
		return NULL;
	}
	buf[rslt] = '\0';
	for (i = rslt; i >= 0; i--){
		if (buf[i] == '/'){
			buf[i + 1] = '\0';
			break;
		}
	}
	return buf;
}

/*test mtd raw read */
static int raw_read_bandwidth(int fd)
{
	u_int8_t *buf = NULL;
	int err;
	int i = 0;
	int buf_size = 0;
	int readsize = 0;
	mtd_info_t meminfo;
        int read_num = 0;
	struct timespec ts_start, ts_end;
	double duration = 0;
	err = ioctl(fd,MEMGETINFO,&meminfo);
	if (err == 0){
		readsize = meminfo.writesize;
		buf = (u_int8_t *) malloc (readsize);
		memset(buf, 0, readsize);
	}else {
		fprintf(stderr,"\n%s MEMGETINFO error, %d %s\n", __FUNCTION__, errno, strerror(errno));
		return err;
	}

	read_num  = meminfo.size/meminfo.writesize;
	/* read start */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
		free (buf);
		return -EIO;
	}
	for (i = read_num; i > 0; i--) {
		err = read (fd,buf, meminfo.writesize);
		if (err < 0){
			     fprintf(stderr,"\n%s read error, %d %s\n", __FUNCTION__, errno, strerror(errno));
			     return err;
		}
	}
	/* read end */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0) {
		free (buf);
		return -EIO;
	}
	/*calculate duration*/
	duration = (ts_end.tv_sec - ts_start.tv_sec)*1000000 +
		   (ts_end.tv_nsec - ts_start.tv_nsec)/1000.0;

	read_duration += duration;
	free (buf);
	return 0;
}

/*test mtd raw erase */
static int raw_erase_bandwidth(int fd)
{
	int i, j;
	region_info_t * reginfo;
	uint32_t start = 0;
	mtd_info_t meminfo;
	struct timespec ts_start, ts_end;
	double duration = 0;

	if (ioctl(fd,MEMGETINFO,&meminfo) == 0)	{
		erase_info_t erase;
		erase.start = 0;
		erase.length = meminfo.erasesize;
		/* erase start */
		if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
			return -EIO;
		}

		for (; (erase.start + erase.length) > meminfo.size; )
		{
			if (ioctl(fd,MEMERASE,&erase) != 0)
			{
				fprintf(stderr,"\n%s Erase error, %d %s\n", __FUNCTION__, errno, strerror(errno));
				close(fd);
				return -EPERM;
			}
			erase.start += meminfo.erasesize;
		}
		/* erase end */
		if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0) {
			return -EIO;
		}

		/*calculate duration*/
		duration = (ts_end.tv_sec - ts_start.tv_sec)*1000000 +
			   (ts_end.tv_nsec - ts_start.tv_nsec)/1000.0;

		erase_duration += duration;
	}else{
		fprintf(stderr,"%s MEMGETINFO error\n", __FUNCTION__);
		return -EPERM;
	}

	return 0;
}

/*test mtd raw write */
static int raw_write_bandwidth(int fd)
{
	u_int8_t *buf = NULL;
	int outfd,err;
	int i = 0;
	int writesize = 0;
        int write_num = 0;
	struct timespec ts_start, ts_end;
	double duration = 0;
	mtd_info_t meminfo;

	if (ioctl(fd,MEMGETINFO,&meminfo) == 0){
		writesize = meminfo.writesize;
		buf = (u_int8_t *) malloc (writesize);
		memset(buf, 0xAA, writesize);
	}else {
		fprintf(stderr,"%s MEMGETINFO error\n", __FUNCTION__);
		return -EPERM;
	}
	write_num  = meminfo.size/meminfo.writesize;

	/* write start */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
		free (buf);
		return -EIO;
	}
	if (0 != lseek (fd,0,SEEK_SET))
	{
		fprintf(stderr,"%s lseek error, %d %s\n", __FUNCTION__, errno, strerror(errno));
		free (buf);
		return -EIO;
	}

	for (i = write_num; i > 0; i--) {
		err = write (fd, buf, meminfo.writesize);
		if (err < 0){
			free (buf);
			fprintf(stderr, "%s write error %d %s\n", __FUNCTION__, errno, strerror(errno));
			return -EFAULT;
		}
	}

	/* write end */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0) {
		free (buf);
		return -EIO;
	}
	duration = (ts_end.tv_sec - ts_start.tv_sec)*1000000 +
		   (ts_end.tv_nsec - ts_start.tv_nsec)/1000.0;
	write_duration += duration;
	free (buf);
	return 0;
}

int fs_read_bandwidth (int fd, size_t len)
{

	u_int8_t *buf = NULL;
	int err;
	int size = len * sizeof (u_int8_t);
	int n = len;
	struct timespec ts_start, ts_end;
	double duration = 0;

	if (0 != lseek (fd,0,SEEK_SET)){
		printf("%s lseek() error", __FUNCTION__);
		return -EIO;
	}

retry:
	if ((buf = (u_int8_t *) malloc (size)) == NULL){
		fprintf (stderr, "%s: malloc(%#x)\n", __FUNCTION__, size);
		if (size != BUF_SIZE) {
			size = BUF_SIZE;
			fprintf (stderr, "%s: trying buffer size %#x\n", __FUNCTION__, size);
			goto retry;
		}
		printf("%s malloc buffer error", __FUNCTION__);
		return -ENOMEM;
	 }

	/* read start */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
		free (buf);
		return -EIO;
	}

	do {
		if (n <= size)
			size = n;
	   	err = read (fd,buf,size);
	   	if (err < 0){
			fprintf (stderr, "%s: read, size %#x, n %#x error\n", __FUNCTION__, size, n);
                        free(buf);
			return -EFAULT;
		 }
		 n -= size;
	} while (n > 0);

	/* read end */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0) {
		free (buf);
		return -EIO;
	}
	duration = (ts_end.tv_sec - ts_start.tv_sec)*1000000 +
		   (ts_end.tv_nsec - ts_start.tv_nsec)/1000.0;
	read_duration += duration;
	free (buf);
	//printf ("%s read %d bytes from  %f \n",__FUNCTION__,len, read_duration);
	return 0;
}


static int fs_write_bandwidth (int fd, u_int32_t len)
{
	u_int8_t *buf = NULL;
	int err =0;
	int size = len * sizeof (u_int8_t);
	int n = len;
	struct timespec ts_start, ts_end;
	double duration = 0;

retry:
	if ((buf = (u_int8_t *) malloc (size)) == NULL){
		fprintf (stderr, "%s: malloc(%#x) failed\n", __FUNCTION__, size);
		if (size != BUF_SIZE){
			size = BUF_SIZE;
			fprintf (stderr, "%s: trying buffer size %#x\n", __FUNCTION__, size);
			goto retry;
		}
		printf(" malloc OK error");
		return -ENOMEM;
	 }
	memset(buf, 0xAA, size);
	if (0 != lseek (fd,0,SEEK_SET))
	{
		fprintf(stderr,"%s lseek error, %d %s\n", __FUNCTION__, errno, strerror(errno));
		free (buf);
		return -EIO;
	}
	/* write start */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_start) < 0) {
		free (buf);
		return -EIO;
	}
	do {
		if (n <= size)
			size = n;
		err = write (fd,buf,size);
		if (err < 0) {
		fprintf (stderr, "%s: write, size %#x, n %#x error\n", __FUNCTION__, size, n);
		free (buf);
		return -EFAULT;
	 	}
		n -= size;
	} while (n > 0);

	/* write end */
	if (clock_gettime(CLOCK_MONOTONIC, &ts_end) < 0) {
		free (buf);
	 	return -EIO;
	}
	duration = (ts_end.tv_sec - ts_start.tv_sec)*1000000 +
		    (ts_end.tv_nsec - ts_start.tv_nsec)/1000.0;
	write_duration += duration;
	if (buf != NULL)
		free (buf);

	//printf ("Write %d bytes to time %f\n",len, write_duration);
	return 0;
}

/*show the mtd info*/
static int show_mtd_info(char* mtd_device)
{
	int i,err,n;
	int fd = 0;
	int bad = 0;
	int ret = 0;
	uint32_t offset = 0;
	struct mtd_info_user mtd;
	static struct region_info_user region[1024];

	if(mtd_device == NULL){
		fprintf(stderr,"mtd_divice should not be NULL!\n");
		return -EINVAL;
	}
	/* Open and size the device */
	if ((fd = open(mtd_device, O_RDWR)) < 0){
		fprintf(stderr,"%s  open error\n" , __FUNCTION__);
		return -EIO;
	}

	if (ioctl(fd,MEMGETINFO,&mtd) != 0){
		fprintf(stderr,"%s MEMGETINFO error\n", __FUNCTION__);
		return -EPERM;

	}

	err = getregions (fd,region,&n);
	if (err < 0) {
		fprintf(stderr,"%s MEMGETREGIONCOUNT error\n", __FUNCTION__);
		return -EPERM;
	 }

	/* Check all the blocks in an erase block for bad blocks */
	do {
		if ((ret = ioctl(fd, MEMGETBADBLOCK, &offset)) < 0) {
			fprintf(stderr,"%s MEMGETBADBLOCK error\n", __FUNCTION__);
			return -EPERM;
		}
		if (ret == 1) {
			bad++;
		}
		offset+=  mtd.erasesize;
	} while ( offset < mtd.size );


	printf ("mtd.type= ");
	switch (mtd.type)
	{
	case MTD_ABSENT:
		printf ("MTD_ABSENT");
		break;
	case MTD_RAM:
		printf ("MTD_RAM");
		break;
	case MTD_ROM:
		printf ("MTD_ROM");
		break;
	case MTD_NORFLASH:
		printf ("MTD_NORFLASH");
		break;
	case MTD_NANDFLASH:
		printf ("MTD_NANDFLASH");
		break;
	default:
		printf ("(unknown type - new MTD API maybe?)");
	 }

	printf ("\nmtd.flags = ");
	if (mtd.flags == MTD_CAP_ROM)
		printf ("MTD_CAP_ROM");
	else if (mtd.flags == MTD_CAP_RAM)
		printf ("MTD_CAP_RAM");
	else if (mtd.flags == MTD_CAP_NORFLASH)
		printf ("MTD_CAP_NORFLASH");
	else if (mtd.flags == MTD_CAP_NANDFLASH)
		printf ("MTD_CAP_NANDFLASH");
	else if (mtd.flags == MTD_WRITEABLE)
		printf ("MTD_WRITEABLE");
	else
		printf ("MTD_UNKNOWN FLAG");

	printf ("\nmtd regions = %d\n",n);
	for (i = 0; i < n; i++){
		printf ("region[%d].offset = 0x%.8x\n"
				"region[%d].erasesize = %d",
				i,region[i].offset,i,region[i].erasesize);
		printf ("\nregion[%d].numblocks = %d\n"
				"region[%d].regionindex = %d\n",
				i,region[i].numblocks,
				i,region[i].regionindex);
	}

	printf("   size  \t pagesize\t  block  \t  obsize  \tbadblock/totalblock\n");
	printf (" 0x%.8x\t0x%.8x\t0x%.8x\t0x%.8x\t %d/%d\t\n", mtd.size,  mtd.writesize,mtd.erasesize,
		mtd.oobsize,bad,mtd.size/mtd.erasesize);
	return 0;
}

static int mtd_raw_test(void)
{
	int fd = 0;
	struct mtd_info_user mtd;
	int i = 0;

	/* Open and size the device */
	if(file_name == NULL){
		fprintf(stderr, "Error:mtd_device is null!");
		return -EINVAL;
	}
	if ((fd = open(file_name, O_SYNC | O_RDWR)) < 0){
		fprintf(stderr,"%s  open error\n" , __FUNCTION__);
		return -EIO;
	}

	if (ioctl(fd,MEMGETINFO,&mtd) != 0){
		fprintf(stderr,"%s MEMGETINFO error\n", __FUNCTION__);
		close(fd);
		return -EPERM;
	}
	if(test_count == 0){
		test_count = 1;
	}

	printf("\nmtd_device:%s count=%d\n", file_name, test_count);
	printf("MTD RAW TEST is calculating, please wait ... \n");
	erase_duration = 0;
	write_duration = 0;
	read_duration = 0;
	for(i = 0; i < test_count; i++)
	{
		raw_erase_bandwidth(fd);
		raw_write_bandwidth(fd);
		raw_read_bandwidth(fd);
		printf("Test has finished %d time(s)\n", i+1);
	}

	printf("MTD RAW Erase Speed: %.3f MB/s\n", ((1.0 * mtd.size * test_count / erase_duration)*1000*1000)/(1024 * 1024));
	printf("MTD RAW Write Speed: %.3f MB/s\n", ((1.0 * mtd.size * test_count / write_duration)*1000*1000)/(1024 * 1024));
	printf("MTD RAW Read Speed: %.3f MB/s\n", ((1.0 * mtd.size * test_count / read_duration)*1000*1000)/(1024 * 1024));
	printf("MTD RAW TEST END! \n");
	return 0;
}

static int mtd_fs_test(void)
{
	int fd = 0;
	int i = 0;

	if(file_name == NULL){
		file_name = malloc(FILE_PATH_LEN);
		get_exe_path(file_name);
	}
	if(file_size == 0){
		file_size = DEFAULT_FILE_SIZE;
	}
	if(test_count == 0)
		test_count = 1;
	printf("\nMTD File Path:%s , file_size = %d, count=%d\n", file_name, file_size, test_count);

	printf("MTD FILE TEST is calculating, please wait ... \n");

	write_duration = 0;
	read_duration = 0;

	if ((fd = open (file_name,O_SYNC | O_RDWR)) < 0){
		fprintf (stderr, "%s: open %s failed\n", __FUNCTION__, file_name);
		return -EIO;
	}


	for(; i < test_count; i++)
	{

		fs_write_bandwidth(fd, file_size);
		fs_read_bandwidth(fd, file_size);
		printf("Test has finished %d time(s)\n", i+1);
	}

	printf("MTD FS Write Speed: %.3f MB/s\n", ((1.0 * file_size * test_count / write_duration)*1000*1000)/(1024 * 1024));
	printf("MTD FS Read Speed: %.3f MB/s\n", ((1.0 * file_size * test_count / read_duration)*1000*1000)/(1024 * 1024));
	printf("MTD FS TEST END! \n");
	close(fd);
	return 0;
}


static int mtd_erase_mtd(char* mtd_device)
{
	int fd = 0;
	struct mtd_info_user mtd;
	/* Open and size the device */
	if ((fd = open(mtd_device, O_RDWR)) < 0){
		fprintf(stderr,"%s  open error\n" , __FUNCTION__);
		return -EIO;
	}

	if (ioctl(fd,MEMGETINFO,&mtd) != 0){
		fprintf(stderr,"%s MEMGETINFO error\n", __FUNCTION__);
		close(fd);
		return -EPERM;

	}
	raw_erase_bandwidth(fd);

	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	char *cmd = NULL;
	int rval  = -EINVAL;

	if (argc < 2) {
		usage(argv[0]);
		return rval;
	}

	cmd = argv[1];
	if (strcmp(cmd, "info") == 0) {
		/*utest_nand info*/
		rval = show_mtd_info(argv[2]);
	} else if (strcmp(cmd, "raw") == 0) {
		/*utest_nand raw*/
		if(argc > 2){
			file_name = strdup(argv[2]);
	        	argv += 2;
			argc -=2;
			process_options(argc, argv);
			rval = mtd_raw_test();
		}else{
			fprintf(stderr,"mtd_device must not be NULL!\n" );
			return -EINVAL;
		}
	} else if (strcmp(cmd, "fs") == 0) {
		/*utest_nand fs*/
		argv++;
		argc--;
		process_options(argc, argv);
		mtd_fs_test();
		rval = 0;
	}else if(strcmp(cmd, "erase") == 0){
		/*utest_nand erase*/
		if(argc > 2){
			rval = mtd_erase_mtd(argv[2]);
			if(rval == 0){
				printf(" Erase OK!\n");
			}
		}else{
			fprintf(stderr,"mtd_device must not be NULL!\n" );
			return -EINVAL;
		}
	}

	return rval;
}

