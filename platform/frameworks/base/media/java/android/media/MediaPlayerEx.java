/* Create by Spreadst */

package android.media;

/**
 * Add for MediaPlayer decoupling
 */
public class MediaPlayerEx extends MediaPlayer {
    /**
     * Add Drm feature When press home key in video when playing a drm file, right
     * should not be consumed. So a new release method is added with a parameter to indicate consume
     * or not.
     */
    public void setNeedToConsume(boolean isConsume) {
        setNeedConsume(isConsume);
    }

    /**
     *  Add Drm feature
     * @hide
     */
    private native void setNeedConsume(boolean isConsume);
}
