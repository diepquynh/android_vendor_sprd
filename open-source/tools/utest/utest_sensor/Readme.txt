
Usage:
  utest_sensor list [-a]
  utest_sensor read [-t sensor_type] [-d delay]
  utest_sensor verify [-t sensor_type] [-d delay] [-o verify_option]

Auto Test: (Here's the example in SP8810EA)

/* List sensors */
shell@android:/ # utest_sensor list
utest_sensor -- list
--------------------------------------------------------
utest_sensor -- list
5 sensors found:
sensor_type: Mag ::  AK8975 3-axis Magnetic field sensor
sensor_type: Acc ::  ST LIS3DH 3-axis Accelerometer
sensor_type: Ori ::  AK8975 Orientation sensor
sensor_type: Lux ::  AL3006 Light sensor
sensor_type: Prx ::  AL3006 Proximity sensor
--------------------------------------------------------
shell@android:/ # utest_sensor list -a
--------------------------------------------------------
utest_sensor -- list
5 sensors found:
AK8975 3-axis Magnetic field sensor
	vendor: Asahi Kasei Microdevices
	version: 1
	handle: 1
	type: Mag
	maxRange: 1228.800049
	resolution: 0.060000
	power: 0.350000 mA
ST LIS3DH 3-axis Accelerometer
	vendor: ST
	version: 1
	handle: 0
	type: Acc
	maxRange: 19.613300
	resolution: 0.009577
	power: 0.145000 mA
AK8975 Orientation sensor
	vendor: Asahi Kasei Microdevices
	version: 1
	handle: 2
	type: Ori
	maxRange: 360.000000
	resolution: 0.015625
	power: 0.495000 mA
AL3006 Light sensor
	vendor: LITEON
	version: 1
	handle: 3
	type: Lux
	maxRange: 1.000000
	resolution: 100000.000000
	power: 0.005000 mA
AL3006 Proximity sensor
	vendor: LITEON
	version: 1
	handle: 4
	type: Prx
	maxRange: 1.000000
	resolution: 1.000000
	power: 0.005000 mA
--------------------------------------------------------

/* Read sensors */
shell@android:/ # utest_sensor read -t Acc -d 1
--------------------------------------------------------
utest_sensor -- read
do_read: type'Acc-1' delay (1)
sensor=Acc, time=5675850915000, value=<  0.6, -0.4,  9.8>
sensor=Acc, time=5675950923000, value=< -0.3, -1.1,  9.6>
sensor=Acc, time=5676050928000, value=<  1.6, -1.1,  9.4>
sensor=Acc, time=5676150921000, value=<  3.0, -1.1,  9.1>
sensor=Acc, time=5676251054000, value=<  4.6, -1.3,  8.3>
sensor=Acc, time=5676350924000, value=<  6.1, -1.3,  7.2>
sensor=Acc, time=5676450917000, value=<  7.2, -1.5,  6.1>
sensor=Acc, time=5676550915000, value=<  7.5, -1.8,  4.2>
sensor=Acc, time=5676650915000, value=<  7.5, -1.8,  5.1>
sensor=Acc, time=5676751031000, value=<  6.4, -2.9,  5.8>
sensor=Acc, time=5676850900000, value=<  3.9, -2.9,  6.9>
       ::               ::               ::
       ::               ::               ::
--------------------------------------------------------

/* verify sensors */
shell@android:/ # utest_sensor verify -t ACCELEROMETER -d 100 -o show
utest_sensor -- verify
-------------------------------------------------------------
ACCELEROMETER  Verify_option : show
	tilt		--	Tilt your device all directions.
	install_dir	--	Put the device in the following 3 ways:
				1. Face up 3s
				2. Head up 3s
				3. Left side down 3s
	stability	--	Keep device still in 30s.
	   ::   		   ::
	   ::   		   ::
-------------------------------------------------------------
shell@android:/ # utest_sensor verify -t ACCELEROMETER -d 100 -o tilt
utest_sensor -- verify
-------------------------------------------------------------
ACCELEROMETER  Verify_option : tilt
X: OK | Y: -- | Z: --
X: OK | Y: -- | Z: OK
X: OK | Y: OK | Z: OK
Success!
-------------------------------------------------------------
shell@android:/ # utest_sensor verify -t ACCELEROMETER -d 100 -o install_dir
utest_sensor -- verify
-------------------------------------------------------------
ACCELEROMETER  Verify_option : install_dir
-->Face up
-->Head up
-->Left side down
-->Face up
   ::
-------------------------------------------------------------
shell@android:/ # utest_sensor verify -t ACCELEROMETER -d 100 -o stability
utest_sensor -- verify
-------------------------------------------------------------
ACCELEROMETER  Verify_option : stability
wait 30s ...
Accuracy = 98.11%
pass!
-------------------------------------------------------------
