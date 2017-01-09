Usage:
   utest_nand info mtd_device
   utest_nand raw  mtd_device [-n count]
   utest_nand fs [-f filepath] [-s size] [-n count]
   utest_nand erase mtd_device
info
   show the mtd_device info, including page size, block size, partition info, bad block stats, dirty block stats, etc.
raw
   test the read/erase/write bandwidth of the raw block device and output the result as MB/s.
fs
   test the read/write bandwidth of the file system and output the result as MB/s.The file size should be big enough to decrease the effect of page cache.
erase
   erase the mtd_device entirely.
mtd_device
   specify the mtd device which to be tested.
-f filepath
   specify the file path. it's current directory by default.
-s size
   specify the file size to test. it's 20MB by default.
-n count
   specify the count to repeat the testing. it's 5 by default.

Auto Test: (Here's the example in SP8810EA)

/*utest_nand info mtd_device*/
root@android:/ # ./utest_nand info /dev/mtd/mtd12
mtd.type= MTD_NANDFLASH
mtd.flags = MTD_CAP_NANDFLASH
mtd regions = 0
   size  	 pagesize	  block  	  obsize  	badblock/totalblock
 0x0b400000	0x00000800	0x00020000	0x00000040	 204/1440

/*utest_nand raw  mtd_device [-n count]*/
root@android:/ # ./utest_nand raw /dev/mtd/mtd13 -n 10

mtd_device:/dev/mtd/mtd13 count=10
MTD RAW TEST is calculating, please wait ...
Test has finished 1 time(s)
Test has finished 2 time(s)
Test has finished 3 time(s)
Test has finished 4 time(s)
Test has finished 5 time(s)
Test has finished 6 time(s)
Test has finished 7 time(s)
Test has finished 8 time(s)
Test has finished 9 time(s)
Test has finished 10 time(s)
MTD RAW Erase Speed: 10655868.720 MB/s
MTD RAW Write Speed: 8.721 MB/s
MTD RAW Read Speed: 3959.772 MB/s
MTD RAW TEST END!


/*utest_nand fs [-f filepath] [-s size] [-n count]*/
root@android:/ # ./utest_nand fs -f /dev/block/mtdblock13 -n10

MTD File Path:/dev/block/mtdblock13 , file_size = 20971520, count=10
MTD FILE TEST is calculating, please wait ...
Test has finished 1 time(s)
Test has finished 2 time(s)
Test has finished 3 time(s)
Test has finished 4 time(s)
Test has finished 5 time(s)
Test has finished 6 time(s)
Test has finished 7 time(s)
Test has finished 8 time(s)
Test has finished 9 time(s)
Test has finished 10 time(s)
MTD FS Write Speed: 2.809 MB/s
MTD FS Read Speed: 221.346 MB/s
MTD FS TEST END!

/*utest_nand erase mtd_device */
root@android:/ # ./utest_nand erase /dev/mtd/mtd13
Erase OK!

