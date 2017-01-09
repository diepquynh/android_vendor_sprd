Dependency:
	Meidaservice must be stoped while testing.

Interface:
	ALSA


Usage:
	utest_audio mix [-l|-c ctrl –v value]
	utest_audio play –n count –f file.wav
	utest_audio capture –s seconds -f file.wav
	utest_audio headset –n count

mix
	display or set the mix control

play
	play a sound file

capture
	capture sound into a file

headset
	do headset hotplug detect and show the result

-l
	list all mix ctrl path

-c ctrl
	specify the ctrl number to be set. it’s used to change the audio path.

-v value
	specify the value to set to a ctrl path

-n count
	specify the testing loop count

-s seconds
	specify the length of record sound

-f file.wav
	specify a wav file

/* Test mix -l */
root@android:/ # ./utest_audio mix -l
controls:hw:2
numid=1,iface=MIXER,name='Master Playback Volume'
    type=INTEGER,access=rw---R--,values=2,min=0,max=15,step=0
    values=15,15
    dBscale-min=-22.50dB,step=1.50dB,mute=0
numid=10,iface=MIXER,name='Line Function'
    type=BOOLEAN,access=rw------,values=1
    values=on
numid=13,iface=MIXER,name='Linein Rec Function'
    type=BOOLEAN,access=rw------,values=1
    values=on
numid=15,iface=MIXER,name='LineinRec Capture Volume'
    type=INTEGER,access=rw---R--,values=1,min=0,max=7,step=0
    values=7
    dBscale-min=-10.50dB,step=1.50dB,mute=0
numid=2,iface=MIXER,name='Line Playback Volume'
    type=INTEGER,access=rw---R--,values=2,min=0,max=19,step=0
    values=19,19
    dBscale-min=-22.50dB,step=1.50dB,mute=0
numid=4,iface=MIXER,name='Mic Boost Volume'
    type=INTEGER,access=rw---R--,values=1,min=0,max=1,step=0
    values=1
    dBscale-min=0.00dB,step=20.00dB,mute=0
numid=11,iface=MIXER,name='Mic Function'
    type=BOOLEAN,access=rw------,values=1
    values=on
numid=3,iface=MIXER,name='Capture Volume'
    type=INTEGER,access=rw---R--,values=1,min=0,max=15,step=0
    values=15
    dBscale-min=0.00dB,step=1.50dB,mute=0
numid=5,iface=MIXER,name='Ear Boost Volume'
    type=INTEGER,access=rw---R--,values=1,min=0,max=1,step=0
    values=0
    dBscale-min=0.00dB,step=6.00dB,mute=0
numid=8,iface=MIXER,name='Earpiece Function'
    type=BOOLEAN,access=rw------,values=1
    values=off
numid=12,iface=MIXER,name='HP Mic Function'
    type=BOOLEAN,access=rw------,values=1
    values=off
numid=9,iface=MIXER,name='HeadPhone Function'
    type=BOOLEAN,access=rw------,values=1
    values=on
numid=6,iface=MIXER,name='HeadPhone Playback Volume'
    type=INTEGER,access=rw---R--,values=2,min=0,max=31,step=0
    values=31,31
    dBminmax-min=-33.50dB,max=4.50dB
numid=14,iface=MIXER,name='Inter PA Playback Volume'
    type=INTEGER,access=rw---R--,values=1,min=0,max=15,step=0
    values=8
    dBscale-min=-24.00dB,step=3.00dB,mute=1
numid=7,iface=MIXER,name='Speaker Function'
    type=BOOLEAN,access=rw------,values=1
    values=on
numid=17,iface=MIXER,name='VBC EQ Switch'
    type=ENUMERATED,access=rw------,values=1,items=2
    Item #0 'off'
    Item #1 'on'
    values=0
numid=18,iface=MIXER,name='VBC EQ Update'
    type=ENUMERATED,access=rw------,values=1,items=2
    Item #0 'idle'
    Item #1 'loading'
    values=0
numid=16,iface=MIXER,name='VBC Switch'
    type=ENUMERATED,access=rw------,values=1,items=2
    Item #0 'dsp'
    Item #1 'arm'
    values=1

/* Test mix -c  -v  */
root@android:/ # ./utest_audio mix -c 7 -v off
numid=7,iface=MIXER,name='Speaker Function'
    type=BOOLEAN,access=rw------,values=1
    values=off

/* Test Play */
root@android:/ # ./utest_audio play -f w.wav -n 5
Testing device:hw:sprdphone,0
Playing 'w.wav' : Signed 16 bit Little Endian, Rate 22050 Hz, Stereo
Sample Rate:       22050
Sample Format:     S16_LE
Number of Channels:2
Bytes per Sample:  2
                                  +00%|00%+
                                  +00%|00%+
                                  +00%|00%+
                                  +00%|00%+
                                  +00%|00%+
/* Test Capture */
root@android:/ # ./utest_audio capture -f test.wav -s 20
Testing device:hw:sprdphone,0
Record data will be be saved in test.wav
Record time is 20 seconds.
Recording 'test.wav' : Signed 16 bit Little Endian, Rate 44100 Hz, Mono
Sample Rate:       44100
Sample Format:     S16_LE
Number of Channels:1
Bytes per Sample:  2
#+                                                 | 01%

/* Test Headset */
root@android:/ # ./utest_audio headset -n 5
Current Headset:OUT
1 Current Headset:IN
2 Current Headset:OUT
3 Current Headset:IN
4 Current Headset:OUT
5 Current Headset:IN
