
package com.sprd.gallery3d.gif;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import com.android.gallery3d.R;
import com.sprd.gallery3d.drm.GifDecoderUtils;

public class GifDecoder {

    private static final String TAG = "GifDecoder";
    private static final boolean DEBUG_GIF = true;

    // to define some error type
    public static final int STATUS_OK = 0;
    public static final int STATUS_FORMAT_ERROR = 1;
    public static final int STATUS_OPEN_ERROR = 2;
    public static final int STATUS_DECODING = -1;

    private int status;

    private InputStream in;

    private int width; // full image width
    private int height; // full image height
    private boolean gctFlag; // global color table used
    private int gctSize; // size of global color table
    private int loopCount = 1; // iterations; 0 = repeat forever

    private int[] gct; // global color table
    private int[] lct; // local color table
    private int[] act; // active color table

    private int bgIndex; // background color index
    private int bgColor; // background color
    private int lastBgColor; // previous bg color
    private int pixelAspect; // pixel aspect ratio

    private boolean lctFlag; // local color table flag
    private boolean interlace; // interlace flag
    private int lctSize; // local color table size

    private int ix, iy, iw, ih; // current image rectangle
    private int lrx, lry, lrw, lrh;
    private Bitmap image; // current frame
    private Bitmap lastImage; // previous frame

    private byte[] block = new byte[256]; // current data block
    private int blockSize = 0; // block size

    // last graphic control extension info
    private int dispose = 0;
    // 0=no action; 1=leave in place; 2=restore to bg; 3=restore to prev
    private int lastDispose = 0;
    private boolean transparency = false; // use transparent color
    private int delay = 0; // delay in milliseconds
    private int transIndex; // transparent color index

    private static final int MaxStackSize = 4096;
    // max decoder pixel stack size

    // LZW decoder working arrays
    private short[] prefix;
    private byte[] suffix;
    private byte[] pixelStack;
    private byte[] pixels;

    // private Vector<GifFrame> frames; // frames read from current file
    private int frameCount;

    private GifFrame mFirstFrame;
    private GifFrame mCurrentFrame;
    private Context mContext;

    private boolean isRun = true;
    private PlayThreadController mPlayThreadController;

    private DecodeHandler mDecodeHandler;
    private HandlerThread mDecodeThread;

    public GifFrameCache mGifFrameCache;
    private final static Object DECODE_LOCK = new Object();
    private int mCurrentPlayFrameCount;
    private DecodeRunnable mDecodeRunnable;
    private Bitmap mBitmap;

    private void wakeupLock() {
        if (frameCount - mCurrentPlayFrameCount < 10) {
            synchronized (DECODE_LOCK) {
                DECODE_LOCK.notifyAll();
            }
        }
    }

    public Bitmap getDisplayBitmap() {
        try {
            mCurrentPlayFrameCount++;
            if (mCurrentPlayFrameCount > frameCount) {
                mCurrentPlayFrameCount = 0;
            }
            mBitmap = mGifFrameCache.getFormCache(mCurrentPlayFrameCount);
            if (mBitmap == null) {
                restartDecode();
            }
            return mBitmap;
        } finally {
            wakeupLock();
        }

    }

