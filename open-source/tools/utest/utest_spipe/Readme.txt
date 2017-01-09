Usage:
    utest_spipe verify | [-d dev] | [-b unit_size] | [-t total_size]
    utest_spipe throughput | [-d dev] | [-b unit_size] | [-t total_size]
    utest_spipe latency | [-d dev] | [-b unit_size] | [-t total_size]



Test instant :
===========================================================================

1. ~# utest_spipe latency -d /dev/spipe_td0 -b 125 -t 100321                  
Latency finished
	Average time cost 22.061 ms per unit, one uint contains 125 Bytes.


2. ~# utest_spipe latency -b 1000
Latency finished
	Average time cost 158.429 ms per frame, one frame contains 1000 bytes.

3. ~# utest_spipe throughput -b 1000 -t 1000000
Throughput finished
	Total data size 0.954 MB, unit size 1000 Bytes, speed 6320.793 KB/s.

4. ~# utest_spipe latency -b 1000 -t 1000000                
Latency finished
	Average time cost 158.279 ms per unit, one uint contains 1000 Bytes.


5. ~# utest_spipe verify -b 127 -t 180315               
Correct data!
	Total data size 0.172 MB, count 1419, unit size 127 Bytes.


