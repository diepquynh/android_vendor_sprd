/* Create by Spreadst */

package android.media;

/**
 * Add for MediaRecorder decoupling
 */
public class MediaRecorderEx extends MediaRecorder {
     /**
     * SPRD:set the file offset. if use64BitFlag is true, this condition can make sure the duration of videorecord(>= 720p) more than one hour.@{
     *
     * @param use64BitFlag the file offset.false is use32Bit, true is use64Bit.
     */
    public void setParam64BitFileOffset(boolean use64BitFlag)
    {
        if (use64BitFlag) {
            setParameter("param-use-64bit-offset=1");
        } else {
            setParameter("param-use-64bit-offset=0" );
        }
    }
    /**  @}*/
}
