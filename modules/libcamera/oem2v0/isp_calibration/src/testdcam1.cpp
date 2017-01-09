#include <getopt.h>


#define LOG_TAG   "main"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <utils/Log.h>
#include <camera/Camera.h>

#include "SprdCameraHardwareInterface.h"


using namespace android;

#define  MAX_ARGS    256
struct _options {
	char*  cameraId ;
	int workmode;
	int during;
	String8  param[MAX_ARGS];
	int    param_cnt;
	int    pic_cnt;
} ;
#define  WK_PREVIEW    0x00
#define  WK_RECVIDEO   0x01
#define  WK_TAKEPIC    0x02
#define  WK_TAKEPIC_RAW    0x03
#define  WK_AUTOFOCUS  0x04

static struct _options opts;
//static SprdCameraParameters  params;


const char *EXEC_NAME = "testdcam";
#define VERSION "1.1.4"


//SIGFPE
extern "C" {
	int pthread_stackshow();
}

#include <signal.h>

#include <setjmp.h>

#define SIZE	100

static void dump(void)
{
#if 0
#include <ucontext.h>
#include <execinfo.h>	/* for backtrace() */

	int j, nptrs;
	void *buffer[100];
	char **strings;

	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols fail");
		//exit(EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
		printf("backtrace: [%02d] %s\n", j, strings[j]);

	free(strings);
#endif
}

static jmp_buf env;
int     err;

static void msg(int signo)
{
 CMR_LOGI("Get SIGFPE %x!\n",signo);
 dump();
 pthread_stackshow();
 if(signo == SIGFPE || signo == SIGSEGV)
{
	sleep(opts.during);
	signal(signo, SIG_DFL);
	raise(signo);

//	longjmp( env, 1);
 } else {
 }
}

static void process_protect(void)
{
	char buf[64];
	pid_t pid;
	int fd;

	sigset_t sigset,oset;/*sigset\u5b58\u653e\u5c4f\u853d\u4fe1\u53f7\uff0coset\u4fdd\u5b58\u5f53\u524d\u5c4f\u853d\u4fe1\u53f7*/
	struct sigaction action1,action2;/*\u4fe1\u53f7\u5904\u7406*/


	CMR_LOGW("sprdcamtest protected 1");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR){
		CMR_LOGW("signal error\n");
	}

	CMR_LOGW("sprdcamtest protected 2");
	pid = getpid();
	bzero(buf, 64);
	snprintf(buf, 64, "/proc/%i/oom_adj",pid);
	fd = open(buf, O_RDWR);
	if(fd>=0){
		buf[0]=-17;
		write(fd,buf,1);
		close(fd);
		CMR_LOGW("sprdcamtest protected ok");
	}

	action1.sa_handler = msg;
#if 0
	sigaction(SIGFPE,&action1,&action2) ;
#else
	if(sigaction(SIGFPE,&action1,&action2) == (long)SIG_ERR )	{
		CMR_LOGE("sigaction fail! " );
		abort();
	}
	action1.sa_handler = msg;
	if(sigaction(SIGSEGV,&action1,&action2) == (long)SIG_ERR )	{
		CMR_LOGE("sigaction fail! " );
		abort();
	}
#endif
//	setjmp( env );
}



#if   0
/**
 * parse_options - Read and validate the programs command line
 * Read the command line, verify the syntax and parse the options.
 *
 * Return:   0 success, -1 error.
 */
int _parse_options(int argc, char *argv[])
{
	int c;

	static const char *sopt = "-hvVswg";
	static const struct option lopt[] = {
		{ "help",	 no_argument,		NULL, 'h' },
		{ "verbose",	 no_argument,		NULL, 'v' },
		{ "version",	 no_argument,		NULL, 'V' },

		{ "sensor", required_argument,		NULL, 's' },
		{ "workmode", required_argument,		NULL, 'w' },
		{ "args",		 required_argument,		NULL, 'g' },
		{ NULL,		 0,			NULL,  0  }
	};

	opterr = 0; /* We'll handle the errors, thank you. */

	while ((c = getopt_long(argc, argv, sopt, lopt, NULL)) != -1) {
		switch (c) {
		case 1:	/* A non-option argument */
			break;

		case 'h':
//			usage();
			exit(9);
		case 'n':
			break;
		case 'v':
			break;
		case 'V':
//			_log_info("%s %s %s %d\n", EXEC_NAME, VERSION,
//				      FUSE_TYPE, fuse_version());
			exit(0);

		case 's':
			opts.cameraId = atoi(optarg);
//			optind+=2;
			 break;

		case 'w':
			opts.workmode = atoi(optarg);
//			optind+=2;
			 break;

		case 'g':
		{
			char  * q;
			 while((q=strchr(optarg, ',')))
			{
				*q= ';';
			}
			 String8 str(optarg);
			 SprdCameraParameters p(str);
			 params =  p;
//			 optind+=2;
		}
			break;

		default:
//			_log_error("%s: Unknown option '%s'.\n", EXEC_NAME,
//				       argv[optind - 1]);
			return -1;
		}
	}

	return 0;
}

