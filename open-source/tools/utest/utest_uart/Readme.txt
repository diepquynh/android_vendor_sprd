Usage:
utest_uart list[-n uarnum]
utest_uart set -n uartnum [-b baud][-p parity][-d databits][-s stopbits][-f flowcontrol]

Auto Test: (Here's the example in SP8810EA)

/* listuart */
root@android:/data # utest_uart list
----------------------------utest_uart begin-----------------------
utest_uart -- list
----------------------------/dev/ttyS0---------------------------
speed = 9600 baud;
data bits = 8
parity = none
stop bits = 1
RTS/CTS is disabled
INBOUND XON/XOFF is disabled
OUTBOUND XON/XOFF is enabled. XON = 11, XOFF = 13
----------------------------/dev/ttyS1---------------------------
speed = 9600 baud;
data bits = 8
parity = none
stop bits = 1
RTS/CTS is disabled
INBOUND XON/XOFF is disabled
OUTBOUND XON/XOFF is enabled. XON = 11, XOFF = 13
----------------------------/dev/ttyS2---------------------------
speed = 9600 baud;
data bits = 8
parity = none
stop bits = 1
RTS/CTS is disabled
INBOUND XON/XOFF is disabled
OUTBOUND XON/XOFF is enabled. XON = 11, XOFF = 13
----------------------------utest_uart end-----------------------

shell@android:/ # utest_uart list-n 0root@android:/data # ./utest_uart list-n 0
----------------------------utest_uart begin-----------------------
utest_uart -- list
----------------------------/dev/ttyS0---------------------------
speed = 9600 baud;
data bits = 8
parity = none
stop bits = 1
RTS/CTS is disabled
INBOUND XON/XOFF is disabled
OUTBOUND XON/XOFF is enabled. XON = 11, XOFF = 13
----------------------------utest_uart end-----------------------

root@android:/data # ./utest_uart set -n 0 -b 4800 -p o -d 6 -s 1.5  -f n
----------------------------utest_uart begin-----------------------
utest_uart -- set
databits set - 6
speed set - 4800
parity set - o
flow control set - n


