/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.gif;

import java.io.InputStream;
import java.util.Vector;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.util.Log;

public class GifDecoder {
    private static final boolean VERBOSE = true;
    /**
     * The following is the decoder's status
     */
    public static final int QPHONE_STATUS_OK = 0;

    /**
     * QPHONE_STATUS_FORMAT_ERROR
     */
    public static final int QPHONE_STATUS_FORMAT_ERROR = 1000;

    /**
     * QPHONE_STATUS_OPEN_ERROR
     */
    public static final int QPHONE_STATUS_OPEN_ERROR = 2000;

    private static final String TAG = "QPhoneGifDecoder";

    protected int mWidth;

    protected int mHeight;

    // global color table used
    protected boolean mGctFlag;

    // size of global color table
    protected int mGctSize;

    // iterations; 0 = repeat forever
    protected int mLoopCount = 1;

    protected InputStream mInputStream;

    protected int mStatus;

    // global color table
    protected int[] mGct;

    // local color table
    protected int[] mLct;

    // active color table
    protected int[] mAct;

    // background color index
    protected int mBgIndex;

    // background color
    protected int mBgColor;

    // previous bg color
    protected int mLastBgColor;

    // pixel aspect ratio
    protected int mPixelAspect;

    // local color table flag
    protected boolean mLctFlag;

    // interlace flag
    protected boolean mInterlace;

    // local color table size
    protected int mLctSize;

    // current image rectangle
    protected int mIx, mIy, mIw, mIh;

    protected int mLrx, mLry, mLrw, mLrh;

    protected Bitmap mImage; // current frame

    protected Bitmap mLastImage; // previous frame

    protected int mFrameindex = 0;

    /**
     *
     * @return int
     */
    public int getFrameindex() {
        return mFrameindex;
    }

    /**
     *
     * @param frameindex frameindex
     */
    public void setFrameindex(int frameindex) {
        this.mFrameindex = frameindex;
        if (frameindex > mFrames.size() - 1) {
            frameindex = 0;
        }
    }

    protected byte[] mBlock = new byte[256]; // current data block

    protected int mBlockSize = 0; // block size

    // last graphic control extension info
    protected int mDispose = 0;

    /*
     * 0=no action; 1=leave in place; 2=restore to bg; 3=restore to prev
     */
    protected int mLastDispose = 0;

    protected boolean mTransparency = false; // use transparent color

    protected int mDelay = 0; // delay in milliseconds

    protected int mTransIndex; // transparent color index

    // max decoder pixel stack size
    protected static final int MaxStackSize = 4096;

    // LZW decoder working arrays
    protected short[] mPrefix;

    protected byte[] mSuffix;

    protected byte[] mPixelStack;

    protected byte[] mPixels;

    protected Vector mFrames; // frames read from current file

    protected int mFrameCount;

    static class QPhoneGifFrame {
        public QPhoneGifFrame(Bitmap im, int del) {
            image = im;
            delay = del;
        }

        public Bitmap image;

        public int delay;
    }

    /**
     *
     * @param n n
     * @return int
     */
    public int getQPhoneDelay(int n) {
        mDelay = -1;
        if ((n >= 0) && (n < mFrameCount)) {
            mDelay = ((QPhoneGifFrame) mFrames.elementAt(n)).delay;
        }
        return mDelay;
    }

    /**
     *
     * @return int
     */
    public int getQPhoneFrameCount() {
        return mFrameCount;
    }

    /**
     *
     * @return Bitmap
     */
    public Bitmap getQPhoneImage() {
        return getQPhoneFrame(0);
    }

    /**
     *
     * @return int
     */
    public int getLoopCount() {
        return mLoopCount;
    }