#else

int usage(void)
{
    fprintf(stderr,"usage: testdacm\n"
            "       --sensor  <sensorid>\n"
            "       --workmode <0~3>\n"
            "       --args  <preview-size=640x480,picture-size=640x480>\n"
			" 	  --args  zoom=1\n"
			"     ......  "
			);
    return  1;
}

/**
 * parse_options - Read and validate the programs command line
 * Read the command line, verify the syntax and parse the options.
 *
 * Return:   0 success, -1 error.
 */
int _parse_options(int argc, char *argv[])
{
    argc--;
    argv++;

	opts.cameraId=0;  //default
	opts.workmode=0;
	opts.during=-1;
    opts.param_cnt =0;

    while(argc > 0){
        char *arg = argv[0];
        char *val = argv[1];
        if(argc < 2) {
			CMR_LOGE("error %d\n",argc);
			return usage();
        }
        argc -= 2;
        argv += 2;
       if(!strcmp(arg, "--sensor")) {
		   CMR_LOGI("sensor id ->%s\n",val);
//            opts.cameraId = atoi(val);
            opts.cameraId = val;
          } else if(!strcmp(arg, "--workmode")) {
		   CMR_LOGI("workmode ->%s\n",val);
           opts.workmode = atoi(val);
        } else if(!strcmp(arg, "--args")) {
			if(opts.param_cnt>= MAX_ARGS  )
				continue;
	/*		char  * q;
			 while((q=strchr(val, ',')))
			{
				*q= ';';
			}*/
			 CMR_LOGI("new str->%s\n",val);
			 //String8 str(val);
			 //SprdCameraParameters p(str);
			 //params =  p;
			 opts.param[opts.param_cnt++]  = String8(val);

        } else if(!strcmp(arg, "--time")) {
		       CMR_LOGI("time ->%s\n",val);
	           opts.during = atoi(val);
        } else {
			    CMR_LOGE("error %s %s\n",arg, argv);
				return usage();
        }
    }

	if(opts.param_cnt == 0)
		return usage();

	return 0;
}
#endif



#define PRINTF_CONFIG  0
void mergepara( camera_device_t  *pdev, String8 &argstr)
{
	char * tmp = pdev->ops->get_parameters(pdev);
	CameraParameters def = CameraParameters(String8(tmp));
	char *a;
	char *b;

	pdev->ops->put_parameters(pdev,tmp);
#if   PRINTF_CONFIG
		CMR_LOGI("old config  =>\n");
	   a= const_cast < char *> ( def.flatten().string());
	    for (;;) {
	        b = strchr(a, ';');
	        if (b == 0) {
	            // If there's no semicolon, this is the last item.
	            String8 v(a);
				CMR_LOGI("%s",v.string());
	            break;
	        }
             else
             {
		        String8 v(a, (size_t)(b-a));
				CMR_LOGI("%s",v.string());
		         a = b+1;
             }
	   }
		CMR_LOGI("old config  end\n");
#endif

	   //a =const_cast < char *>  (add.flatten().string());
	   a =  (char *) argstr.string();
             CMR_LOGI("new str:  %s\n",a);

	    for (;;) {
	        // Find the bounds of the key name.
	        b = strchr(a, '=');
	        if (b == 0)
	            break;

	        // Create the key string.
	        String8 k(a, (size_t)(b-a));

	        // Find the value.
	        a = b+1;
	        b = strchr(a, ',');
	        if (b == 0) {
	            // If there's no semicolon, this is the last item.
		//		String8 v(a, strlen(a));
				CMR_LOGI("k=%s,v=%s",k.string(), a);
				def.set( k.string(), a);
	            break;
	        }
             else
             {
		        String8 v(a, (size_t)(b-a));
				CMR_LOGI("k=%s,v=%s",k.string(), v.string());
				def.set( k.string(),  v.string());
		         a = b+1;
             }
	   }

#if   PRINTF_CONFIG
	   CMR_LOGI("new config =>\n");
	   a=const_cast < char *>   ( def.flatten().string());
		for (;;) {
			b = strchr(a, ';');
			if (b == 0) {
				// If there's no semicolon, this is the last item.
				String8 v(a);
				CMR_LOGI("%s",v.string());
				break;
			}
			 else
			 {
			String8 v(a, (size_t)(b-a));
			CMR_LOGI("%s",v.string());
			 a = b+1;
			 }
	   }
	CMR_LOGI("new config  end\n");
#endif
	pdev->ops->set_parameters(pdev,def.flatten().string());

	if(opts.workmode == WK_TAKEPIC) {
		opts.pic_cnt = def.getInt("capture-mode");
		CMR_LOGW("WK_TAKEPIC:opts.pic_cnt = %d\n",opts.pic_cnt);
	}
}

