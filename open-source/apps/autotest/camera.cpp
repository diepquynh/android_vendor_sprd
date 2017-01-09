// 
// Spreadtrum Auto Tester
//
// anli   2012-11-29
//
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
//#include <asm/page.h>
#include <linux/ion.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "type.h"
#include "camera.h"
#include <ion_sprd.h>

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_cam {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//using namespace android;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define DCAM_DEV                    "/dev/video0"
#define ION_DEV                     "/dev/ion"

#define CAM_DEF_FMT                 V4L2_PIX_FMT_YUV420
#define CAM_DEF_PIXER_SIZE          2
#define CAM_BUFFER_NUM              2
//------------------------------------------------------------------------------
struct cam_buffer_t {
    uchar * phys_addr;
    uchar * virt_addr;
    uint    length;
};

static int                s_ion_fd   = -1;
static int                s_cam_fd   = -1;
static int                s_width    = 0;
static int                s_height   = 0;
static volatile int       s_starting = 0;
static int                sCurCamBuf = -1;
static pthread_t          sptid      = -1;
static pthread_mutex_t    sMutxEvent = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t     sCondEvent = PTHREAD_COND_INITIALIZER;
static cam_buffer_t       sCamBufs[CAM_BUFFER_NUM];
//------------------------------------------------------------------------------
//
static void * dcam_thread( void * param );
static int    dcam_open_dev( void );
static void   dcam_close_dev( void );
static int    xioctl( int fd, int cmd, void * arg );
static int    dcam_verify_cap( uint cap );
static int    dcam_init( int cam_id );
static int    dcam_alloc_mem( void );
static void   dcam_free_mem( void );
static int    dcam_streamon( void );
static int    dcam_streamoff( void );
static int    dcam_querybuf( int index );
static int    dcam_reqbufs( void );
static int    dcam_qbuf( int index );
static int    dcam_dqbuf( struct v4l2_buffer *buf );

inline uint roundUpToPageSize(uint x) {
        return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int camOpen( int cam_id, int width, int height )
{
    if( -1 != s_cam_fd ) {
        WRNMSG("already started!\n");
        return 0;
    }
    
    s_width  = width;
    s_height = height;
    
    if( dcam_open_dev() < 0 ) {
        return -1;
    }
    
    if( dcam_verify_cap(V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING) < 0 ) {
        ERRMSG("Not support capture&streaming \n");
        dcam_close_dev();
        return -2;
    }
    
    if( dcam_init(cam_id) < 0 ) {
        ERRMSG("dcam init fail!\n");
        dcam_close_dev();
        return -3;
    }
    
    if( dcam_alloc_mem() < 0 ) {
        ERRMSG("dcam alloc mem fail!\n");
        return -4;
    }

    if( dcam_reqbufs() < 0 ) {
        dcam_close_dev();
        ERRMSG("dcam req bufs fail!\n");
        return -5;
    }

    return 0;
}

//------------------------------------------------------------------------------
int camStart( void )
{
	FUN_ENTER;
	if( dcam_streamon() < 0 ) {
        ERRMSG("dcam streamon fail!\n");
        return -1;
    }

	sCurCamBuf = -1;
    s_starting = 1;
    sptid      = -1;
    pthread_create(&sptid, NULL, dcam_thread, NULL);

	FUN_EXIT;
	return 0;
}

//------------------------------------------------------------------------------
static void * dcam_thread( void * param )
{
	DBGMSG("dcam_thread enter...\n");

    struct v4l2_buffer buf;
    
    uint cnt = 5;
    //FILE * pf = fopen("/mnt/sdcard/cam.data", "w");
    while( s_starting && cnt-- ) {
        //
        if( dcam_dqbuf(&buf) < 0) {
            continue;
        }

        AT_ASSERT( buf.index < CAM_BUFFER_NUM );
        AT_ASSERT( (uchar *)(buf.m.userptr) == sCamBufs[buf.index].phys_addr );
        
        DBGMSG("Cam Data Ready: index = %d, phy addr = %p\n", buf.index, (uchar *)(buf.m.userptr));
        
        //if( pf ) {
        //    fwrite(sCamBufs[buf.index].virt_addr, 1, sCamBufs[buf.index].length, pf);
        //}
        pthread_mutex_lock(&sMutxEvent);
        DBGMSG("cur cam buf = %d\n", buf.index);
        sCurCamBuf = buf.index; 
        pthread_cond_signal(&sCondEvent);
        pthread_mutex_unlock(&sMutxEvent);
        
        if( 1 == cnt ) {
            INFMSG("capture done, data buffer index = %d.\n", sCurCamBuf);
            break;
        }

        usleep(30 * 1000);
        if( !s_starting ) {
			break;
		}

        pthread_mutex_lock(&sMutxEvent);
        DBGMSG("cur cam buf invalid\n");
        sCurCamBuf = -1;    
        pthread_mutex_unlock(&sMutxEvent);
        //
        if( dcam_qbuf(buf.index) < 0) {
            continue;
        }
    }
    
    //if( pf ) {
    //    fclose(pf);
    //}
    DBGMSG("dcam_thread exit: cnt = %d\n", cnt);
    return NULL;
}

//------------------------------------------------------------------------------
int camGetData( uchar * buffer, uint size )
{
	FUN_ENTER;
    int ret = -1;
    pthread_mutex_lock(&sMutxEvent);
    if( sCurCamBuf >= 0 && sCurCamBuf < CAM_BUFFER_NUM ) {
        DBGMSG("cur cam %d data is ready.\n", sCurCamBuf);
        ret = 0;
    } else {
        struct timespec to;
        to.tv_nsec = 0;
        to.tv_sec  = time(NULL) + 1;
        
        pthread_cond_init(&sCondEvent, NULL);
        if( 0 == pthread_cond_timedwait(&sCondEvent, &sMutxEvent, &to) ) {
            DBGMSG("cam %d data arrived.\n", sCurCamBuf);
            AT_ASSERT( sCurCamBuf >= 0 && sCurCamBuf < CAM_BUFFER_NUM );
            ret = 0;
        } else {
            WRNMSG("pthread_cond_timedwait fail: %s\n", strerror(errno));
        }
    }
    
    if( 0 == ret ) {
        AT_ASSERT( buffer != NULL );
        AT_ASSERT( (int)size < s_width * s_height );
        
        uchar * data = sCamBufs[sCurCamBuf].virt_addr + (s_width * s_height - (size >> 1));
        memcpy(buffer, data, size);
    }
    
    pthread_mutex_unlock(&sMutxEvent);

	FUN_EXIT;
    return ret;
}

//------------------------------------------------------------------------------
int camStop( void )
{
	FUN_ENTER;
    if( -1 == s_cam_fd ) {
        WRNMSG("already stoped!\n");
        return 0;
    }
    
    s_starting = 0;
	dcam_streamoff();
	usleep(40 * 1000);

	if( -1 != sptid ) {
        int cnt = 10;
        while( cnt-- ) {
			int pk_ret = pthread_kill(sptid, 0);
			if( ESRCH == pk_ret || EINVAL == pk_ret ) {
				break;
			} else {
				WRNMSG("cam thread is running!\n");
				usleep(100 * 1000);
			}
        }
    }

    sptid = -1;

	FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
int camClose( void )
{
	FUN_ENTER;

	dcam_free_mem();

	if( s_cam_fd >= 0 ) {
		close(s_cam_fd);
		s_cam_fd = -1;
	}
	FUN_EXIT;
    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static int dcam_open_dev( void )
{  
    int counter = 5;

    while( (s_cam_fd < 0) && counter-- ) {
        s_cam_fd = open(DCAM_DEV, O_RDWR);

        if (s_cam_fd < 0) {
            DBGMSG("Cannot open '%s': %s", DCAM_DEV, strerror(errno));
            usleep(100 * 1000);
        } 
    }

    return s_cam_fd;
}

void dcam_close_dev( void )
{
    if( s_cam_fd >= 0 ) {
        close(s_cam_fd);
        s_cam_fd = -1;
    }
}

//------------------------------------------------------------------------------
static int xioctl( int fd, int cmd, void * arg )
{
    int r;

    do {
        r = ioctl(fd, cmd, arg);
    } while (-1 == r && EINTR == errno);
    return r;
    //return 0;
}

static int dcam_verify_cap( uint cap )
{
    AT_ASSERT( s_cam_fd >= 0 );
    
    struct v4l2_capability vcap;

    vcap.capabilities = 0;
    if( xioctl(s_cam_fd, VIDIOC_QUERYCAP, &vcap) < 0 ) {
        ERRMSG("Fail to VIDIOC_QUERYCAP: %s\n", strerror(errno));
    }

    return (vcap.capabilities & cap) ? 0 : -1;
}

//------------------------------------------------------------------------------
static int dcam_init( int cam_id )
{
    struct v4l2_streamparm vsprm;

    memset(&vsprm, 0, sizeof(vsprm));
    vsprm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    vsprm.parm.raw_data[196] = 1;  // YUV
    vsprm.parm.raw_data[197] = 0;  // no rotation
    vsprm.parm.raw_data[198] = (uchar)cam_id; // sensor id
    vsprm.parm.raw_data[199] = 1; // sensor id flag

    if( xioctl(s_cam_fd, VIDIOC_S_PARM, &vsprm) < 0 ) {
        ERRMSG("Fail to VIDIOC_S_PARM: %s\n", strerror(errno));
        return -1;
    }

    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = s_width;
    fmt.fmt.pix.height      = s_height;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.pixelformat = CAM_DEF_FMT;
    
    if( xioctl(s_cam_fd, VIDIOC_S_FMT, &fmt) < 0 ) {
        DBGMSG("Fail to VIDIOC_S_FMT: %s\n", strerror(errno));
        return -2;
    }

    DBGMSG("pixFormat       = %d\n", fmt.fmt.pix.pixelformat);
    DBGMSG("pixWidth        = %d\n", fmt.fmt.pix.width);
    DBGMSG("pixHeight       = %d\n", fmt.fmt.pix.height);
    DBGMSG("pixSizeimage    = %d\n", fmt.fmt.pix.sizeimage);
    DBGMSG("pixBytesperline = %d\n", fmt.fmt.pix.bytesperline);
 
    return 0;
}

static struct ion_handle_data s_ihdata;
static uchar *                s_vaddr;
static uint                   s_vsize;

static int dcam_alloc_mem( void )
{
    if( s_ion_fd >= 0 ) {
        WRNMSG("already alloced!\n");
        return 0;
    }
    
    int buf_size = roundUpToPageSize(s_width * s_height * CAM_DEF_PIXER_SIZE);
    int mem_size = CAM_BUFFER_NUM * buf_size;

    DBGMSG("buf size = %d, mem size = %d\n", buf_size, mem_size);

	int fd = open(ION_DEV, O_RDWR | O_SYNC);
	if( fd < 0 ) {
		ERRMSG("open '%s' error: %s\n", ION_DEV, strerror(errno));
		return -errno;
	}

	struct ion_allocation_data iadata;

	iadata.len   = mem_size;
    iadata.align = getpagesize();
    iadata.flags = ION_HEAP_CARVEOUT_MASK;
	if( ioctl(fd, ION_IOC_ALLOC, &iadata) < 0) {
		ERRMSG("ION_IOC_ALLOC error: %s\n", strerror(errno));
		close(fd);
        return -errno;
    }

	struct ion_fd_data     ifdata;
	struct ion_handle_data ihdata;

	ifdata.handle = iadata.handle;
	ihdata.handle = iadata.handle;

	if (ioctl(fd, ION_IOC_MAP, &ifdata) < 0) {
		ERRMSG("ION_IOC_MAP error: %s\n", strerror(errno));

		ioctl(fd, ION_IOC_FREE, &ihdata);
		close(fd);
		return -errno;
	}

	struct ion_phys_data   ipdata;
	struct ion_custom_data icdata;
	ipdata.fd_buffer = ifdata.fd;
	icdata.cmd = ION_SPRD_CUSTOM_PHYS;
	icdata.arg = (unsigned long)&ipdata;
	if( ioctl(fd, ION_IOC_CUSTOM, &icdata) < 0 ) {
		ERRMSG("ION_IOC_CUSTOM error: %s\n", strerror(errno));

		ioctl(fd, ION_IOC_FREE, &ihdata);
		close(fd);
		return -errno;
	}

	unsigned long phy_addr = ipdata.phys;
	int phy_size = ipdata.size;

	uchar * viraddr = (uchar *)mmap(0, mem_size, PROT_READ | PROT_WRITE,
				MAP_SHARED, ifdata.fd, 0);
	if( viraddr == MAP_FAILED ) {
		ERRMSG("mmap(fd=%d, size=%u) failed (%s)", fd, mem_size, strerror(errno));

		ioctl(fd, ION_IOC_FREE, &ihdata);
		close(fd);
		return -errno;
	}

    uchar * phyaddr = (uchar *)phy_addr;
    for( int i = 0; i < CAM_BUFFER_NUM; ++i ) {
        sCamBufs[i].virt_addr = viraddr + i * buf_size;
        sCamBufs[i].phys_addr = phyaddr + i * buf_size;
        sCamBufs[i].length    = buf_size;
        
        DBGMSG("vir addr = %p, phy addt = %p, len = %d\n",
            sCamBufs[i].virt_addr, sCamBufs[i].phys_addr, sCamBufs[i].length );
    }

	s_ion_fd = fd;
	s_vsize  = mem_size;
	s_vaddr  = viraddr;
	s_ihdata = ihdata;

    return 0;
}

static void dcam_free_mem( void )
{
	if( s_ion_fd >= 0 ) {
		munmap(s_vaddr, s_vsize);
		ioctl(s_ion_fd, ION_IOC_FREE, &s_ihdata);
		close(s_ion_fd);
		s_ion_fd = -1;
	}
}

//------------------------------------------------------------------------------
static int dcam_querybuf( int index )
{
	struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory    = V4L2_MEMORY_USERPTR;
    buf.index     = index;
    buf.m.userptr = (unsigned long)sCamBufs[index].phys_addr;
    buf.length    = sCamBufs[index].length;

    if( xioctl(s_cam_fd, VIDIOC_QUERYBUF, &buf) < 0 ) {
        ERRMSG("Fail to VIDIOC_QUERYBUF: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int dcam_reqbufs( void )
{
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));
    req.count  = CAM_BUFFER_NUM;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if( xioctl(s_cam_fd, VIDIOC_REQBUFS, &req) < 0 ) {
        DBGMSG("Fail to VIDIOC_REQBUFS: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int dcam_qbuf( int index )
{
    AT_ASSERT( index < CAM_BUFFER_NUM );
    
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory    = V4L2_MEMORY_USERPTR;
    buf.index     = index;
    buf.m.userptr = (unsigned long)sCamBufs[index].phys_addr;
    buf.length    = sCamBufs[index].length;

    DBGMSG("QBuf: userptr = %p, len = %d\n", (uchar *)(buf.m.userptr), buf.length);

    if( xioctl(s_cam_fd, VIDIOC_QBUF, &buf) < 0 ) {
        ERRMSG("Fail to VIDIOC_QBUF: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int dcam_dqbuf( struct v4l2_buffer *buf )
{
    memset(buf, 0, sizeof(struct v4l2_buffer));
    buf->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_USERPTR;
    if( xioctl(s_cam_fd, VIDIOC_DQBUF, buf) < 0 ) {
        ERRMSG("Fail to VIDIOC_DQBUF: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------------
static int dcam_streamon(void)
{
	dcam_querybuf(0);

	for( int i = 0; i < CAM_BUFFER_NUM; ++i ) {
        dcam_qbuf(i);
    }

    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if( xioctl(s_cam_fd, VIDIOC_STREAMON, &type) < 0 ) {
        ERRMSG("DCAM: Fail to VIDIOC_STREAMON: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
static int dcam_streamoff(void)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( xioctl(s_cam_fd, VIDIOC_STREAMOFF, &type) < 0 ) {
        DBGMSG("DCAM: Fail to VIDIOC_STREAMOFF: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
int flashlightSetValue(int value)
{
	int ret = 0;
	char cmd[200] = " ";

	FUN_ENTER;
	sprintf(cmd, "echo 0x%02x > /sys/class/flash_test/flash_test/flash_value", value);
	ret = system(cmd) ? -1 : 0;
	DBGMSG("cmd = %s,ret = %d\n", cmd,ret);
	FUN_EXIT;

	return ret;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