    private void resetDecoder() {
        if (DEBUG_GIF) {
            Log.d(TAG, "GifDecoder: reset the decoder-----");
        }
        if (in != null) {
            try {
                in.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
            in = null;
        }
        width = 0;
        height = 0;
        gctFlag = false;
        gctSize = 0;
        loopCount = 1;
        gct = null;
        lct = null;
        act = null;
        bgIndex = 0;
        bgColor = 0;
        lastBgColor = 0;
        pixelAspect = 0;
        lctFlag = false;
        interlace = false;
        lctSize = 0;
        ix = 0;
        iy = 0;
        iw = 0;
        ih = 0;
        lrx = 0;
        lry = 0;
        lrw = 0;
        lrh = 0;
        if (image != null) {
            image.recycle();
            image = null;
        }
        if (lastImage != null) {
            lastImage.recycle();
            lastImage = null;
        }
        blockSize = 0;
        dispose = 0;
        lastDispose = 0;
        transparency = false;
        prefix = null;
        suffix = null;
        pixelStack = null;
        pixels = null;
        /*
         * if (frames != null) { frames.clear(); frames = null; } else { frames
         * = new Vector<GifDecoder.GifFrame>(); }
         */
        frameCount = -1;
        status = STATUS_DECODING;
        recycleFrames();
        mFirstFrame = null;
        mCurrentFrame = null;
        mCurrentPlayFrameCount = -1;
    }

    public GifDecoder() {
        initDecodeThread();
    }

    public GifDecoder(Context context) {
        setcontext(context);
        initDecodeThread();
    }

    public GifDecoder(Context context, PlayThreadController controller) {
        setcontext(context);
        setPlayThreadController(controller);
        initDecodeThread();
    }

    private void initDecodeThread() {
        if (mDecodeThread == null) {
            mDecodeThread = new HandlerThread("decode_thread");
            mDecodeThread.start();
        }
        mDecodeHandler = new DecodeHandler(mDecodeThread.getLooper());
        mGifFrameCache = new GifFrameCache();
    }

    /*
     * WHEN QUIT OF DECODER THIS METHOD MUST BE CALLED AFTER PALY THREAD BEEN
     * STOPED.
     */
    public void quitDecoder() {

        if (mDecodeHandler != null) {
            mDecodeHandler.stop();
        }
        if (mDecodeThread != null) {
            mDecodeThread.quit();
        }
        mDecodeHandler = null;
        mDecodeThread = null;
        mContext = null;
        recycleFrames();
        mGifFrameCache = null;
        if (lastImage != null) {
            lastImage.recycle();
            lastImage = null;
        }
        if (image != null) {
            image.recycle();
            image = null;
        }
        mPlayThreadController = null;
    }

    public void setPlayThreadController(PlayThreadController controller) {
        this.mPlayThreadController = controller;
    }

    public void setcontext(Context context) {
        this.mContext = context;
    }

    public boolean isFinish() {
        return status == STATUS_OK;
    }

    private int read(String path) {
        return read(new File(path));
    }

    private int read(Uri uri) {
        if (mContext == null) {
            throw new NullPointerException(
                    "mContext is null, need initialized it!");
        }
        InputStream is = null;
        try {
            /* SPRD: Drm feature start @{ */
            if (GifDecoderUtils.getInstance().isReadDrmUri()) {
                is = GifDecoderUtils.getInstance().readDrmUri();
            } else {
                is = mContext.getContentResolver().openInputStream(uri);
            }
            /* SPRD: Drm feature end @} */
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            status = STATUS_OPEN_ERROR;
        }
        return read(is);
    }

    private int read(File file) {
        InputStream is = null;
        if (file.exists()) {
            try {
                is = new FileInputStream(file);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
        }
        return read(is);
    }

    public int read(InputStream is) {
        if (!isRun) {
            if (DEBUG_GIF) {
                Log.d(TAG, "GifDecoder: the decode is not want to be started -----");
            }
            status = STATUS_OPEN_ERROR;
            return status;
        }
        if (mPlayThreadController == null) {
            throw new NullPointerException("the mPlayThreadController is null, need initialized!");
        }
        status = STATUS_DECODING;
        mPlayThreadController.lock();
        resetDecoder();
        try {
            if (is != null) {
                in = is;
            } else {
                status = STATUS_OPEN_ERROR;
                return status;
            }
            decode();
        } catch (Exception e) {
            e.printStackTrace();
            status = STATUS_FORMAT_ERROR;
            Log.e(TAG, "decode exception : ", e);
        } finally {
            try {
                if (in != null) {
                    in.close();
                    if (DEBUG_GIF) {
                        Log.d(TAG, "GifDecoder: input stream has closed -----");
                    }
                }
            } catch (IOException e) {
                Log.w(TAG, "find exception when close inputstream : " + e.getMessage());
            }
        }
        return status;
    }

    private void decode() {
        if (DEBUG_GIF) {
            Log.d(TAG, "GifDecoder: start to decode-----");
        }
        readHeader();
        if (!err()) {
            readContents();
            if (frameCount < 0) {
                status = STATUS_FORMAT_ERROR;
            }
        }
    }

    private void recycleFrames() {
        if (DEBUG_GIF) {
            Log.d(TAG, "GifDecoder: start to recycle the last decoded gif.");
        }
        if (mFirstFrame == null) {
            return;
        }
        GifFrame cache = mFirstFrame;
        GifFrame temp = null;
        while (cache.next != null) {
            temp = cache.next;
            cache.next = null;
            cache = temp;
        }
        if(mGifFrameCache != null)
            mGifFrameCache.reset();
    }

    public GifFrame getFirstFrame() {
        return mFirstFrame;
    }

    // some public method @{
    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public int getLoopCount() {
        return loopCount;
    }

    public int getFrameCount() {
        return frameCount;
    }

    private boolean err() {
        return status == STATUS_FORMAT_ERROR || status == STATUS_OPEN_ERROR;
    }

    private void readHeader() {
        String id = "";
        for (int i = 0; i < 6; i++) {
            id += (char) read();
        }
        if (!id.startsWith("GIF")) {
            status = STATUS_FORMAT_ERROR;
            return;
        }
        readLSD();
        if (gctFlag && !err()) {
            gct = readColorTable(gctSize);
            // SPRD for bug 502899,[coverity] CID110627 gct maybe is null
            if(gct == null) return;
            bgColor = gct[bgIndex];
        }
    }

    private int[] readColorTable(int ncolors) {
        int nbytes = 3 * ncolors;
        int[] tab = null;
        byte[] c = new byte[nbytes];
        int n = 0;
        try {
            n = in.read(c);
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (n < nbytes) {
            status = STATUS_FORMAT_ERROR;
        } else {
            tab = new int[256]; // max size to avoid bounds checks
            int i = 0;
            int j = 0;
            while (i < ncolors) {
                int r = ((int) c[j++]) & 0xff;
                int g = ((int) c[j++]) & 0xff;
                int b = ((int) c[j++]) & 0xff;
                tab[i++] = 0xff000000 | (r << 16) | (g << 8) | b;
            }
        }
        return tab;
    }

    private void readLSD() {
        // logical screen size
        width = readShort();
        height = readShort();
        // packed fields
        int packed = read();
        gctFlag = (packed & 0x80) != 0; // 1 : global color table flag
        // 2-4 : color resolution
        // 5 : gct sort flag
        gctSize = 2 << (packed & 7); // 6-8 : gct size
        bgIndex = read(); // background color index
        pixelAspect = read(); // pixel aspect ratio

    }

    private int readShort() {
        // read 16-bit value, LSB first
        return read() | (read() << 8);

    }

    private int read() {
        int curByte = 0;
        try {
            curByte = in.read();
        } catch (Exception e) {
            Log.e(TAG, " exception at read input stream : " + e.getMessage());
            status = STATUS_FORMAT_ERROR;
        }
        return curByte;
    }

    public void readContents() {
        // read GIF file content blocks
        boolean done = false;
        while (!(done || err())) {
            if (!isRun) {
                status = STATUS_FORMAT_ERROR;
                return;
            }
            if (frameCount - mCurrentPlayFrameCount > (GifFrameCache.MAX_CACHE_SIZE - 1)) {
                synchronized (DECODE_LOCK) {
                    try {
                        DECODE_LOCK.wait();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
            int code = read();
            switch (code) {
                case 0x2C: // image separator
                    readImage();
                    break;
                case 0x21: // extension
                    code = read();
                    switch (code) {
                        case 0xf9: // graphics control extension
                            readGraphicControlExt();
                            break;

                        case 0xff: // application extension
                            readBlock();
                            String app = "";
                            for (int i = 0; i < 11; i++) {
                                app += (char) block[i];
                            }
                            if (app.equals("NETSCAPE2.0")) {
                                readNetscapeExt();
                            } else {
                                skip(); // don't care
                            }
                            break;
                        default: // uninteresting extension
                            skip();
                    }
                    break;
                case 0x3b: // terminator
                    if (mCurrentFrame != mFirstFrame) {
                        mCurrentFrame.next = mFirstFrame;
                    } else {
                        if (DEBUG_GIF) {
                            Log.d(TAG, "GifDecoder: this gif may has only one frame.");
                        }
                        mPlayThreadController.lock();
                    }
                    if (DEBUG_GIF) {
                        Log.d(TAG, "GifDecoder: decode finished-----");
                    }
                    status = STATUS_OK;
                    done = true;
                    break;
                case 0x00: // bad byte, but keep going and see what happens
                    break;
                default:
                    status = STATUS_FORMAT_ERROR;
            }
        }
    }

    private void skip() {
        do {
            readBlock();
        } while ((blockSize > 0) && !err());
    }

    private void readNetscapeExt() {
        do {
            readBlock();
            if (block[0] == 1) {
                // loop count sub-block
                int b1 = ((int) block[1]) & 0xff;
                int b2 = ((int) block[2]) & 0xff;
                loopCount = (b2 << 8) | b1;
            }
        } while ((blockSize > 0) && !err());
    }

    private int readBlock() {
        blockSize = read();
        int n = 0;
        if (blockSize > 0) {
            try {
                int count = 0;
                while (n < blockSize) {
                    count = in.read(block, n, blockSize - n);
                    if (count == -1) {
                        break;
                    }
                    n += count;
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
            if (n < blockSize) {
                status = STATUS_FORMAT_ERROR;
            }
        }
        return n;
    }

    private void readGraphicControlExt() {
        read(); // block size
        int packed = read(); // packed fields
        dispose = (packed & 0x1c) >> 2; // disposal method
        if (dispose == 0) {
            dispose = 1; // elect to keep old image if discretionary
        }
        transparency = (packed & 1) != 0;
        delay = readShort() * 10; // delay in milliseconds
        transIndex = read(); // transparent color index
        read(); // block terminator
    }

    private void readImage() {
        // offset of X
        ix = readShort(); // (sub)image position & size
        // offset of Y
        iy = readShort();
        // width of bitmap
        iw = readShort();
        // height of bitmap
        ih = readShort();

        // Local Color Table Flag
        int packed = read();
        lctFlag = (packed & 0x80) != 0; // 1 - local color table flag

        // Interlace Flag, to array with interwoven if ENABLE, with order
        // otherwise
        interlace = (packed & 0x40) != 0; // 2 - interlace flag
        // 3 - sort flag
        // 4-5 - reserved
        lctSize = 2 << (packed & 7); // 6-8 - local color table size
        if (lctFlag) {
            lct = readColorTable(lctSize); // read table
            // SPRD for bug 502899,[coverity] CID110626 gct maybe is null
            if(lct == null) return;
            act = lct; // make local table active
        } else {
            act = gct; // make global table active
            if (bgIndex == transIndex) {
                bgColor = 0;
            }
        }
        int save = 0;
        if (transparency) {
            save = act[transIndex];
            act[transIndex] = 0; // set transparent color if specified
        }
        if (act == null) {
            status = STATUS_FORMAT_ERROR; // no color table defined
        }
        if (err()) {
            return;
        }
        decodeImageData(); // decode pixel data
        skip();
        if (err()) {
            return;
        }
        frameCount++;
        // create new image to receive frame data
        image = setPixels(); // transfer pixel data to image
        // SPRD: fix bug 339429 the image maybe null when occur OOM
        if (image == null)
            return;
        mGifFrameCache.addToCache(frameCount, image);
        GifFrame frame = new GifFrame(delay);

        // create the link list
        if (isRun) {
            if (mFirstFrame == null) {
                mFirstFrame = frame;
                mCurrentFrame = mFirstFrame;
                if (DEBUG_GIF) {
                    Log.d(TAG, "GifDecoder: the first frame been decoded-----");
                }
                mPlayThreadController.unlock();
            } else {
                mCurrentFrame.next = frame;
                mCurrentFrame = frame;
            }

            if (transparency) {
                act[transIndex] = save;
            }
        }
        resetFrame();
    }

    private void resetFrame() {
        lastDispose = dispose;
        lrx = ix;
        lry = iy;
        lrw = iw;
        lrh = ih;
        lastImage = image;
        lastBgColor = bgColor;
        dispose = 0;
        transparency = false;
        delay = 0;
        lct = null;
    }

    private Bitmap setPixels() {
        /* SPRD: fix bug 348285 the gif picture is too large to decode @{ */
        // int[] dest = dest = new int[width * height];
        int[] dest;
        try {
            dest = new int[width * height];
        } catch (OutOfMemoryError e) {
            Log.e(TAG, "bitmap is too large : width * height = " + width * height, e);
            Toast.makeText(mContext, R.string.gif_decode_failed, Toast.LENGTH_SHORT).show();
            return null;
        }
        /* @} */
        // fill in starting image contents based on last image's dispose code
        if (lastDispose > 0) {
            /*
             * if (lastDispose == 3) { // use image before last int n =
             * frameCount - 2; if (n > 0) { lastImage = getFrame(n - 1).image; }
             * else { lastImage = null; } }
             */
            if (mCurrentFrame != null) {
                lastImage = mGifFrameCache.getFormCache(frameCount - 1);
            } else {
                lastImage = null;
            }
            if (lastImage != null && !lastImage.isRecycled()) {
                lastImage.getPixels(dest, 0, width, 0, 0, width, height);
                // copy pixels
                if (lastDispose == 2) {
                    // fill last image rect area with background color
                    int c = 0;
                    if (!transparency) {
                        c = lastBgColor;
                    }
                    for (int i = 0; i < lrh; i++) {
                        int n1 = (lry + i) * width + lrx;
                        int n2 = n1 + lrw;
                        for (int k = n1; k < n2; k++) {
                            dest[k] = c;
                        }
                    }
                }
            }
        }

        // copy each source line to the appropriate place in the destination
        int pass = 1;
        int inc = 8;
        int iline = 0;
        for (int i = 0; i < ih; i++) {
            int line = i;
            if (interlace) {
                if (iline >= ih) {
                    pass++;
                    switch (pass) {
                        case 2:
                            iline = 4;
                            break;
                        case 3:
                            iline = 2;
                            inc = 4;
                            break;
                        case 4:
                            iline = 1;
                            inc = 2;
                    }
                }
                line = iline;
                iline += inc;
            }
            line += iy;
            if (line < height) {
                int k = line * width;
                int dx = k + ix; // start of line in dest
                int dlim = dx + iw; // end of dest line
                if ((k + width) < dlim) {
                    dlim = k + width; // past dest edge
                }
                int sx = i * iw; // start of line in source
                while (dx < dlim) {
                    // map color and insert in destination
                    int index = ((int) pixels[sx++]) & 0xff;
                    int c = act[index];
                    if (c != 0) {
                        dest[dx] = c;
                    }
                    dx++;
                }
            }
        }
        Bitmap bm;
        try {
            bm = Bitmap.createBitmap(dest, width, height, Config.RGB_565);
        } catch (OutOfMemoryError e) {
            Log.e(TAG, "create bitmap out of memory :", e);
            return null;
        }
        return bm;
    }

    private void decodeImageData() {
        int NullCode = -1;
        int npix = iw * ih;
        int available, clear, code_mask, code_size, end_of_information, in_code, old_code, bits, code, count, i, datum, data_size, first, top, bi, pi;

        if ((pixels == null) || (pixels.length < npix)) {
            try {
                pixels = new byte[npix]; // allocate new pixel array
            } catch (OutOfMemoryError e) {
                Log.d(TAG, "decodeImageData -> npix is too large, do nothing.");
                Toast.makeText(mContext, R.string.gif_decode_failed, Toast.LENGTH_SHORT).show();
                return;
            }
        }
        if (prefix == null) {
            prefix = new short[MaxStackSize];
        }
        if (suffix == null) {
            suffix = new byte[MaxStackSize];
        }
        if (pixelStack == null) {
            pixelStack = new byte[MaxStackSize + 1];
        }
        // Initialize GIF data stream decoder.
        data_size = read();
        clear = 1 << data_size;
        end_of_information = clear + 1;
        available = clear + 2;
        old_code = NullCode;
        code_size = data_size + 1;
        code_mask = (1 << code_size) - 1;
        for (code = 0; code < clear; code++) {
            prefix[code] = 0;
            suffix[code] = (byte) code;
        }
        /* SPRD: fix bug 524155,gif decoder error,gif can't display all. @{ */
        // Decode GIF pixel stream.
        datum = bits = count = first = top = pi = bi = 0;
        for (i = 0; i < npix;) {
            if (bits < code_size) {
                if (count == 0) {
                    // Read a new data block.
                    count = readBlock();
                    if (count <= 0) {
                        break;
                    }
                    bi = 0;
                }
                datum += (((int) block[bi]) & 0xff) << bits;
                bits += 8;
                bi++;
                count--;
                continue;
            }

            // Get the next code.
            code = datum & code_mask;
            datum >>= code_size;
            bits -= code_size;

            // Interpret the code.
            if (code == clear) {
                // Reset decoder.
                code_size = data_size + 1;
                code_mask = (1 << code_size) - 1;
                available = clear + 2;
                old_code = NullCode;
                continue;
            }
            if (code > available) {
                break;
            }
            if (code == end_of_information) {
                break;
            }

            if (old_code == NullCode) {
                pixelStack[top++] = suffix[code];
                old_code = code;
                first = code;
                continue;
            }
            in_code = code;
            if (code >= available) {
                pixelStack[top++] = (byte) first;
                code = old_code;
            }
            while (code >= clear) {
                pixelStack[top++] = suffix[code];
                code = prefix[code];
            }
            first = ((int) suffix[code]) & 0xff;
            pixelStack[top++] = (byte) first;

            // Add a new string to the string table.
            if (available < MaxStackSize) {
                prefix[available] = (short) old_code;
                suffix[available] = (byte) first;
                available++;
                if (((available & code_mask) == 0) && (available < MaxStackSize)) {
                    code_size++;
                    code_mask += available;
                }
            }
            old_code = in_code;

            while (top > 0) {
                // Pop a pixel off the pixel stack.
                top--;
                pixels[pi++] = pixelStack[top];
                i++;
            }
        }
        /* @} */

        for (i = pi; i < npix; i++) {
            pixels[i] = 0; // clear missing pixels
        }
    }

    // the frame of a gif
    static class GifFrame {
        // to access image & delay w/o interface
        public int delay;
        public GifFrame next;

        public GifFrame(int del) {
            delay = del;
        }

    }

    interface PlayThreadController {
        public void lock();

        public void unlock();
    }

    public class DecodeRunnable implements Runnable {
        // private InputStream mIs;
        private Uri mUri;
        private boolean isContune;

        /*
         * public DecodeRunnable(InputStream is){ this.mIs = is; this.isContune
         * = true; }
         */

        public DecodeRunnable(Uri uri) {
            this.mUri = uri;
            isContune = true;
            synchronized (DECODE_LOCK) {
                DECODE_LOCK.notifyAll();
            }
        }

        @Override
        public void run() {
            if (!this.isContune) {
                return;
            }
            isRun = true;
             read(this.mUri);
        }

        public void quit() {
            this.isContune = false;
            isRun = false;
        }
    }

    public class DecodeHandler extends Handler {
        private DecodeRunnable mTempDecodeRunnable;

        public DecodeHandler(Looper looper) {
            super(looper);
        }

        public void postDecode(DecodeRunnable decode) {
            stop();
            mTempDecodeRunnable = decode;
            post(mTempDecodeRunnable);
        }

        public void stop() {
            if (mTempDecodeRunnable != null) {
                mTempDecodeRunnable.quit();
                removeCallbacks(mTempDecodeRunnable);
                mTempDecodeRunnable = null;
            }
        }
    }

    public void startDecode(Uri uri) {
        if (DEBUG_GIF) {
            Log.d(TAG, "GifDecoder put work to the loop-----");
        }
        /* SPRD: Drm feature start @{ */
        GifDecoderUtils.getInstance().initDrm(uri);
        /* SPRD: Drm feature end @} */
        mDecodeRunnable = new DecodeRunnable(uri);
        mDecodeHandler.postDecode(mDecodeRunnable);
    }

    private void restartDecode() {
        if (mGifFrameCache.getFormCache(0) == null && mDecodeRunnable != null) {
            mDecodeHandler.post(mDecodeRunnable);
        }
    }
}