static void _release_memory(struct camera_memory *mem)
{
	if(mem) {
		MemoryHeapBase	*mHeap = (MemoryHeapBase  *)mem->handle ;
		if(mHeap)
			delete mHeap;
		free(mem);
	}
}

camera_memory_t* get_memory(int fd, size_t buf_size, unsigned int num_bufs,
                                                  void *user)
{
	MemoryHeapBase  *mHeap ;
	camera_memory_t*  p  =  (camera_memory_t*) malloc(sizeof(camera_memory_t));
	if(fd>=0)
		mHeap = new MemoryHeapBase(fd, buf_size * num_bufs);
	else
		mHeap = new MemoryHeapBase(buf_size * num_bufs);

	if(mHeap&& p)
	{
		p->data = mHeap->getBase();
		p->size = buf_size * num_bufs;
		p->handle = mHeap;
		p->release= _release_memory;
		return p;
	}
	else
	{
		CMR_LOGI("maloc  0x%x,buf_size * num_bufs =%x * %x fail ",p,buf_size * num_bufs);
		return NULL;
	}
}

void notify_cb(int32_t msg_type,
        int32_t ext1,
        int32_t ext2,
        void *user)
{
	CMR_LOGI("notify_cb 0x%x\n",msg_type);

    switch (msg_type) {
		case CAMERA_MSG_ERROR:
			CMR_LOGI("CAMERA_MSG_ERROR");
            break;
        case CAMERA_MSG_SHUTTER:
			CMR_LOGI("CAMERA_MSG_SHUTTER");
            break;
		case CAMERA_MSG_FOCUS:
			CMR_LOGI("CAMERA_MSG_FOCUS");
			break;
		case CAMERA_MSG_FOCUS_MOVE:
			CMR_LOGI("CAMERA_MSG_FOCUS_MOVE");
			break;
		case CAMERA_MSG_RAW_IMAGE_NOTIFY:
			CMR_LOGI("CAMERA_MSG_RAW_IMAGE_NOTIFY");
			break;

        default:
            break;
    }
}


static struct {
	sem_t capture_sem;
	int  pic_cnt;
} takepic_action = {0};

void data_cb(int32_t msg_type,
        const camera_memory_t *data, unsigned int index,
        camera_frame_metadata_t *metadata, void *user)
{
	CMR_LOGI("data_cb 0x%x\n",msg_type);

    switch (msg_type) {
        case CAMERA_MSG_PREVIEW_FRAME:
//            client->handlePreviewData(msgType, dataPtr, metadata);
			CMR_LOGI("preview recieved frame %x\n",index);
            break;
		case CAMERA_MSG_PREVIEW_METADATA:
			CMR_LOGI("preview recieved FD frame %x\n",index);
            break;

        case CAMERA_MSG_POSTVIEW_FRAME:
//            client->handlePostview(dataPtr);
            break;
        case CAMERA_MSG_RAW_IMAGE:
//            client->handleRawPicture(dataPtr);
			CMR_LOGW("takepicture recieved raw picture\n");
            break;
        case CAMERA_MSG_COMPRESSED_IMAGE:
//            client->handleCompressedPicture(dataPtr);
			if(data) {
				if(takepic_action.pic_cnt == 0 || takepic_action.pic_cnt == opts.pic_cnt-1) {
//					sleep(20);  //for trace  dump
				}

				if(opts.pic_cnt <=1 || ++takepic_action.pic_cnt >= opts.pic_cnt){
					sem_post(&takepic_action.capture_sem);
					CMR_LOGW("takepicture all OK !!\n");
					takepic_action.pic_cnt = 0;
				} else {
					CMR_LOGW("takepicture %d OK !!\n",takepic_action.pic_cnt);
				}
			} else {
				CMR_LOGW("takepicture error happen !!\n");
				takepic_action.pic_cnt = 0;
				sem_post(&takepic_action.capture_sem);
			}
            break;

        default:
//            client->handleGenericData(msgType, dataPtr, metadata);
            break;
    }
}

