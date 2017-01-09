

#define LOG_TAG   __FILE__

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <semaphore.h>

#include <utils/Log.h>
#include <utils/String8.h>
#include <camera/CameraParameters.h>
#include <camera/CameraMetadata.h>

#include <hardware/camera3.h>
#include <MemoryHeapIon.h>
#include <gralloc_priv.h>
#include "ion_sprd.h"

#include "cmr_common.h"
#include "isp_otp.h"


using namespace android;

#define  MAX_ARGS    256
struct _options {
	char*  cameraId ;
	int workmode;
	int during;
	String8  param[MAX_ARGS];
	int    param_cnt;
	int    pic_cnt;

	camera3_device_t  *pdev  ;
	CameraParameters   cp;

	camera3_stream_configuration_t sc;
	camera3_stream_t   cs;
	camera3_stream_buffer_t sb;
	buffer_handle_t  buffer;
	camera3_capture_request_t request;
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
#include <ucontext.h>

#include <setjmp.h>
//#include <execinfo.h>	/* for backtrace() */

#define SIZE	100

static void dump(void)
{
#if 0
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
static int     err;

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
	sigaction(SIGFPE,&action1,&action2);
//#else
	if(sigaction(SIGFPE,&action1,&action2) == (int)SIG_ERR )	{
		CMR_LOGE("sigaction fail! " );
		abort();
	}
	action1.sa_handler = msg;
	if(sigaction(SIGSEGV,&action1,&action2) == (int)SIG_ERR )	{
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
CameraParameters mergepara(camera3_device_t  *pdev, String8 &argstr)
{
//	char * tmp = pdev->ops->get_parameters(pdev);
	CameraParameters def ;//= CameraParameters(String8(tmp));
	char *a;
	char *b;

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

	if(opts.workmode == WK_TAKEPIC) {
		opts.pic_cnt = def.getInt("capture-mode");
		CMR_LOGW("WK_TAKEPIC:opts.pic_cnt = %d\n",opts.pic_cnt);
	}
	return def;
}


typedef void (*camera_release_memory)(struct camera_memory *mem);

typedef struct camera_memory {
    void *data;
    size_t size;
    void *handle;
    camera_release_memory release;
} camera_memory_t;


static void _release_memory(struct camera_memory *mem)
{
	if(mem) {
		MemoryHeapBase	*mHeap = (MemoryHeapBase  *)mem->handle ;
		if(mHeap)
			delete mHeap;
		free(mem);
	}
}

camera_memory_t* _get_memory(private_handle_t *pBuf)
{
	camera_memory_t*  p  =  (camera_memory_t*) malloc(sizeof(camera_memory_t));
	MemoryHeapIon  *mHeap = new MemoryHeapIon("/dev/ion", pBuf->size, 0 , (1<<31) | ION_HEAP_ID_MASK_SYSTEM);

	if(mHeap&& p)
	{
		p->data = mHeap->getBase();
		p->handle = mHeap;
		p ->size = pBuf->size;
		p->release= _release_memory;
		pBuf ->fd   = mHeap->getHeapID();
		pBuf ->base = (uint64_t)mHeap->getBase();
		return p;
	}
	else
	{
		CMR_LOGI("get_memory fail !",p);
		return NULL;
	}
}



static struct {
	sem_t capture_sem;
	int  pic_cnt;
} takepic_action = {0};


extern "C" {
extern	struct camera_module HAL_MODULE_INFO_SYM ;
//extern int write_otp_sn(unsigned char *data_buf, unsigned int data_size);
}

	int  start_camera(void  **pdev)
	{
		int  ops_status=NO_ERROR;
		camera3_device_t  *tmp=NULL;

		if(opts.pdev == NULL) {
			CMR_LOGW("pdev->common.methods->open \n");
			ops_status = HAL_MODULE_INFO_SYM.common.methods->open(&HAL_MODULE_INFO_SYM.common,opts.cameraId,(hw_device_t**)&tmp);
			if(ops_status!=0){
				CMR_LOGE("\n open %d fail ",opts.cameraId);
				if(tmp)
					tmp->common.close((hw_device_t*)tmp);  //avoid next time retry fail
				*pdev = NULL;
				return -1;
			}
			*pdev = opts.pdev = tmp;
		} else {
			*pdev = opts.pdev;
		}
		return ops_status;
	}

	int  stop_camera(void  *pdev)
	{
		int  ops_status=NO_ERROR;
		camera3_device_t  *tmp = (camera3_device_t  *)pdev;

		if(opts.pdev){
			CMR_LOGW("pdev->common.close \n");
			ops_status = tmp->common.close((hw_device_t*)pdev);
			opts.pdev = NULL;
			mtrace_print_alllog(TRACE_MEMSTAT);
			return ops_status;
		}
	}

static void process_capture_result(const struct camera3_callback_ops *,
		const camera3_capture_result_t *result)
{
	CMR_LOGI("E:\n");
#if 0
	int  ops_status=NO_ERROR;

	camera3_device_t  *pdev 			= opts.pdev;
	camera3_capture_request_t request	= opts.request;

	request.frame_number	= result->frame_number + 1;

	ops_status = pdev->ops->process_capture_request(pdev,&request);
	if(ops_status){
		CMR_LOGE("fail");
	}
#endif
}

static void notify(const struct camera3_callback_ops *,
		const camera3_notify_msg_t *msg)
{
	CMR_LOGI("msg->type 0x%x\n", msg->type);
	if(msg->type == CAMERA3_MSG_SHUTTER) {
		CMR_LOGI("shutter:frame_number = 0x%x , timestamp = 0x%x\n",
				msg->message.shutter.frame_number,
				msg->message.shutter.timestamp);
	}
}


static camera3_callback_ops_t camera3ops = {
	process_capture_result : process_capture_result,
	notify : notify,
};

void start_preview(camera3_device_t  *pdev, CameraParameters * para)
{
	int  ops_status=NO_ERROR;
	camera3_stream_configuration_t sc;
	CameraMetadata  Metadata(5);
	camera_metadata_t *md;
	camera3_stream_t   cs;
	camera3_stream_buffer_t sb;
	camera3_capture_request_t request;
	camera_memory_t  *cm;

	CMR_LOGI("start_preview before real action");

	CMR_LOGE("camera3_dev configure_streams");
	sc.num_streams 	= 1;
	sc.streams = (camera3_stream_t **) malloc(sizeof(camera3_stream_t *)*sc.num_streams);
	if(!sc.streams){
		CMR_LOGE("malloc streams fail ");
		return;
	}
	sc.streams[0] 	= &cs;
	cs.stream_type = CAMERA3_STREAM_OUTPUT ;
	para->getPreviewSize((int *)&cs.width,(int *)&cs.height);
	cs.format 	= HAL_PIXEL_FORMAT_YCrCb_420_SP ;
	cs.usage 	= 0;
    ops_status = pdev->ops->configure_streams(pdev, &sc);
	if(ops_status){
		CMR_LOGE("fail");
	}

	CMR_LOGE("camera3_dev construct_default_request_settings");
    md = (camera_metadata_t* )pdev->ops->construct_default_request_settings(pdev, CAMERA3_TEMPLATE_PREVIEW);
	if(!md){
		CMR_LOGE("fail");
	}

	CMR_LOGE("camera3_dev process_capture_request");
	Metadata.append(md);
	md = Metadata.release();

	request.frame_number		=0x80000000;
	request.settings 			= md;
	request.input_buffer 		= NULL;
	request.num_output_buffers 	= 1;

	private_handle_t prv(-1,(cs.width*cs.height)*2,0);
	buffer_handle_t  buffer = &prv;
	sb.stream = &cs;
	sb.status = 0;
	sb.buffer = (buffer_handle_t *)&buffer;
	sb.acquire_fence = -1;
	sb.release_fence = -1;
	request.output_buffers = (const camera3_stream_buffer_t *)&sb;
	cm  = _get_memory(&prv);

	ops_status = pdev->ops->process_capture_request(pdev,&request);
	if(ops_status){
		CMR_LOGE("fail");
	}
	sleep(5);

	_release_memory(cm);
	free(sc.streams);
}
int  main(int  argc,char **  arg)
{
	int  ops_status = NO_ERROR;
	camera3_device_t  *pdev = NULL;
	CameraParameters cp;
	camera3_stream_configuration_t sc;
	camera3_stream_t   cs;
	CameraMetadata  Metadata(5);
	camera_metadata_t *md;
	uint8_t intent = 0;
	camera3_stream_buffer_t sb;
	camera3_capture_request_t request;
	camera3_stream_buffer stream_buffer ;
	camera_memory_t  *cm;
//	struct private_handle_t handler;

	CMR_LOGI("enter  sprdcamtest program");
//	process_protect();

	if(_parse_options(argc,arg))
		return  -1;

	CMR_LOGI("module common.methods->open");
	start_camera((void **)&pdev);
	CMR_LOGI("module set_callbacks");
	HAL_MODULE_INFO_SYM.set_callbacks(NULL);
	cp = mergepara(pdev,opts.param[0]);
	sem_init(&takepic_action.capture_sem, 0, 0);
	takepic_action.pic_cnt = 0;

	CMR_LOGE("camera3_dev initialize");
	ops_status = pdev->ops->initialize(pdev,&camera3ops);
	if(ops_status){
		CMR_LOGE("fail");
	}

	while(opts.during == -1){sleep(100);};
	switch(opts.workmode)
	{
		case WK_PREVIEW:
		{
			CMR_LOGI("\n WK_PREVIEW during %d s=>",opts.during);

			CMR_LOGE("camera3_dev configure_streams");
			sc.num_streams 	= 1;
			sc.streams = (camera3_stream_t **) malloc(sizeof(camera3_stream_t *)*sc.num_streams);
			if(!sc.streams){
				CMR_LOGE("malloc streams fail ");
				break;
			}
			sc.streams[0] 	= &cs;
			cs.stream_type = CAMERA3_STREAM_OUTPUT ;
			cp.getPreviewSize((int *)&cs.width,(int *)&cs.height);
			cs.format 	= HAL_PIXEL_FORMAT_YCrCb_420_SP ;
			cs.usage 	= 0;
		    ops_status = pdev->ops->configure_streams(pdev, &sc);
			if(ops_status){
				CMR_LOGE("fail");
			}

			CMR_LOGE("camera3_dev construct_default_request_settings");
		    md = (camera_metadata_t* )pdev->ops->construct_default_request_settings(pdev, CAMERA3_TEMPLATE_PREVIEW);
			if(!md){
				CMR_LOGE("fail");
			}

			CMR_LOGE("camera3_dev process_capture_request");
			Metadata.append(md);
		    intent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
			Metadata.update(ANDROID_CONTROL_CAPTURE_INTENT,&intent,1);
			md = Metadata.release();

			request.frame_number		=0x80000000;
			request.settings 			= md;
			request.input_buffer 		= NULL;
			request.num_output_buffers 	= 1;

			private_handle_t prv(-1,(cs.width*cs.height)*2,0);
			buffer_handle_t  buffer = &prv;
			sb.stream = &cs;
			sb.status = 0;
			sb.buffer = (buffer_handle_t *)&buffer;
			sb.acquire_fence = -1;
			sb.release_fence = -1;
			request.output_buffers = (const camera3_stream_buffer_t *)&sb;
			cm  = _get_memory(&prv);

			opts.cp = cp;
			opts.request = request;
			ops_status = pdev->ops->process_capture_request(pdev,&request);
			if(ops_status){
				CMR_LOGE("fail");
			}
			if(opts.during != -1){
				sleep(opts.during);
			} else {
				CMR_LOGI("\n opts.during = -1\n");
			}
			for(int i=1; i<opts.param_cnt ;i++)
			{
				CMR_LOGI("\n WK_PREVIEW PRARM %d", i);
				cp = mergepara(pdev,opts.param[i]);
				ops_status = pdev->ops->process_capture_request(pdev,&request);
				sleep(opts.during);
			}
			CMR_LOGI("\n WK_PREVIEW during %d s  stopping",opts.during);
	//		pdev->ops->stop_preview(pdev);
			_release_memory(cm);
			free(sc.streams);
		}
		break;

		case WK_RECVIDEO:
//		ops_status = pdev->ops->start_preview(pdev);
		sleep(1);
		CMR_LOGI("\n WK_RECVIDEO during %d s=>",opts.during);
//		pdev->ops->enable_msg_type(pdev, CAMERA_MSG_VIDEO_FRAME);

//		ops_status = pdev->ops->start_recording(pdev);
		sleep(opts.during);
//		pdev->ops->stop_recording(pdev);
		CMR_LOGI("\n WK_RECVIDEO during %d s  stopping",opts.during);
		sleep(1);
		break;

		case WK_TAKEPIC:
		{
			start_preview(pdev ,&cp);

			CMR_LOGE("camera3_dev configure_streams");
			sc.num_streams 	= 1;
			sc.streams = (camera3_stream_t **) malloc(sizeof(camera3_stream_t *)*sc.num_streams);
			if(!sc.streams){
				CMR_LOGE("malloc streams fail ");
				break;
			}
			sc.streams[0] 	= &cs;
			cs.stream_type = CAMERA3_STREAM_OUTPUT ;
			cp.getPictureSize((int *)&cs.width,(int *)&cs.height);
			cs.format	= HAL_PIXEL_FORMAT_BLOB ;
			cs.usage 	= 0;
			ops_status = pdev->ops->configure_streams(pdev, &sc);
			if(ops_status){
				CMR_LOGE("fail");
			}

			CMR_LOGE("camera3_dev construct_default_request_settings");
			md = (camera_metadata_t* )pdev->ops->construct_default_request_settings(pdev, CAMERA3_TEMPLATE_STILL_CAPTURE);
			if(!md){
				CMR_LOGE("fail");
			}

			CMR_LOGE("camera3_dev process_capture_request");
		    intent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
			Metadata.update(ANDROID_CONTROL_CAPTURE_INTENT,&intent,1);
			md = Metadata.release();

			request.frame_number		=0x80000000;
			request.settings 			= md;
			request.input_buffer 		= NULL;
			request.num_output_buffers 	= 1;

			private_handle_t prv(-1,(cs.width*cs.height)*2,0);
			buffer_handle_t  buffer = &prv;
			sb.stream = &cs;
			sb.status = 0;
			sb.buffer = (buffer_handle_t *)&buffer;
			sb.acquire_fence = -1;
			sb.release_fence = -1;
			request.output_buffers = (const camera3_stream_buffer_t *)&sb;
			cm  = _get_memory(&prv);

			ops_status = pdev->ops->process_capture_request(pdev,&request);
			if(ops_status){
				CMR_LOGE("fail");
			}
			if(opts.during != -1){
				sleep(opts.during);
			} else {
				CMR_LOGI("\n opts.during = -1\n");
			}
			CMR_LOGI("\n WK_TAKEPICTURE during %d s  stopping",opts.during);

			_release_memory(cm);
			free(sc.streams);
		}
		break;

		case WK_TAKEPIC_RAW:
		sleep(5);

//		pdev->ops->enable_msg_type(pdev,CAMERA_MSG_RAW_IMAGE|CAMERA_MSG_COMPRESSED_IMAGE
//			|CAMERA_MSG_RAW_IMAGE_NOTIFY|CAMERA_MSG_COMPRESSED_IMAGE|CAMERA_MSG_ERROR);
		for(int j=0;j<opts.during;j++){
			CMR_LOGI("\nWK_TAKEPIC_RAW %d =>",j);
//			status_t ret = pdev->ops->take_picture(pdev);
//			CMR_LOGI("\nWK_TAKEPIC_RAW %d,ret=%d",j,ret);
			//pHwobj.cancelPicture();
			sleep(opts.during);
			break;
		}
		break;

		case WK_AUTOFOCUS:
		sleep(1);
		CMR_LOGI("\n WK_AUTOFOCUS during %d s=>",opts.during);
//		pdev->ops->enable_msg_type(pdev, CAMERA_MSG_ALL_MSGS);
//		pdev->ops->auto_focus(pdev);
		sleep(opts.during);
//		pdev->ops->cancel_auto_focus(pdev);
		CMR_LOGI("\n WK_AUTOFOCUS during %d s=>",opts.during);
		sleep(opts.during);
		break;

	}
	CMR_LOGI("pdev common.close");
	stop_camera((void *)pdev);

//	exit(0);
	CMR_LOGI("exit	sprdcamtest program");
	return ops_status;
}

