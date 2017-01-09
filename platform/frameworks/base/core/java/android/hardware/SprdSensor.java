package android.hardware;


/**
 * SPRD: add some new sensor type
 *
 * @hide
 */

public final class SprdSensor {

    private static final int SPRD_SENSOR_TYPE_BASE = 0x10000;

    /**
     * A constant describing a shake sensor.
     *
     * A sensor of this type triggers when the device is shaken.
     * This sensor deactivates itself immediately after it triggers.
     *
     * @hide This sensor is expected to only be used by the System Application.
     */
    public static final int TYPE_SPRDHUB_SHAKE = SPRD_SENSOR_TYPE_BASE + 1;

    /**
     * A constant string describing a shake sensor.
     *
     * @hide
     */
    public static final String STRING_TYPE_SPRDHUB_SHAKE = "sprd.sensorhub.shake";

    /**
     * A constant describing a pocket-mode sensor.
     *
     * A sensor of this type triggers when the device is put into a pocket.
     * The only allowed return value is 1.0. This sensor deactivates
     * itself immediately after it triggers.
     *
     * @hide This sensor is expected to only be used by the System Application.
     */
    public static final int TYPE_SPRDHUB_POCKET_MODE = SPRD_SENSOR_TYPE_BASE + 2;

    /**
     * A constant string describing a pocket-mode sensor.
     *
     * @hide
     */
    public static final String STRING_TYPE_SPRDHUB_POCKET_MODE = "sprd.sensorhub.pocket_mode";

    /**
     * A constant describing a tap sensor.
     *
     * A sensor of this type triggers when the device is tapped single or twice.
     * This sensor deactivates itself immediately after it triggers.
     *
     * @hide Expected to be used by system core service.
     */
    public static final int TYPE_SPRDHUB_TAP = SPRD_SENSOR_TYPE_BASE + 3;

    /**
     * A constant string describing a tap sensor.
     *
     * @hide
     */
    public static final String STRING_TYPE_SPRDHUB_TAP = "sprd.sensorhub.tap";

    /**
     * A constant describing a face-up/face-down sensor.
     *
     * A sensor of this type triggers when the device is faced up or faced down.
     * This sensor deactivates itself immediately after it triggers.
     *
     * @hide Expected to be used by System Application.
     */
    public static final int TYPE_SPRDHUB_FACE_UP_DOWN = SPRD_SENSOR_TYPE_BASE + 4;

    /**
     * A constant string describing a face-up/face-down sensor.
     *
     * @hide
     */
    public static final String STRING_TYPE_SPRDHUB_FACE_UP_DOWN = "sprd.sensorhub.face_up_down";

    /**
     * A constant describing a flip sensor.
     *
     * A sensor of this type triggers when the device is fliped.
     * reporting-mode: on_change.
     *
     * @hide Expected to be used by System Application.
     */
    public static final int TYPE_SPRDHUB_FLIP = SPRD_SENSOR_TYPE_BASE + 10;

    /**
     * A constant string describing a flip sensor.
     *
     * @hide
     */
    public static final String STRING_TYPE_SPRDHUB_FLIP = "sprd.sensorhub.flip";

    /**
     * A constant describing a hand-up sensor.
     *
     * A sensor of this type triggers when the device is handed up
     * reporting mode: on_change
     *
     * @hide Expected to be used by System Application.
     */
    public static final int TYPE_SPRDHUB_HAND_UP = SPRD_SENSOR_TYPE_BASE + 12;

    /**
     * A constant string describing a hand-up sensor.
     *
     * @hide
     */
    public static final String STRING_TYPE_SPRDHUB_HAND_UP = "sprd.sensorhub.hand_up";

    /**
     * A constant describing a hand-down sensor.
     *
     * A sensor of this type triggers when the device is handed down
     * reporting mode: on_change
     *
     * @hide Expected to be used by System Application.
     */
    public static final int TYPE_SPRDHUB_HAND_DOWN = SPRD_SENSOR_TYPE_BASE + 13;

    /**
     * A constant string describing a hand-down sensor.
     *
     * @hide
     */
    public static final String STRING_TYPE_SPRDHUB_HAND_DOWN = "sprd.sensorhub.hand_down";

}