void data_cb_timestamp(int64_t timestamp,
        int32_t msg_type,
        const camera_memory_t *data, unsigned int index,
        void *user)
{
	CMR_LOGI("data_cb_timestamp 0x%x\n",msg_type);
	if(msg_type == CAMERA_MSG_VIDEO_FRAME) {
		CMR_LOGI("recorder recieved frame %x\n",index);
	}
}


extern "C" {
	extern	struct camera_module HAL_MODULE_INFO_SYM ;
	//extern int write_otp_sn(unsigned char *data_buf, unsigned int data_size);

	int  start_camera(camera_device_t  **pdev)
	{
		int  ops_status 		= NO_ERROR;
		camera_device_t  *tmp 	= NULL;

		if(*pdev == NULL) {
			CMR_LOGW("pdev->common.methods->open \n");
			ops_status = HAL_MODULE_INFO_SYM.common.methods->open(&HAL_MODULE_INFO_SYM.common, opts.cameraId, (hw_device_t**)&tmp);
			if(ops_status != 0){
				CMR_LOGE("\n open %d fail ",opts.cameraId);
				if(tmp)
					tmp->common.close((hw_device_t*)tmp);  //avoid next time retry fail
				mtrace_print_alllog(TRACE_MEMSTAT);
				*pdev = NULL;
				return -1;
			}
			tmp->ops->set_callbacks(tmp,notify_cb,data_cb,data_cb_timestamp,get_memory,&opts.cameraId);
			tmp->ops->enable_msg_type(tmp, CAMERA_MSG_ALL_MSGS);
			tmp->ops->disable_msg_type(tmp, CAMERA_MSG_VIDEO_FRAME);
			//ops_status = tmp->ops->start_preview(tmp);
			if(ops_status != 0){
				CMR_LOGE("\n open %d fail ",opts.cameraId);
				if(tmp)
					tmp->common.close((hw_device_t*)tmp);  //avoid next time retry fail
				mtrace_print_alllog(TRACE_MEMSTAT);
				*pdev = NULL;
				return -1;
			}
			*pdev = tmp;
		}
		return ops_status;
	}

	int  stop_camera(camera_device_t  *pdev)
	{
		int  ops_status = NO_ERROR;

		if(pdev) {
			CMR_LOGW("pdev->common.close \n");
			ops_status = pdev->common.close((hw_device_t*)pdev);
			mtrace_print_alllog(TRACE_MEMSTAT);
		}
		return ops_status;
	}
}

static inline SprdCameraHardware *obj(struct camera_device *dev)
{
	return reinterpret_cast<SprdCameraHardware *>(dev->priv);
}

