
Usage:
  utest_mem bandwidth [-s size] [-n count] [-p processoes]
  utest_mem verify [-s size] [-n count]

Auto Test: (Here's the example in SP8810EA)

/* Test L1 Cache */
shell@android:/ # utest_mem bandwidth -s 4096 -n 65536                         
utest_mem -- bandwidth
 1 Memory Speed: 1439.472 MB/s


/* Test L2 Cache */
shell@android:/ # utest_mem bandwidth -s 65536 -n 4096                         
utest_mem -- bandwidth
 1 Memory Speed: 1068.647 MB/s

/* Test Memory */
shell@android:/ # utest_mem bandwidth -s 4096000 -n 64                         
utest_mem -- bandwidth
 1 Memory Speed: 380.456 MB/s

/* Test Memory with 2 processes */
shell@android:/ # utest_mem bandwidth -s 4096000 -n 64 -p 2                    
utest_mem -- bandwidth
 2 Memory Speed: 205.782 MB/s
 1 Memory Speed: 195.220 MB/s