    protected void setQPhonePixels() {
        if(VERBOSE)
            Log.d(TAG, "----->>>>>setQphonePixels(): mWidth = " + mWidth + ", mHeight = " + mHeight);
        int[] dest = new int[mWidth * mHeight];
        // Log.d(TAG,
        // "fill in starting image contents based on last image's dispose code");
        if (mLastDispose > 0) {
            if (mLastDispose == 3) {
                // use image before last
                int n = mFrameCount - 2;
                if (n > 0) {
                    mLastImage = getQPhoneFrame(n - 1);
                } else {
                    mLastImage = null;
                }
            }

            if (mLastImage != null) {
                mLastImage.getPixels(dest, 0, mWidth, 0, 0, mWidth, mHeight);
                // copy pixels
                if (mLastDispose == 2) {
                    // Log.d(TAG,
                    // "fill last image rect area with background color");
                    int c = 0;
                    if (!mTransparency) {
                        c = mLastBgColor;
                    }
                    for (int i = 0; i < mLrh; i++) {
                        int n1 = (mLry + i) * mWidth + mLrx;
                        int n2 = n1 + mLrw;
                        for (int k = n1; k < n2; k++) {
                            dest[k] = c;
                        }
                    }
                }
            }
        }

        // Log.d(TAG,
        // "copy each source line to the appropriate place in the destination");
        int pass = 1;
        int inc = 8;
        int iline = 0;
        for (int i = 0; i < mIh; i++) {
            int line = i;
            if (mInterlace) {
                if (iline >= mIh) {
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
                            break;
                        default :
                            break;
                    }
                }
                line = iline;
                iline += inc;
            }

            line += mIy;
            if (line < mHeight) {
                int k = line * mWidth;
                int dx = k + mIx; // start of line in dest
                int dlim = dx + mIw; // end of dest line
                if ((k + mWidth) < dlim) {
                    dlim = k + mWidth; // past dest edge
                }
                int sx = i * mIw; // start of line in source
                while (dx < dlim) {
                    // Log.d(TAG, "map color and insert in destination");
                    int index = ((int) mPixels[sx++]) & 0xff;
                    int c = mAct[index];
                    if (c != 0) {
                        dest[dx] = c;
                    }
                    dx++;
                }
            }
        }