int  main(int  argc,char **  arg)
{
	int  ops_status  = NO_ERROR;
	camera_device_t  *pdev = NULL;
	SprdCameraHardware *pobj;

	CMR_LOGI("enter  sprdcamtest program");
	process_protect();

	if(_parse_options(argc,arg))
		return  -1;

	ops_status = start_camera(&pdev);
	if(!pdev){
		CMR_LOGE("\n open %d fail ",opts.cameraId);
		return -1;
	} else {
		mergepara(pdev,opts.param[0]);
		sem_init(&takepic_action.capture_sem, 0, 0);
		takepic_action.pic_cnt = 0;
//		stop_camera(pdev);

	//	ops_status = pdev->ops->start_preview(pdev);
	//	write_otp_sn(0, 0);
	}
	while(opts.during == -1){sleep(100);};
	switch(opts.workmode)
	{
		case WK_PREVIEW:
		CMR_LOGI("\n WK_PREVIEW during %d s=>",opts.during);
		ops_status = pdev->ops->start_preview(pdev);
		if(opts.during != -1){
			sleep(opts.during);
		} else {
			CMR_LOGI("\n opts.during = -1\n");
		}
		for(int i=1; i<opts.param_cnt ;i++)
		{
			CMR_LOGI("\n WK_PREVIEW PRARM %d", i);
			mergepara(pdev,opts.param[i]);
			sleep(opts.during);
		}
		CMR_LOGI("\n WK_PREVIEW during %d s  stopping",opts.during);
		pdev->ops->stop_preview(pdev);
		break;

		case WK_RECVIDEO:
		CMR_LOGI("\n WK_RECVIDEO during %d s=>",opts.during);
		ops_status = pdev->ops->start_preview(pdev);
		sleep(1);
		CMR_LOGI("\n WK_RECVIDEO during %d s=>",opts.during);
		pdev->ops->enable_msg_type(pdev, CAMERA_MSG_VIDEO_FRAME);

		ops_status = pdev->ops->start_recording(pdev);
		sleep(opts.during);
		pdev->ops->stop_recording(pdev);
		CMR_LOGI("\n WK_RECVIDEO during %d s  stopping",opts.during);
		sleep(1);
		break;

		case WK_TAKEPIC:
		CMR_LOGI("\n WK_TAKEPIC during %d s=>",opts.during);
		ops_status = pdev->ops->start_preview(pdev);
		sleep(5);
		pdev->ops->enable_msg_type(pdev,CAMERA_MSG_RAW_IMAGE|CAMERA_MSG_COMPRESSED_IMAGE
			|CAMERA_MSG_RAW_IMAGE_NOTIFY|CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_ERROR);
		for(int j=0;j<opts.during;j++){
			struct timespec 		 ts;

			CMR_LOGI("\n WK_TAKEPIC %d =>",j);
			ops_status = pdev->ops->take_picture(pdev);
			CMR_LOGI("\n WK_TAKEPIC %d,ret=%d",j,ops_status);
			//pHwobj.cancelPicture();
			//sleep(opts.during);

			/* Posix mandates CLOCK_REALTIME here */
			clock_gettime( CLOCK_REALTIME, &ts );

			/* Check it as per Posix */
			if (ts.tv_sec < 0 ||
				ts.tv_nsec < 0 ||
				ts.tv_nsec >= 1000000000) {
				return -1;
			}
			/*wait for 200ms*/
			ts.tv_sec += opts.during;
			sem_timedwait(&takepic_action.capture_sem, &ts);
			break;
		}
		break;

		case WK_TAKEPIC_RAW:
		CMR_LOGI("\n WK_TAKEPIC_RAW during %d s=>",opts.during);
		ops_status = pdev->ops->start_preview(pdev);
		sleep(5);
		pobj = obj(pdev);
		if(!pobj) {
			CMR_LOGE("get SprdCameraHardware fail!");
			break;
		}

		pobj->setCaptureRawMode(1);
		pdev->ops->enable_msg_type(pdev,CAMERA_MSG_RAW_IMAGE|CAMERA_MSG_COMPRESSED_IMAGE
			|CAMERA_MSG_RAW_IMAGE_NOTIFY|CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_ERROR);
		for(int j=0;j<opts.during;j++){
			CMR_LOGI("\nWK_TAKEPIC_RAW %d =>",j);
			status_t ret = pdev->ops->take_picture(pdev);
			CMR_LOGI("\nWK_TAKEPIC_RAW %d,ret=%d",j,ret);
			//pHwobj.cancelPicture();
			sleep(opts.during);
			break;
		}
		break;

		case WK_AUTOFOCUS:
		sleep(1);
		CMR_LOGI("\n WK_AUTOFOCUS during %d s=>",opts.during);
		pdev->ops->enable_msg_type(pdev, CAMERA_MSG_ALL_MSGS);
		pdev->ops->auto_focus(pdev);
		sleep(opts.during);
		pdev->ops->cancel_auto_focus(pdev);
		CMR_LOGI("\n WK_AUTOFOCUS during %d s=>",opts.during);
		sleep(opts.during);
		break;

	}
	CMR_LOGI("common.close now");

	pdev->common.close((hw_device_t*)pdev);

//	exit(0);
	CMR_LOGI("exit	sprdcamtest program");
	return ops_status;
}

