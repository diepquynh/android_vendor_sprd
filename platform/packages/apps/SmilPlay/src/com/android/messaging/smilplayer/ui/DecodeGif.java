/*
 * Copyright (c) 2012 Thunder Software Technology Co.,Ltd.
 * All Rights Reserved.
 * Thundersoft Confidential and Proprietary.
 * Developed by Thundersoft Engineering team.
 */
package com.android.messaging.smilplayer.ui;

import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Vector;

//import com.android.mms.ui.MessageUtils;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.net.Uri;
import android.util.Log;

public class DecodeGif{

	private static final String TAG = "GifPlayerActivity_GIFDecode";
	public static final long MAX_BUFFER_SIZE_ONCE_LOAD = 10024;
    public static final int FRAMES_STATUS_NEED_INIT = 0;
    public static final int FRAMES_STATUS_PREPARING = 1;
    public static final int FRAMES_STATUS_INITED = 2;

	public static final int STATUS_OK = 0;
	public static final int STATUS_FORMAT_ERROR = 1;
	public static final int STATUS_OPEN_ERROR = 2;

	protected InputStream mInputStream;
	protected int status;

	protected int width; // full image width
	protected int height; // full image height
	protected boolean gctFlag; // global color table used
	protected int gctSize; // size of global color table
	protected int loopCount = 1; // iterations; 0 = repeat forever

	protected int[] gct; // global color table
	protected int[] lct; // local color table
	protected int[] act; // active color table

	protected int bgIndex; // background color index
	protected int bgColor; // background color
	protected int lastBgColor; // previous bg color
       protected int pixelAspect; // pixel aspect ratio

	protected boolean lctFlag; // local color table flag
	protected boolean interlace; // interlace flag
	protected int lctSize; // local color table size

	protected int ix, iy, iw, ih; // current image rectangle
	protected int lrx, lry, lrw, lrh;

	protected Bitmap lastImage; // previous frame
	protected int frameindex = 0;

	protected int iLoadedSize = 0;

	protected byte[] block = new byte[256]; // current data block
	protected int blockSize = 0; // block size

	// last graphic control extension info
	protected int dispose = 0;
	// 0=no action; 1=leave in place; 2=restore to bg; 3=restore to prev
	protected int lastDispose = 0;
	protected boolean transparency = false; // use transparent color
	protected int delay = 0; // delay in milliseconds
	protected int transIndex; // transparent color index

	protected static final int MaxStackSize = 4096;
	// max decoder pixel stack size

	// LZW decoder working arrays
	protected short[] prefix;
	protected byte[] suffix;
	protected byte[] pixelStack;
	protected byte[] pixels;

	protected GifFrames frames1; // frames1 read from current file
	protected GifFrames frames2; // frames2 read from current file
    protected GifFrames mCurrentFrames; // current frames to show var gallery
    protected GifFrames mFramesForLoad; // current frames to load gif

	protected int frameCount;
    protected int framesCount;
    protected int mFramesTotal;
	protected int maxFrameSize;
    protected GifFrame mFirstFrame;
    protected GifFrame mCurrFrame;
    protected GifFrame mReservedFrame;

    protected Context mContext;
    protected Uri mUri;

    protected boolean loadFrontFrame;

    private boolean bExit;

    private boolean isOsDebug(){
         return true;
    }

    public void setbExit(boolean bExit) {
        synchronized (this) {
            this.bExit = bExit;
            notify();
        }
    }

    public DecodeGif(Context context, Uri uri) {
        if (isOsDebug()) {
        Log.d(TAG, "DecodeGif enter");
        }
        mContext = context;
        mUri = uri;
    }

	public void interrupt(){
		status = STATUS_OPEN_ERROR;
	}