        try {
            mImage = Bitmap.createBitmap(dest, mWidth, mHeight, Config.ARGB_8888);
        } catch (OutOfMemoryError e) {
            mImage = null;
            // e.printStackTrace();
        } catch (Exception e) {
            // e.printStackTrace();
        }
    }

    /**
     *
     * @param n n
     * @return Bitmap
     */
    public Bitmap getQPhoneFrame(int n) {
        Bitmap im = null;
        if ((n >= 0) && (n < mFrameCount)) {
            im = ((QPhoneGifFrame) mFrames.elementAt(n)).image;
        }
        return im;
    }

    /**
     *
     * @return Bitmap
     */
    public Bitmap getQPhoneNext() {
        mFrameindex++;
        if (mFrameindex > mFrames.size() - 1 || mFrameindex <= 0) {
            mFrameindex = 0;
        }
        return ((QPhoneGifFrame) mFrames.elementAt(mFrameindex)).image;
    }

    /**
     *
     * @param is is
     * @return int
     */
    public int readQPhoneContent(InputStream is) {
        init();
        if (is != null) {
            mInputStream = is;
            readQPhoneHeader();
            if (!getQPhoneErrorStatus()) {
                readQPhoneContents();
                if (mFrameCount < 0) {
                    mStatus = QPHONE_STATUS_FORMAT_ERROR;
                }
            }
        } else {
            mStatus = QPHONE_STATUS_OPEN_ERROR;
        }

        try {
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return mStatus;
    }

    /* SPRD: fix bug 529313,gif decoder error,gif can't display all. @{ */
    protected void decodeQPhoneImageData() {
        int null_code = -1;
        int npix = mIw * mIh;
        int available, clear, code_mask, code_size, end_of_information, in_code, old_code, bits, code, count, i, datum, data_size, first, top, bi, pi;

        if ((mPixels == null) || (mPixels.length < npix)) {
            // Log.d(TAG, "allocate new pixel array");
            mPixels = new byte[npix];
        }

        if (mPrefix == null) {
            mPrefix = new short[MaxStackSize];
        }

        if (mSuffix == null) {
            mSuffix = new byte[MaxStackSize];
        }

        if (mPixelStack == null) {
            mPixelStack = new byte[MaxStackSize + 1];
        }

        // Log.d(TAG, "Initialize GIF data stream decoder.");
        data_size = readQPhoneContent();
        clear = 1 << data_size;
        end_of_information = clear + 1;

        available = clear + 2;
        old_code = null_code;
        code_size = data_size + 1;
        code_mask = (1 << code_size) - 1;

        for (code = 0; code < clear; code++) {
            mPrefix[code] = 0;
            mSuffix[code] = (byte) code;
        }

        // Log.d(TAG, "Decode qphone gif pixel stream.");
        datum = bits = count = first = top = pi = bi = 0;
        for (i = 0; i < npix;) {
            // if (top == 0) {
            if (bits < code_size) {
                // Log.d(TAG,
                // "Load bytes until there are enough bits for a code.");
                if (count == 0) {
                    // Log.d(TAG, "Read a new data block.");
                    count = readQPhoneBlock();
                    if (count <= 0) {
                        break;
                    }
                    bi = 0;
                }
                datum += (((int) mBlock[bi]) & 0xff) << bits;
                bits += 8;
                bi++;
                count--;
                continue;
            }

            // Log.d(TAG, "Get the next code.");
            code = datum & code_mask;
            datum >>= code_size;
            bits -= code_size;

            // Log.d(TAG, "Interpret the code");
            if ((code > available) || (code == end_of_information)) {
                break;
            }
            if (code == clear) {
                // Log.d(TAG, "Reset decoder.");
                code_size = data_size + 1;
                code_mask = (1 << code_size) - 1;
                available = clear + 2;
                old_code = null_code;
                continue;
            }
            if (old_code == null_code) {
                mPixelStack[top++] = mSuffix[code];
                old_code = code;
                first = code;
                continue;
            }
            in_code = code;
            if (code >= available) {
                mPixelStack[top++] = (byte) first;
                code = old_code;
            }
            while (code >= clear) {
                mPixelStack[top++] = mSuffix[code];
                code = mPrefix[code];
            }
            first = ((int) mSuffix[code]) & 0xff;
            mPixelStack[top++] = (byte) first;

            // Log.d(TAG, "Add a new string to the string table");
            /*
            if (available >= MaxStackSize) {
               break;
            }
            */
            if (available < MaxStackSize) {
                mPrefix[available] = (short) old_code;
                mSuffix[available] = (byte) first;
                available++;
                if (((available & code_mask) == 0) && (available < MaxStackSize)) {
                    code_size++;
                    code_mask += available;
                }
                old_code = in_code;
            }
            while (top > 0) {
                // Log.d(TAG, "Pop a pixel off the pixel stack.");
                top--;
                mPixels[pi++] = mPixelStack[top];
                i++;
            }
        }
        for (i = pi; i < npix; i++) {
            // Log.d(TAG, "clear missing pixels");
            mPixels[i] = 0;
        }
    }

    protected boolean getQPhoneErrorStatus() {
        return mStatus != QPHONE_STATUS_OK;
    }

    protected void init() {
        mStatus = QPHONE_STATUS_OK;
        mFrameCount = 0;
        mFrames = new Vector();
        mGct = null;
        mLct = null;
    }

    protected int readQPhoneContent() {
        int currentByte = 0;
        try {
            currentByte = mInputStream.read();
        } catch (Exception e) {
            mStatus = QPHONE_STATUS_FORMAT_ERROR;
        }
        return currentByte;
    }

    protected int readQPhoneBlock() {
        mBlockSize = readQPhoneContent();
        int n = 0;

        if (mBlockSize > 0) {
            try {
                int count = 0;
                while (n < mBlockSize) {
                    count = mInputStream.read(mBlock, n, mBlockSize - n);

                    if (count == -1) {
                        break;
                    }
                    n += count;
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            if (n < mBlockSize) {
                mStatus = QPHONE_STATUS_FORMAT_ERROR;
            }
        }
        return n;
    }

    protected int[] readQPhoneColorTable(int colors) {
        int bytes = 3 * colors;
        int[] tab = null;
        byte[] bytesArray = new byte[bytes];
        int n = 0;

        try {
            n = mInputStream.read(bytesArray);
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (n < bytes) {
            mStatus = QPHONE_STATUS_FORMAT_ERROR;
        } else {
            // Log.d(TAG, "max size to avoid bounds checks");
            tab = new int[256];

            int i = 0;
            int j = 0;
            while (i < colors) {
                int r = ((int) bytesArray[j++]) & 0xff;
                int g = ((int) bytesArray[j++]) & 0xff;
                int b = ((int) bytesArray[j++]) & 0xff;
                tab[i++] = 0xff000000 | (r << 16) | (g << 8) | b;
            }
        }
        return tab;
    }

    protected void readQPhoneContents() {
        // read Gif file content blocks
        boolean done = false;
        while (!(done || getQPhoneErrorStatus())) {
            int code = readQPhoneContent();

            switch (code) {
                case 0x2C: // image separator
                    readQPhoneImage();
                    break;

                case 0x21: // extension
                    code = readQPhoneContent();
                    switch (code) {
                        case 0xf9: // graphics control extension
                            readQPhoneGraphicControlExt();
                            break;

                        case 0xff: // application extension
                            readQPhoneBlock();
                            String app = "";
                            for (int i = 0; i < 11; i++) {
                                app += (char) mBlock[i];
                            }
                            if (app.equals("NETSCAPE2.0")) {
                                /*
                                 * we can follow
                                 * http://odur.let.rug.nl/~kleiweg/
                                 * gif/netscape.html for reference
                                 */
                                readQPhoneNetscapeExt();
                            } else {
                                skip(); // don't care
                            }
                            break;

                        default: // uninteresting extension
                            skip();
                    }
                    break;

                case 0x3b:
                    Log.d(TAG, "terminator, decode succeed");
                    done = true;
                    break;

                case 0x00:
                    // Log.d(TAG,
                    // "bad byte, but keep going and see what happens");
                    break;

                default:
                    mStatus = QPHONE_STATUS_FORMAT_ERROR;
            }
        }
    }

    protected void readQPhoneGraphicControlExt() {
        readQPhoneContent(); // block size
        int packed = readQPhoneContent(); // packed fields

        mDispose = (packed & 0x1c) >> 2; // disposal method
        if (mDispose == 0) {
            // Log.d(TAG, "select to keep old image if discretionary");
            mDispose = 1;
        }
        mTransparency = (packed & 1) != 0;
        mDelay = readQPhoneShort() * 10; // delay in milliseconds
        mTransIndex = readQPhoneContent(); // transparent color index
        readQPhoneContent(); // block terminator
    }

    protected void readQPhoneHeader() {
        String id = "";
        for (int i = 0; i < 6; i++) {
            id += (char) readQPhoneContent();
        }

        if (!id.startsWith("GIF")) {
            Log.e(TAG, "not start with GIF");
            mStatus = QPHONE_STATUS_FORMAT_ERROR;
            return;
        }

        readQPhoneLSD();
        if (mGctFlag && !getQPhoneErrorStatus()) {
            mGct = readQPhoneColorTable(mGctSize);
            if(mGct != null) {
                mBgColor = mGct[mBgIndex];
            } else {
                mStatus = QPHONE_STATUS_FORMAT_ERROR;
            }
        }
    }

    protected void readQPhoneImage() {
        mIx = readQPhoneShort(); // (sub)image position & size
        mIy = readQPhoneShort();
        mIw = readQPhoneShort();
        mIh = readQPhoneShort();

        int packed = readQPhoneContent();
        mLctFlag = (packed & 0x80) != 0; // 1 - local color table flag
        mInterlace = (packed & 0x40) != 0;
        // 2 - interlace flag
        // 3 - sort flag
        // 4-5 - reserved
        // 6-8 - local color table size
        mLctSize = 2 << (packed & 7);
        if (mLctFlag) {
            mLct = readQPhoneColorTable(mLctSize); // read table
            // Log.d(TAG, "make local table active");
            mAct = mLct;

        } else {
            // Log.d(TAG, "make global table active");
            mAct = mGct;
            if (mBgIndex == mTransIndex) {
                mBgColor = 0;
            }
        }

        int save = 0;
        /* SPRD: CID 110603 : Dereference null return value (NULL_RETURNS) @{ */
        if (mTransparency && mAct != null) {
        // if (mTransparency) {
        /* @} */
            // Log.d(TAG, "set transparent color if specified");
            save = mAct[mTransIndex];
            mAct[mTransIndex] = 0;

        }

        if (mAct == null) {
            Log.e(TAG, "no color table defined");
            mStatus = QPHONE_STATUS_FORMAT_ERROR;
        }
        if (getQPhoneErrorStatus()) {
            return;
        }

        // Log.d(TAG, "decode pixel data");
        decodeQPhoneImageData();
        skip();
        if (getQPhoneErrorStatus()) {
            return;
        }
        mFrameCount++;
        try {
            // Log.d(TAG, "create new image to receive frame data");
            mImage = Bitmap.createBitmap(mWidth, mHeight, Config.ARGB_8888);
        } catch (OutOfMemoryError e) {
            mImage = null;
            // e.printStackTrace();
        } catch (Exception e) {
            // e.printStackTrace();
        }

        // createImage(width, height);
        // Log.d(TAG, "transfer pixel data to image");
        setQPhonePixels();
        // Log.d(TAG, "add image to frame");
        mFrames.addElement(new QPhoneGifFrame(mImage, mDelay));
        // list
        if (mTransparency) {
            mAct[mTransIndex] = save;
        }
        resetQPhoneFrame();
    }

    protected void readQPhoneLSD() {
        // logical screen size
        mWidth = readQPhoneShort();
        mHeight = readQPhoneShort();
        // packed fields
        int packedField = readQPhoneContent();
        mGctFlag = (packedField & 0x80) != 0;
        // 1 : global color table flag
        // 2-4 : color resolution
        // 5 : gct sort flag
        // 6-8 : gct size
        mGctSize = 2 << (packedField & 7);
        mBgIndex = readQPhoneContent(); // background color index
        mPixelAspect = readQPhoneContent(); // pixel aspect ratio
    }

    protected void readQPhoneNetscapeExt() {
        do {
            readQPhoneBlock();
            if (mBlock[0] == 1) {
                // loop count sub-block
                int b1 = ((int) mBlock[1]) & 0xff;
                int b2 = ((int) mBlock[2]) & 0xff;
                mLoopCount = (b2 << 8) | b1;
            }
        } while ((mBlockSize > 0) && !getQPhoneErrorStatus());
    }

    protected int readQPhoneShort() {
        // Log.d(TAG, "read 16-bit value, LSB first");
        return readQPhoneContent() | (readQPhoneContent() << 8);
    }

    public void recycleBitmaps() {
        for (int i = 0; i < mFrameCount; i++) {
            recycleBitmap(((QPhoneGifFrame) mFrames.get(i)).image);
        }
    }
    public void clear() {
        mPrefix = null;
        mSuffix = null;
        mPixelStack = null;
        mPixels = null;
        mBlock = null;
    }
    private void recycleBitmap(Bitmap bmp) {
        if(bmp != null && !bmp.isRecycled()) {
            bmp.recycle();
        }
    }
    protected void resetQPhoneFrame() {
        mLastDispose = mDispose;

        mLrw = mIw;
        mLrh = mIh;

        mLrx = mIx;
        mLry = mIy;
        mDispose = 0;

        mLastImage = mImage;
        mLastBgColor = mBgColor;

        mTransparency = false;
        mDelay = 0;
        mLct = null;
    }

    /**
     * Skips variable length blocks up to and including next zero length block.
     */
    protected void skip() {
        do {
            readQPhoneBlock();
        } while ((mBlockSize > 0) && !getQPhoneErrorStatus());
    }
}
