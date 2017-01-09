
Usage:
  utest_input key [-r seconds]
  utest_input key [-f seqfile]
  utest_input touch [-r seconds]
  utest_input touch [-r seqfile]


Test for input system(keypad and touchscreen): (Here's the example in SP8810EA)

/* Test keypad input event for 10 seconds, out put the key event that the user inputed*/

root@android:/ # utest_input key -r 10
utest input -- key
  event.type = 1
  event.code = 116
  event.value= 1
  event.type = 0
  event.code = 0
  event.value= 0
  event.type = 1
  event.code = 116
  event.value= 0
  event.type = 0
  event.code = 0
  event.value= 0
there is no key event with 5 seconds
there is no key event with 5 seconds
utest input(key -r) finished



/* Ask the user to input the key event as the exact sequence, out put the key event that the user inputed, */
/* and if the input sequence is correct,the test passed  */

root@android:/ # utest_input key -f seqfile
utest input -- key
hit the key as the following sequence:
  FUNCTION(KEY_CODE), VALUE(DOWN:1,UP:0;
  KEY_POWER(116),     DOWN(1);
  KEY_POWER(116),     UP(0);
  KEY_VOLUP(24),      DOWN(1);
  KEY_VOLUP(24),      UP(0);
  KEY_VOLDOWN(8),     DOWN(1);
  KEY_VOLDOWN(8),     UP(0);
  KEY_CAMERA(9),      DOWN(1);
  KEY_CAMERA(9),      UP(0);
  eventcode:116 down
  eventcode:116 up
  eventcode:24 down
  eventcode:24 up
  eventcode:8 down
  eventcode:8 up
  eventcode:9 down
  eventcode:9 up
utest input(key -f) finished


/* Test touchscreen input event for 10 seconds, out put the touchscreen event that the user inputed, */
/* and output the maximun sample rate,minimum sample rate and average sample rate */

root@android:/ # utest_input touch -r 10
utest input -- touch
  event.type = 0
  event.code = 2
  event.value= 0
  event.type = 0
  event.code = 0
  event.value= 0
  event.type = 3
  event.code = 53
  event.value= 125
  event.type = 3
  event.code = 54
  event.value= 188
  ........
  event.code = 53
  event.value= 138
  event.type = 3
  event.code = 54
  event.value= 222
  event.type = 0
  event.code = 2
  event.value= 0
  event.type = 0
  event.code = 0
  event.value= 0
  event.type = 1
  event.code = 330
  event.value= 0
  curren sample : 754 times/s

there is no touchscreen event with 5 seconds
  maximum_rate:   754 times/s
  minimum_rate:   345 times/s
  average_rate:       493 times/s
utest input(touch -r) finished


/* Ask the user to input the touchscreen event as the exact sequence, out put the touchscreen event that the user inputed, */
/* and if the input sequence is correct,the test passed  */

root@android:/ # utest_input touch -f seqfile
utest input -- touch
touch the screen position as the following sequence:
  POSITION, VALUE(DOWN:1,UP:0;
  LEFT_TOP,     DOWN(1);
  LEFT_TOP,     UP(0);
  LEFT_BOTTOM,      DOWN(1);
  LEFT_BOTTOM,      UP(0);
  RIGHT_TOP,     DOWN(1);
  RIGHT_TOP,     UP(0);
  RIGHT_BOTTOM,      DOWN(1);
  RIGHT_BOTTOM,      UP(0);
  MIDDLE,      DOWN(1);
  MIDDLE,      UP(0);

......

touch position: 54  782
touch position: 53  480
touch position: 54  782
touch position: 53  480
touch position: 54  782
touch position: 53  232
touch position: 54  366
utest input(touch -f) finished