	protected Bitmap setPixels() {
		int[] dest = new int[width * height];
		// fill in starting image contents based on last image's dispose code
		if (lastDispose > 0) {
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

        Bitmap image; // current frame
		try {

			image = Bitmap.createBitmap(dest, width, height, Config.RGB_565);
		} catch (OutOfMemoryError er) {
			Log.e(TAG, "setPixels got OutOfMemory", er);
			image = null;
			System.gc();
		}
		return image;
	}

	public GifFrame getFirstFrame() {
        if (isOsDebug()) {
	    Log.d(TAG, "getFirstFrame enter");
	    }
	    if (mFirstFrame == null || mFirstFrame.image == null) {
	        if (isOsDebug()) {
	        Log.d(TAG, "getFirstFrame mFirstFrame null");
	        }
        }else {
            if (isOsDebug()) {
            Log.d(TAG, "getFirstFrame mFirstFrame not null");
            }
        }
        return mFirstFrame;
	}

	public synchronized GifFrame next() {
        if (mCurrentFrames == null
                || !freshCurrentFrames()
                || mCurrentFrames.mFrames.size() <= 0) {
            if (isOsDebug()) {
            Log.d(TAG, "next return null");
            }
            return null;
        }

		mCurrFrame = (GifFrame) mCurrentFrames.mFrames.elementAt(frameindex++);
		return mCurrFrame;
	}

	private boolean freshCurrentFrames() {
        if (isOsDebug()) {
        Log.d(TAG, "freshCurrentFrames enter");
        }
        if (mCurrentFrames == null) {
            mCurrentFrames = frames1;
            if (isOsDebug()) {
            Log.d(TAG, "freshCurrentFrames mCurrentFrames = frames1");
            }
        }

        if (mCurrentFrames.status != FRAMES_STATUS_INITED) {
            return false;
        }

        if (mCurrentFrames.mFrames.size() == 0
                || mCurrentFrames.mFrames.size() == frameindex) {
            mCurrentFrames.status = FRAMES_STATUS_NEED_INIT;
            mCurrentFrames = mCurrentFrames.equals(frames1) ? frames2 : frames1;

            notify();
            if (isOsDebug()) {
            Log.d(TAG, "freshCurrentFrames notify end");
            }
            frameindex = 0;

			// if (mCurrentFrames.equals(frames1)) {
			// Log.d(TAG, "freshCurrentFrames set mCurrentFrames to frames1");
			// }else {
			// Log.d(TAG, "freshCurrentFrames set mCurrentFrames to frames2");
			// }
        }
        if (isOsDebug()) {
        Log.d(TAG, "freshCurrentFrames end");
        }
        return true;
    }
	public void loadGif() {
        for (;!bExit;) {
            try {
                ContentResolver cr = mContext.getContentResolver();
                mInputStream = cr.openInputStream(mUri);

                readHeader();
                if (!err()) {
                    readContents();
                }
                recycleFrames();
                mInputStream.close();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
                status = STATUS_OPEN_ERROR;
                return;
            } catch (Exception e) {
                e.printStackTrace();
                status = STATUS_OPEN_ERROR;
                return;
            }
            //MMz04 - 20120425 -Fixed bug 15780 - Catch OutOfMemoryError.
            catch (OutOfMemoryError e) {
                e.printStackTrace();
            }
            //MMz04 - 20120425 -Fixed bug 15780 - End
            if (isOsDebug()) {
            Log.d(TAG, "loadGif for loop end");
            }
        }
		recycleFrames();
        if (isOsDebug()) {
        Log.d(TAG, "loadGif end");
        }
	}

	protected void decodeImageData() {
		int NullCode = -1;
		int npix = iw * ih;
		int available, clear, code_mask, code_size, end_of_information, in_code, old_code, bits, code, count, i, datum, data_size, first, top, bi, pi;

		if ((pixels == null) || (pixels.length < npix)) {
			pixels = new byte[npix]; // allocate new pixel array
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

		// Decode GIF pixel stream.
		datum = bits = count = first = top = pi = bi = 0;
		for (i = 0; i < npix;) {
			if (top == 0) {
				if (bits < code_size) {
					// Load bytes until there are enough bits for a code.
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

				// Interpret the code
				if ((code > available) || (code == end_of_information)) {
					break;
				}
				if (code == clear) {
					// Reset decoder.
					code_size = data_size + 1;
					code_mask = (1 << code_size) - 1;
					available = clear + 2;
					old_code = NullCode;
					continue;
				}
				if (old_code == NullCode) {
					pixelStack[top++] = suffix[code];
					old_code = code;
					first = code;
					continue;
				}
				in_code = code;
				if (code == available) {
					pixelStack[top++] = (byte) first;
					code = old_code;
				}
				while (code > clear) {
					pixelStack[top++] = suffix[code];
					code = prefix[code];
				}
				first = ((int) suffix[code]) & 0xff;
				// Add a new string to the string table,
				if (available >= MaxStackSize) {
					break;
				}
				pixelStack[top++] = (byte) first;
				prefix[available] = (short) old_code;
				suffix[available] = (byte) first;
				available++;
				if (((available & code_mask) == 0)
						&& (available < MaxStackSize)) {
					code_size++;
					code_mask += available;
				}
				old_code = in_code;
			}

			// Pop a pixel off the pixel stack.
			top--;
			pixels[pi++] = pixelStack[top];
			i++;
		}
		for (i = pi; i < npix; i++) {
			pixels[i] = 0; // clear missing pixels
		}
        if (isOsDebug()) {
		Log.d(TAG, "decodeImageData end");
		}
	}

	protected boolean err() {
		return status != STATUS_OK;
	}

	protected void init() {
		status = STATUS_OK;
		frameCount = 0;
		frames1 = new GifFrames();
		frames2 = new GifFrames();
		mFramesForLoad = frames1;
		mCurrentFrames = frames1;
		gct = null;
		lct = null;
		loadFrontFrame = true;
		switchFramesForLoad();
        if (isOsDebug()) {
		Log.d(TAG, "init end");
		}
	}

	protected int read() {
		int curByte = 0;
		try {
			curByte = mInputStream.read();
		} catch (Exception e) {
		    Log.d(TAG, "read exception");
		    e.printStackTrace();
			status = STATUS_FORMAT_ERROR;
		}
		return curByte;
	}

	protected int readBlock() {
		blockSize = read();
		int n = 0;
		if (blockSize > 0) {
			try {
				int count = 0;
				while (n < blockSize) {
					count = mInputStream.read(block, n, blockSize - n);
					if (count == -1) {
						break;
					}
					n += count;
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			if (n < blockSize) {
				status = STATUS_FORMAT_ERROR;
			}
		}
		return n;
	}

	protected int[] readColorTable(int ncolors) {
		int nbytes = 3 * ncolors;
		int[] tab = null;
		byte[] c = new byte[nbytes];
		int n = 0;
		try {
			n = mInputStream.read(c);
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

	protected void readContents() {
		// read GIF file content blocks
		boolean done = false;
		while (!(done || err()) && !bExit) {
			int code = read();
	        if (isOsDebug()) {
			Log.d(TAG, "readContents loop read code = " + code);
			}
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
				done = true;
		        if (isOsDebug()) {
				Log.d(TAG, "readContents loop code done = " + done);
				}
				break;
			case 0x00: // bad byte, but keep going and see what happens
				break;
			default:
		        if (isOsDebug()) {
			    Log.d(TAG, "readContents loop default code = " + code);
			    }
				status = STATUS_FORMAT_ERROR;
			}
		}
        if (isOsDebug()) {
		Log.d(TAG, "readContents while exit done: " + done + " err() : " + err() + " bExit : " + bExit);
		}
	}

	protected void readGraphicControlExt() {
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

	protected void readHeader() {
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
			bgColor = gct[bgIndex];
		}
	}

	private synchronized void prepareForReadImage() {
        try {
            if (frames1.status == FRAMES_STATUS_INITED  && frames2.status == FRAMES_STATUS_INITED) {
                if (isOsDebug()) {
                Log.d(TAG, "readImage wait() frames1.status: " + frames1.status + "frames2.status: " + frames2.status);
                }
                wait();
                if (isOsDebug()) {
                Log.d(TAG, "readImage wait() end frames1.status: " + frames1.status + "frames2.status: " + frames2.status);
                }
            }

            recycleFrames();
            if (mFramesForLoad == null || mFramesForLoad.status == FRAMES_STATUS_INITED) {
                switchFramesForLoad();
            }

        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

	protected void readImage() {
	    prepareForReadImage();

		ix = readShort(); // (sub)image position & size
		iy = readShort();
		iw = readShort();
		ih = readShort();
		int packed = read();
		lctFlag = (packed & 0x80) != 0; // 1 - local color table flag
		interlace = (packed & 0x40) != 0; // 2 - interlace flag
		// 3 - sort flag
		// 4-5 - reserved
		lctSize = 2 << (packed & 7); // 6-8 - local color table size
		if (lctFlag) {
			lct = readColorTable(lctSize); // read table
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
		try {
			decodeImageData(); // decode pixel data
		} catch (OutOfMemoryError e) {
	         resetFrameAsError();
	         return;
		}
		skip();
		if (err()) {
			return;
		}

		try {
	        Bitmap image = setPixels(); // transfer pixel data to image
			if(image == null || mFramesForLoad == null){
	            resetFrameAsError();
	            return;
			}
			GifFrame gifFrameTemp = new GifFrame(image, delay);

			if (mFirstFrame == null) {
		        if (isOsDebug()) {
			    Log.d(TAG, "delay " + gifFrameTemp.delay);
			    }
			    mFirstFrame = gifFrameTemp;
            }

			mFramesForLoad.mFrames.addElement(gifFrameTemp); // add image to frame
			frameCount++;

			// list
			if (transparency) {
				act[transIndex] = save;
			}
			lastImage = image;
			resetFrame();

            if (maxFrameSize == 0) {
                if (isOsDebug()) {
                Log.d(TAG, "readImage maxFrameSize == 0");
                }
                iLoadedSize += image.getRowBytes();
                if (iLoadedSize > MAX_BUFFER_SIZE_ONCE_LOAD) {
                    maxFrameSize = frameCount;
                    if (isOsDebug()) {
                    Log.d(TAG, "readImage maxFrameSize =" + maxFrameSize + " , iLoadedSize = " + iLoadedSize);
                    }
                }
            }

            //load first 10 frames to frames1 to improve performance
            if (loadFrontFrame ) {
                if (frameCount == 10) {
                    mFramesForLoad.status = FRAMES_STATUS_INITED;
                    loadFrontFrame = false;
                    iLoadedSize = 0;
                    frameCount = 0;
                    if (isOsDebug()) {
                    Log.d(TAG, "readImage set loadFrontFrame :false");
                    }
                }
            }

            if (maxFrameSize != 0 && mFramesForLoad.mFrames.size() >= maxFrameSize) {
                mFramesForLoad.status = FRAMES_STATUS_INITED;
                loadFrontFrame = false;
                iLoadedSize = 0;
                frameCount = 0;
            }
            if (isOsDebug()) {
            Log.d(TAG, "readImage exit frameCount = " + frameCount);
            }
		} catch (OutOfMemoryError e) {
		    Log.e(TAG, "readImage OutOfMemoryError");
            resetFrameAsError();
		}
	}

	private void switchFramesForLoad() {
	    if(frames1.status == FRAMES_STATUS_NEED_INIT){
	        if (isOsDebug()) {
	        Log.d(TAG, "switchFramesForLoad set to frames1");
	        }
	        mFramesForLoad = frames1;
	    } else if (frames2.status == FRAMES_STATUS_NEED_INIT){
	        if (isOsDebug()) {
	        Log.d(TAG, "setFramesForLoad set to frames2");
	        }
            mFramesForLoad = frames2;
	    } else {        
	        if (isOsDebug()) {
	        Log.d(TAG, "setFramesForLoad do nothing");
	        }
	        //do nothing
            return;
        }

	    mFramesForLoad.status = FRAMES_STATUS_PREPARING;
	    frameCount = 0;
    }

	protected void readLSD() {
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

	protected void readNetscapeExt() {
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

	protected int readShort() {
		// read 16-bit value, LSB first
		return read() | (read() << 8);
	}


	private void resetFrameAsError() {
		lastDispose = dispose;
		lrx = ix;
		lry = iy;
		lrw = iw;
		lrh = ih;
		dispose = 0;
		transparency = false;
		delay = 0;
		lct = null;
	}

	protected void resetFrame() {
		lastDispose = dispose;
		lrx = ix;
		lry = iy;
		lrw = iw;
		lrh = ih;
		lastBgColor = bgColor;
		dispose = 0;
		transparency = false;
		delay = 0;
		lct = null;
	}

	/**
	 * Skips variable length blocks up to and including next zero length block.
	 */
	protected void skip() {
		do {
			readBlock();
		} while ((blockSize > 0) && !err());
	}

    protected void recycleFrames() {
        if (isOsDebug()) {
        Log.d(TAG, "------recycleFrames----- bForce " + bExit );
        }

        recycleFrames1();
        recycleFrames2();

        if (bExit) {
            frameCount = 0;
            if (mReservedFrame!= null && mReservedFrame.image != null) {
                mReservedFrame.image.recycle();
                mReservedFrame.image = null;
            }
        }
    }

    protected void recycleFrames1() {
        if (null != frames1 && (frames1.status == FRAMES_STATUS_NEED_INIT || bExit) && frames1.mFrames.size() > 0) {
            if (isOsDebug()) {
            Log.d(TAG, "------recycleFrames-----frames1");
            }
            int count = 0;
            while (count < frames1.mFrames.size() -1) {
                if (isOsDebug()) {
                Log.d(TAG, "------recycleFrames-----frames1 count:" + count);
                }
                GifFrame indexFrame = (GifFrame) frames1.mFrames.elementAt(count);
                if (indexFrame.image != null && !indexFrame.image.isRecycled()) {
                    indexFrame.image.recycle();
                    indexFrame.image = null;
                }
                count++;
            }
            if (mReservedFrame!= null && mReservedFrame.image != null) {
                if (isOsDebug()) {
                Log.d(TAG, "------recycleFrames----- frame1 count " + count);
                }
                mReservedFrame.image.recycle();
                mReservedFrame.image = null;
            }
            mReservedFrame = frames1.mFrames.elementAt(count);
            frames1.mFrames.clear();
            frames1.status = FRAMES_STATUS_NEED_INIT;
        }
    }

    protected void recycleFrames2() {
        if (null != frames2 && (frames2.status == FRAMES_STATUS_NEED_INIT || bExit) && frames2.mFrames.size() > 0) {
            if (isOsDebug()) {
            Log.d(TAG, "------recycleFrames-----frames2");
            }
            int count = 0;
            while (count < frames2.mFrames.size() - 1) {
                if (isOsDebug()) {
                Log.d(TAG, "------recycleFrames-----frames2 count:" + count);
                }
                GifFrame indexFrame = (GifFrame) frames2.mFrames.elementAt(count);
                if (indexFrame.image != null && !indexFrame.image.isRecycled()) {
                    indexFrame.image.recycle();
                    indexFrame.image = null;
                }
                count++;
            }

            if (mReservedFrame!= null && mReservedFrame.image != null) {
                mReservedFrame.image.recycle();
                mReservedFrame.image = null;
            }
            mReservedFrame = frames2.mFrames.elementAt(count);
            frames2.mFrames.clear();
            frames2.status = FRAMES_STATUS_NEED_INIT;
        }
    }

    static class GifFrame {
        public GifFrame(Bitmap im, int del) {
            image = im;
            delay = del;
        }

        public Bitmap image;

        public int delay;
    }

    static class GifFrames {
        public GifFrames() {
            mFrames = new Vector<GifFrame>();
            status = FRAMES_STATUS_NEED_INIT;
        }

        public Vector<GifFrame> mFrames;

        //the status of mFrames(need init\data preparing\inited)
        private int status;
    }
}
