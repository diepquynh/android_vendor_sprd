/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <sys/stat.h>
#include <time.h>
#include <asm/byteorder.h>
#include <math.h>

#define PCM_NAME                     "hw:sprdphone,0"              /* default device */
#define CARD_NAME                    "hw:2"                        /* default card */
#define FRAME_PER_PERIOD             32                            /* frames in one period */
#define HEADSET_STATE_PATH           "/sys/class/switch/h2w/state" /* headset state */

/*Used for capture(record) test*/
#define DEFAULT_RECORD_CHANNELS      1                     /*channels for capture*/
#define DEFAULT_RECORD_FORMAT        SND_PCM_FORMAT_S16_LE /*format for capture*/
#define DEFAULT_RECORD_RATE          44100                 /*sample rate for capture*/
#define DEFAULT_RECORD_FILE_NAME     "record.wav"          /* file in which capture data is saved*/
#define DEFAULT_RECORD_TIME          10                    /*unit:second*/

#define ERR(x...)	fprintf(stderr, ##x)
#define INFO(x...)	fprintf(stdout, ##x)

#undef __swab16
#define __swab16(x)  __arch__swab16((x))
#undef __swab32
#define __swab32(x)  __arch__swab32((x))

#define LEVEL_BASIC		(1<<0)
#define LEVEL_INACTIVE		(1<<1)
#define LEVEL_ID		(1<<2)

#undef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL

#define wav_check_file_space(buffer, len, blimit) \
	if (len > blimit) { \
		blimit = len; \
		if ((buffer = realloc(buffer, blimit)) == NULL) { \
			ERR("not enough memory");		  \
			return -1;  \
		} \
	}

/* Function to convert from percentage to volume. val = percentage */
#define amixer_convert_prange1(val, min, max) \
		ceil((val) * ((max) - (min)) * 0.01 + (min))
#define amixer_check_range(val, min, max) \
			((val < min) ? (min) : (val > max) ? (max) : (val))

/* Definitions for Microsoft WAVE format */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)		(v)
#define LE_INT(v)		(v)
#define BE_SHORT(v)		bswap_16(v)
#define BE_INT(v)		bswap_32(v)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)		bswap_16(v)
#define LE_INT(v)		bswap_32(v)
#define BE_SHORT(v)		(v)
#define BE_INT(v)		(v)
#else
#error "Wrong endian"
#endif

#define WAV_RIFF		COMPOSE_ID('R','I','F','F')
#define WAV_WAVE		COMPOSE_ID('W','A','V','E')
#define WAV_FMT			COMPOSE_ID('f','m','t',' ')
#define WAV_DATA		COMPOSE_ID('d','a','t','a')
/* Used with WAV_FMT_EXTENSIBLE format */
#define WAV_GUID_TAG		"\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71"


/* WAVE fmt block constants from Microsoft mmreg.h header */
#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003
#define WAV_FMT_DOLBY_AC3_SPDIF 0x0092
#define WAV_FMT_EXTENSIBLE      0xfffe

enum {
	VUMETER_NONE,
	VUMETER_MONO,
	VUMETER_STEREO
};
typedef struct {
	u_int magic;		/* 'RIFF' */
	u_int length;		/* filelen */
	u_int type;		/* 'WAVE' */
} WaveHeader;

typedef struct {
	u_short format;		/* see WAV_FMT_* */
	u_short channels;
	u_int sample_fq;	/* frequence of sample */
	u_int byte_p_sec;
	u_short byte_p_spl;	/* samplesize; 1 or 2 bytes */
	u_short bit_p_spl;	/* 8, 12 or 16 bit */
} WaveFmtBody;

typedef struct {
	WaveFmtBody format;
	u_short ext_size;
	u_short bit_p_spl;
	u_int channel_mask;
	u_short guid_format;	/* WAV_FMT_* */
	u_char guid_tag[14];	/* WAV_GUID_TAG */
} WaveFmtExtensibleBody;

typedef struct {
	u_int type;		/* 'data' */
	u_int length;		/* samplecount */
} WaveChunkHeader;

static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams;

/*following variables can be given in command */
static const char*       file_name       = NULL;      /*specify a wav file*/
static bool              is_mix_list     = false;     /*list all mix ctrl path*/
static int               ctrl_num        = 0;         /*specify the ctrl number to be set*/
static char*             ctrl_val        = NULL;      /*specify the value to set to a ctrl path*/
static int               test_loop       = 1;         /*specify the testing loop count*/
static int               record_sec      = DEFAULT_RECORD_TIME;  /*specify the length of record sound*/

static char*             card_name       = CARD_NAME; /*specify card nme*/
static int               record_channels = DEFAULT_RECORD_CHANNELS;
static int               record_rate     = DEFAULT_RECORD_RATE;
static snd_pcm_stream_t  pcm_stream      = SND_PCM_STREAM_PLAYBACK;
static snd_pcm_t*        pcm_handle      = NULL;
static int               timelimit       = 0;
static u_char*           audiobuf        = NULL;
static snd_pcm_uframes_t chunk_size      = 0;
static unsigned          period_time     = 0;
static int               monotonic       = 0;
static int               vumeter         = VUMETER_STEREO;
static size_t            bits_per_sample = 0;
static size_t            bits_per_frame  = 0;
static size_t            chunk_bytes     = 0;
static off64_t           pbrec_count     = LLONG_MAX;
static off64_t           fdcount         = 0;

static void usage(void)
{
	INFO("Usage:\n");
	INFO("  utest_audio mix [-l|-c ctrl -v value]\n");
	INFO("  utest_audio play -n count -f file.wav\n");
	INFO("  utest_audio capture -s seconds -f file.wav\n");
	INFO("  utest_audio headset -n count\n");
	INFO("\nmix\n");
	INFO("  display or set the mix control\n");
	INFO("play\n");
	INFO("  play a sound file\n");
	INFO("capture\n");
	INFO("  capture sound into a file\n");
	INFO("headset\n");
	INFO("  do headset hotplug detect and show the result\n");
	INFO("\n-l\n");
	INFO("  list all mix ctrl path\n");
	INFO("-c ctrl\n");
	INFO("  specify the ctrl number to be set. it's used to change the audio path.\n");
	INFO("-v value\n");
	INFO("  specify the value to set to a ctrl path\n");
	INFO("-n count\n");
	INFO("  specify the testing loop count\n");
	INFO("-s seconds\n");
	INFO("  specify the length of record sound\n");
	INFO("-f file.wav\n");
	INFO("  specify a wav file\n");
}

void process_options(int argc, char **argv)
{
	int opt = 0;
	while ((opt = getopt (argc, argv, "lc:v:n:f:s:")) != -1) {
		switch (opt) {
		case 'l':
			is_mix_list = true;
			break;
		case 'c':
			ctrl_num = atoi(optarg);
			break;
		case 'v':
			if (!(ctrl_val = strdup(optarg))){
				ERR("error ");
			}
			break;
		case 'n':
			test_loop = atoi(optarg);
			break;
		case 'f':
			if (!(file_name = strdup(optarg))){
				ERR("error ");
			}
			break;
		case 's':
			record_sec = atoi(optarg);
			break;
		default:
			break;
		}
	}
}

static void amixer_show_control_id(snd_ctl_elem_id_t *id)
{
	unsigned int index = 0;
	unsigned int device = 0;
	unsigned int subdevice = 0;

	INFO("numid=%u,iface=%s,name='%s'",
		snd_ctl_elem_id_get_numid(id),
		snd_ctl_elem_iface_name(snd_ctl_elem_id_get_interface(id)),
		snd_ctl_elem_id_get_name(id));
	index = snd_ctl_elem_id_get_index(id);
	device = snd_ctl_elem_id_get_device(id);
	subdevice = snd_ctl_elem_id_get_subdevice(id);

	if (index)
		INFO(",index=%i", index);
	if (device)
		INFO(",device=%i", device);
	if (subdevice)
		INFO(",subdevice=%i", subdevice);
}
static void amixer_print_spaces(unsigned int spaces)
{
	while (spaces-- > 0)
		putc(' ', stdout);
}

static void amixer_print_dB(long dB)
{
	INFO("%li.%02lidB", dB / 100, (dB < 0 ? -dB : dB) % 100);
}

static void amixer_decode_tlv(unsigned int spaces, unsigned int *tlv, unsigned int tlv_size)
{
	unsigned int type = tlv[0];
	unsigned int size;
	unsigned int idx = 0;

	if (tlv_size < 2 * sizeof(unsigned int)) {
		INFO("TLV size error!\n");
		return;
	}
	amixer_print_spaces(spaces);
	INFO("  ");
	type = tlv[idx++];
	size = tlv[idx++];
	tlv_size -= 2 * sizeof(unsigned int);
	if (size > tlv_size) {
		INFO("TLV size error (%i, %i, %i)!\n", type, size, tlv_size);
		return;
	}
	switch (type) {
	case SND_CTL_TLVT_CONTAINER:
		size += sizeof(unsigned int) -1;
		size /= sizeof(unsigned int);
		while (idx < size) {
			if (tlv[idx+1] > (size - idx) * sizeof(unsigned int)) {
				INFO("TLV size error in compound!\n");
				return;
			}
			amixer_decode_tlv(spaces + 2, tlv + idx, tlv[idx+1]);
			idx += 2 + (tlv[1] + sizeof(unsigned int) - 1) / sizeof(unsigned int);
		}
		break;
	case SND_CTL_TLVT_DB_SCALE:
		INFO("dBscale-");
		if (size != 2 * sizeof(unsigned int)) {
			while (size > 0) {
				INFO("0x%08x,", tlv[idx++]);
				size -= sizeof(unsigned int);
			}
		} else {
			INFO("min=");
			amixer_print_dB((int)tlv[2]);
			INFO(",step=");
			amixer_print_dB(tlv[3] & 0xffff);
			INFO(",mute=%i", (tlv[3] >> 16) & 1);
		}
		break;
#ifdef SND_CTL_TLVT_DB_LINEAR
	case SND_CTL_TLVT_DB_LINEAR:
		INFO("dBlinear-");
		if (size != 2 * sizeof(unsigned int)) {
			while (size > 0) {
				INFO("0x%08x,", tlv[idx++]);
				size -= sizeof(unsigned int);
			}
		} else {
			INFO("min=");
			amixer_print_dB(tlv[2]);
			INFO(",max=");
			amixer_print_dB(tlv[3]);
		}
		break;
#endif
#ifdef SND_CTL_TLVT_DB_RANGE
	case SND_CTL_TLVT_DB_RANGE:
		INFO("dBrange-\n");
		if ((size / (6 * sizeof(unsigned int))) != 0) {
			while (size > 0) {
				INFO("0x%08x,", tlv[idx++]);
				size -= sizeof(unsigned int);
			}
			break;
		}
		idx = 0;
		while (idx < size) {
			amixer_print_spaces(spaces + 2);
			INFO("rangemin=%i,", tlv[0]);
			INFO(",rangemax=%i\n", tlv[1]);
			amixer_decode_tlv(spaces + 4, tlv + 2, 6 * sizeof(unsigned int));
			idx += 6 * sizeof(unsigned int);
		}
		break;
#endif
#ifdef SND_CTL_TLVT_DB_MINMAX
	case SND_CTL_TLVT_DB_MINMAX:
	case SND_CTL_TLVT_DB_MINMAX_MUTE:
		if (type == SND_CTL_TLVT_DB_MINMAX_MUTE)
			INFO("dBminmaxmute-");
		else
			INFO("dBminmax-");
		if (size != 2 * sizeof(unsigned int)) {
			while (size > 0) {
				INFO("0x%08x,", tlv[idx++]);
				size -= sizeof(unsigned int);
			}
		} else {
			INFO("min=");
			amixer_print_dB(tlv[2]);
			INFO(",max=");
			amixer_print_dB(tlv[3]);
		}
		break;
#endif
	default:
		INFO("unk-%i-", type);
		while (size > 0) {
			INFO("0x%08x,", tlv[idx++]);
			size -= sizeof(unsigned int);
		}
		break;
	}
	putc('\n', stdout);
}


static const char *amixer_control_access(snd_ctl_elem_info_t *info)
{
	static char result[10];
	char *res = result;

	*res++ = snd_ctl_elem_info_is_readable(info) ? 'r' : '-';
	*res++ = snd_ctl_elem_info_is_writable(info) ? 'w' : '-';
	*res++ = snd_ctl_elem_info_is_inactive(info) ? 'i' : '-';
	*res++ = snd_ctl_elem_info_is_volatile(info) ? 'v' : '-';
	*res++ = snd_ctl_elem_info_is_locked(info) ? 'l' : '-';
	*res++ = snd_ctl_elem_info_is_tlv_readable(info) ? 'R' : '-';
	*res++ = snd_ctl_elem_info_is_tlv_writable(info) ? 'W' : '-';
	*res++ = snd_ctl_elem_info_is_tlv_commandable(info) ? 'C' : '-';
	*res++ = '\0';
	return result;
}

static int amixer_show_control(const char *space, snd_hctl_elem_t *elem, int level)
{
	int err;
	unsigned int item, idx, count, *tlv;
	snd_ctl_elem_type_t type;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_value_t *control;
	snd_aes_iec958_t iec958;

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_value_alloca(&control);

	if ((err = snd_hctl_elem_info(elem, info)) < 0) {
		ERR("Control %s snd_hctl_elem_info error: %s\n", card_name, snd_strerror(err));
		return err;
	}
	if (level & LEVEL_ID) {
		snd_hctl_elem_get_id(elem, id);
		amixer_show_control_id(id);
		INFO("\n");
	}
	count = snd_ctl_elem_info_get_count(info);
	type = snd_ctl_elem_info_get_type(info);
	INFO("%s  type=%s,access=%s,values=%i", space,
		(char*)snd_ctl_elem_type_name(snd_ctl_elem_info_get_type(info)),
		(char*)amixer_control_access(info), count);
	switch (type) {
	case SND_CTL_ELEM_TYPE_INTEGER:
		INFO(",min=%li,max=%li,step=%li\n",
		       snd_ctl_elem_info_get_min(info),
		       snd_ctl_elem_info_get_max(info),
		       snd_ctl_elem_info_get_step(info));
		break;
	case SND_CTL_ELEM_TYPE_INTEGER64:
		INFO(",min=%Li,max=%Li,step=%Li\n",
		       snd_ctl_elem_info_get_min64(info),
		       snd_ctl_elem_info_get_max64(info),
		       snd_ctl_elem_info_get_step64(info));
		break;
	case SND_CTL_ELEM_TYPE_ENUMERATED:
	{
		unsigned int items = snd_ctl_elem_info_get_items(info);
		INFO(",items=%u\n", items);
		for (item = 0; item < items; item++) {
			snd_ctl_elem_info_set_item(info, item);
			if ((err = snd_hctl_elem_info(elem, info)) < 0) {
				ERR("Control %s element info error: %s\n", card_name, snd_strerror(err));
				return err;
			}
			INFO("%s  Item #%u '%s'\n", space, item, snd_ctl_elem_info_get_item_name(info));
		}
		break;
	}
	default:
		INFO("\n");
		break;
	}
	if (level & LEVEL_BASIC) {
		if (!snd_ctl_elem_info_is_readable(info))
			goto __skip_read;
		if ((err = snd_hctl_elem_read(elem, control)) < 0) {
			ERR("Control %s element read error: %s\n", card_name, snd_strerror(err));
			return err;
		}
		INFO("%s  values=", space);
		for (idx = 0; idx < count; idx++) {
			if (idx > 0)
				INFO(",");
			switch (type) {
			case SND_CTL_ELEM_TYPE_BOOLEAN:
				INFO("%s", snd_ctl_elem_value_get_boolean(control, idx) ? "on" : "off");
				break;
			case SND_CTL_ELEM_TYPE_INTEGER:
				INFO("%li", snd_ctl_elem_value_get_integer(control, idx));
				break;
			case SND_CTL_ELEM_TYPE_INTEGER64:
				INFO("%Li", snd_ctl_elem_value_get_integer64(control, idx));
				break;
			case SND_CTL_ELEM_TYPE_ENUMERATED:
				INFO("%u", snd_ctl_elem_value_get_enumerated(control, idx));
				break;
			case SND_CTL_ELEM_TYPE_BYTES:
				INFO("0x%02x", snd_ctl_elem_value_get_byte(control, idx));
				break;
			case SND_CTL_ELEM_TYPE_IEC958:
				snd_ctl_elem_value_get_iec958(control, &iec958);
				INFO("[AES0=0x%02x AES1=0x%02x AES2=0x%02x AES3=0x%02x]",
				       iec958.status[0], iec958.status[1],
				       iec958.status[2], iec958.status[3]);
				break;
			default:
				INFO("?");
				break;
			}
		}
		INFO("\n");
__skip_read:
		if (!snd_ctl_elem_info_is_tlv_readable(info))
			goto __skip_tlv;
		tlv = malloc(4096);
		if ((err = snd_hctl_elem_tlv_read(elem, tlv, 4096)) < 0) {
			ERR("Control %s element TLV read error: %s\n", card_name, snd_strerror(err));
			free(tlv);
			return err;
		}
		amixer_decode_tlv(strlen(space), tlv, 4096);
		free(tlv);
	}
__skip_tlv:
	return 0;
}

static long amixer_get_integer(char **ptr, long min, long max)
{
	long val = min;
	char *p = *ptr, *s;

	if (*p == ':')
		p++;
	if (*p == '\0' || (!isdigit(*p) && *p != '-'))
		goto out;

	s = p;
	val = strtol(s, &p, 10);
	if (*p == '.') {
		p++;
		strtol(p, &p, 10);
	}
	if (*p == '%') {
		val = (long)amixer_convert_prange1(strtod(s, NULL), min, max);
		p++;
	}
	val = amixer_check_range(val, min, max);
	if (*p == ',')
		p++;
 out:
	*ptr = p;
	return val;
}

static long amixer_get_integer64(char **ptr, long long min, long long max)
{
	long long val = min;
	char *p = *ptr, *s;

	if (*p == ':')
		p++;
	if (*p == '\0' || (!isdigit(*p) && *p != '-'))
		goto out;

	s = p;
	val = strtol(s, &p, 10);
	if (*p == '.') {
		p++;
		strtol(p, &p, 10);
	}
	if (*p == '%') {
		val = (long long)amixer_convert_prange1(strtod(s, NULL), min, max);
		p++;
	}
	val = amixer_check_range(val, min, max);
	if (*p == ',')
		p++;
 out:
	*ptr = p;
	return val;
}

static int amixer_get_ctl_enum_item_index(snd_ctl_t *handle, snd_ctl_elem_info_t *info, char **ptrp)
{
	char *ptr = *ptrp;
	int items, i, len;
	const char *name;

	items = snd_ctl_elem_info_get_items(info);
	if (items <= 0)
		return -1;

	for (i = 0; i < items; i++) {
		snd_ctl_elem_info_set_item(info, i);
		if (snd_ctl_elem_info(handle, info) < 0)
			return -1;
		name = snd_ctl_elem_info_get_item_name(info);
		len = strlen(name);
		if (! strncmp(name, ptr, len)) {
			if (! ptr[len] || ptr[len] == ',' || ptr[len] == '\n') {
				ptr += len;
				*ptrp = ptr;
				return i;
			}
		}
	}
	return -1;
}


/*  Safe read  */
static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t result = 0, res;

	while (count > 0) {
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return result > 0 ? result : res;
		count -= res;
		result += res;
		buf = (char *)buf + res;
	}
	return result;
}

/* calculate the data count to read from/to dsp */
static off64_t calc_data_count(void)
{
	off64_t count;

	if (timelimit == 0) {
		count = pbrec_count;
	} else {
		count = snd_pcm_format_size(hwparams.format, hwparams.rate * hwparams.channels);
		count *= (off64_t)timelimit;
	}

	return count < pbrec_count ? count : pbrec_count;
}
/* I/O error handler */
static void io_xrun_handler(void)
{
	snd_pcm_status_t *status;
	int res;

	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(pcm_handle, status))<0) {
		ERR("status error: %s", snd_strerror(res));
		return;
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		if (monotonic) {
			ERR("%s !!!\n", "underrun");
		} else {
			struct timeval now, diff, tstamp;
			gettimeofday(&now, 0);
			snd_pcm_status_get_trigger_tstamp(status, &tstamp);
			timersub(&now, &tstamp, &diff);
			ERR("%s!!! (at least %.3f ms long)\n",
				pcm_stream == SND_PCM_STREAM_PLAYBACK ? ("underrun") :("overrun"),
				diff.tv_sec * 1000 + diff.tv_usec / 1000.0);
		}

		if ((res = snd_pcm_prepare(pcm_handle))<0) {
			ERR("xrun: prepare error: %s", snd_strerror(res));
			return;
		}
		return;		/* ok, data should be accepted again */
	}

	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {

		if (pcm_stream == SND_PCM_STREAM_CAPTURE) {
			ERR("capture stream format change? attempting recover...\n");
			if ((res = snd_pcm_prepare(pcm_handle))<0) {
				ERR("xrun(DRAINING): prepare error: %s", snd_strerror(res));
			}
			return;
		}
	}
	ERR("read/write error, state = %s", snd_pcm_state_name(snd_pcm_status_get_state(status)));
}

/* I/O suspend handler */
static void io_suspend_handler(void)
{
	int res;

	INFO("Suspended. Trying resume. ");
	while ((res = snd_pcm_resume(pcm_handle)) == -EAGAIN)
		sleep(1);	/* wait until suspend flag is released */
	if (res < 0) {
		ERR("Failed. Restarting stream. ");
		if ((res = snd_pcm_prepare(pcm_handle)) < 0) {
			ERR("suspend: prepare error:");
			return;
		}
	}
	INFO("Done.\n");
}

/* write a WAVE-header */
static void wav_writeheader(int fd, size_t cnt)
{
	WaveHeader h;
	WaveFmtBody f;
	WaveChunkHeader cf, cd;
	int bits;
	u_int tmp;
	u_short tmp2;

	/* WAVE cannot handle greater than 32bit (signed?) int */
	if (cnt == (size_t)-2)
		cnt = 0x7fffff00;

	bits = 8;
	switch ((unsigned long) hwparams.format) {
	case SND_PCM_FORMAT_U8:
		bits = 8;
		break;
	case SND_PCM_FORMAT_S16_LE:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S32_LE:
        case SND_PCM_FORMAT_FLOAT_LE:
		bits = 32;
		break;
	case SND_PCM_FORMAT_S24_LE:
	case SND_PCM_FORMAT_S24_3LE:
		bits = 24;
		break;
	default:
		ERR("Wave doesn't support %s format...", snd_pcm_format_name(hwparams.format));
		return;
	}
	h.magic = WAV_RIFF;
	tmp = cnt + sizeof(WaveHeader) + sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + sizeof(WaveChunkHeader) - 8;
	h.length = LE_INT(tmp);
	h.type = WAV_WAVE;

	cf.type = WAV_FMT;
	cf.length = LE_INT(16);

        if (hwparams.format == SND_PCM_FORMAT_FLOAT_LE)
                f.format = LE_SHORT(WAV_FMT_IEEE_FLOAT);
        else
                f.format = LE_SHORT(WAV_FMT_PCM);
	f.channels = LE_SHORT(hwparams.channels);
	f.sample_fq = LE_INT(hwparams.rate);

	tmp2 = hwparams.channels * snd_pcm_format_physical_width(hwparams.format) / 8;
	f.byte_p_spl = LE_SHORT(tmp2);
	tmp = (u_int) tmp2 * hwparams.rate;

	f.byte_p_sec = LE_INT(tmp);
	f.bit_p_spl = LE_SHORT(bits);

	cd.type = WAV_DATA;
	cd.length = LE_INT(cnt);

	if (write(fd, &h, sizeof(WaveHeader)) != sizeof(WaveHeader) ||
	    write(fd, &cf, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader) ||
	    write(fd, &f, sizeof(WaveFmtBody)) != sizeof(WaveFmtBody) ||
	    write(fd, &cd, sizeof(WaveChunkHeader)) != sizeof(WaveChunkHeader)) {
		ERR("write error");
	}
}

static void wav_writetail(int fd)
{				/* only close output */
	WaveChunkHeader cd;
	off64_t length_seek;
	off64_t filelen;
	u_int rifflen;

	length_seek = sizeof(WaveHeader) +
		      sizeof(WaveChunkHeader) +
		      sizeof(WaveFmtBody);
	cd.type = WAV_DATA;
	cd.length = fdcount > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(fdcount);
	filelen = fdcount + 2*sizeof(WaveChunkHeader) + sizeof(WaveFmtBody) + 4;
	rifflen = filelen > 0x7fffffff ? LE_INT(0x7fffffff) : LE_INT(filelen);
	if (lseek64(fd, 4, SEEK_SET) == 4)
		write(fd, &rifflen, 4);
	if (lseek64(fd, length_seek, SEEK_SET) == length_seek)
		write(fd, &cd, sizeof(WaveChunkHeader));
	if (fd != 1)
		close(fd);
}

/* helper for wav_test_file */
static size_t wav_test_file_read(int fd, u_char *buffer, size_t *size, size_t reqsize, int line)
{
	if (*size >= reqsize)
		return *size;
	if ((size_t)safe_read(fd, buffer + *size, reqsize - *size) != reqsize - *size) {
		ERR("read error (called from line %i)", line);
		return -1;
	}
	return *size = reqsize;
}

/* test, if it's a .WAV file, > 0 if ok (and set the speed, stereo etc.)  == 0 if not
 * Value returned is bytes to be discarded. */
static ssize_t wav_test_file(int fd, u_char *_buffer, size_t size)
{
	WaveHeader *h = (WaveHeader *)_buffer;
	u_char *buffer = NULL;
	size_t blimit = 0;
	WaveFmtBody *f;
	WaveChunkHeader *c;
	u_int type, len;
	if (size < sizeof(WaveHeader))
		return -1;
	if (h->magic != WAV_RIFF || h->type != WAV_WAVE)
		return -1;
	if (size > sizeof(WaveHeader)) {
		wav_check_file_space(buffer, size - sizeof(WaveHeader), blimit);
		memcpy(buffer, _buffer + sizeof(WaveHeader), size - sizeof(WaveHeader));
	}
	size -= sizeof(WaveHeader);
	while (1) {
		wav_check_file_space(buffer, sizeof(WaveChunkHeader), blimit);
		wav_test_file_read(fd, buffer, &size, sizeof(WaveChunkHeader), __LINE__);
		c = (WaveChunkHeader*)buffer;
		type = c->type;
		len = LE_INT(c->length);
		len += len % 2;
		if (size > sizeof(WaveChunkHeader))
			memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
		size -= sizeof(WaveChunkHeader);
		if (type == WAV_FMT)
			break;
		wav_check_file_space(buffer, len, blimit);
		wav_test_file_read(fd, buffer, &size, len, __LINE__);
		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}

	if (len < sizeof(WaveFmtBody)) {
		ERR("unknown length of 'fmt ' chunk (read %u, should be %u at least)",
		      len, (u_int)sizeof(WaveFmtBody));
		return -1;
	}
	wav_check_file_space(buffer, len, blimit);
	wav_test_file_read(fd, buffer, &size, len, __LINE__);
	f = (WaveFmtBody*) buffer;
	if (LE_SHORT(f->format) == WAV_FMT_EXTENSIBLE) {
		WaveFmtExtensibleBody *fe = (WaveFmtExtensibleBody*)buffer;
		if (len < sizeof(WaveFmtExtensibleBody)) {
			ERR("unknown length of extensible 'fmt ' chunk (read %u, should be %u at least)",
					len, (u_int)sizeof(WaveFmtExtensibleBody));
			return -1;
		}
		if (memcmp(fe->guid_tag, WAV_GUID_TAG, 14) != 0) {
			ERR("wrong format tag in extensible 'fmt ' chunk");
			return -1;
		}
		f->format = fe->guid_format;
	}
        if (LE_SHORT(f->format) != WAV_FMT_PCM &&
            LE_SHORT(f->format) != WAV_FMT_IEEE_FLOAT) {
                ERR("can't play WAVE-file format 0x%04x which is not PCM or FLOAT encoded", LE_SHORT(f->format));
		return -1;
	}
	if (LE_SHORT(f->channels) < 1) {
		ERR("can't play WAVE-files with %d tracks", LE_SHORT(f->channels));
		return -1;
	}
	hwparams.channels = LE_SHORT(f->channels);
	switch (LE_SHORT(f->bit_p_spl)) {
	case 8:
		hwparams.format = SND_PCM_FORMAT_U8;
		break;
	case 16:
		hwparams.format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		switch (LE_SHORT(f->byte_p_spl) / hwparams.channels) {
		case 3:
			hwparams.format = SND_PCM_FORMAT_S24_3LE;
			break;
		case 4:
			hwparams.format = SND_PCM_FORMAT_S24_LE;
			break;
		default:
			ERR("Can't play WAVE-files with sample %d bits in %d bytes wide (%d channels)",
			      LE_SHORT(f->bit_p_spl), LE_SHORT(f->byte_p_spl), hwparams.channels);
			return -1;
		}
		break;
	case 32:
                if (LE_SHORT(f->format) == WAV_FMT_PCM)
                        hwparams.format = SND_PCM_FORMAT_S32_LE;
                else if (LE_SHORT(f->format) == WAV_FMT_IEEE_FLOAT)
                        hwparams.format = SND_PCM_FORMAT_FLOAT_LE;
		break;
	default:
		ERR(" can't play WAVE-files with sample %d bits wide", LE_SHORT(f->bit_p_spl));
		return -1;
	}
	hwparams.rate = LE_INT(f->sample_fq);

	if (size > len)
		memmove(buffer, buffer + len, size - len);
	size -= len;

	while (1) {
		u_int type, len;

		wav_check_file_space(buffer, sizeof(WaveChunkHeader), blimit);
		wav_test_file_read(fd, buffer, &size, sizeof(WaveChunkHeader), __LINE__);
		c = (WaveChunkHeader*)buffer;
		type = c->type;
		len = LE_INT(c->length);
		if (size > sizeof(WaveChunkHeader))
			memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
		size -= sizeof(WaveChunkHeader);
		if (type == WAV_DATA) {
			if (len < pbrec_count && len < 0x7ffffffe)
				pbrec_count = len;
			if (size > 0)
				memcpy(_buffer, buffer, size);
			free(buffer);
			return size;
		}
		len += len % 2;
		wav_check_file_space(buffer, len, blimit);
		wav_test_file_read(fd, buffer, &size, len, __LINE__);
		if (size > len)
			memmove(buffer, buffer + len, size - len);
		size -= len;
	}

	/* shouldn't be reached */
	return -1;
}

static void print_vu_meter_mono(int perc, int maxperc)
{
	const int bar_length = 50;
	char line[80];
	int val;

	for (val = 0; val <= perc * bar_length / 100 && val < bar_length; val++)
		line[val] = '#';
	for (; val <= maxperc * bar_length / 100 && val < bar_length; val++)
		line[val] = ' ';
	line[val] = '+';
	for (++val; val <= bar_length; val++)
		line[val] = ' ';
	if (maxperc > 99)
		sprintf(line + val, "| MAX");
	else
		sprintf(line + val, "| %02i%%", maxperc);
	fputs(line, stdout);
	if (perc > 100)
		INFO(" !clip  ");
}

static void print_vu_meter_stereo(int *perc, int *maxperc)
{
	const int bar_length = 35;
	char line[80];
	int c;

	memset(line, ' ', sizeof(line) - 1);
	line[bar_length + 3] = '|';

	for (c = 0; c < 2; c++) {
		int p = perc[c] * bar_length / 100;
		char tmp[4];
		if (p > bar_length)
			p = bar_length;
		if (c)
			memset(line + bar_length + 6 + 1, '#', p);
		else
			memset(line + bar_length - p - 1, '#', p);
		p = maxperc[c] * bar_length / 100;
		if (p > bar_length)
			p = bar_length;
		if (c)
			line[bar_length + 6 + 1 + p] = '+';
		else
			line[bar_length - p - 1] = '+';
		if (maxperc[c] > 99)
			sprintf(tmp, "MAX");
		else
			sprintf(tmp, "%02d%%", maxperc[c]);
		if (c)
			memcpy(line + bar_length + 3 + 1, tmp, 3);
		else
			memcpy(line + bar_length, tmp, 3);
	}
	line[bar_length * 2 + 6 + 2] = 0;
	fputs(line, stdout);
}

static void print_vu_meter(signed int *perc, signed int *maxperc)
{
	if (vumeter == VUMETER_STEREO)
		print_vu_meter_stereo(perc, maxperc);
	else
		print_vu_meter_mono(*perc, *maxperc);
}

static void print_file_header(const char *file)
{
	INFO("%s '%s' : ", (pcm_stream == SND_PCM_STREAM_PLAYBACK) ? ("Playing") : ("Recording"), file);
	INFO("%s, ", snd_pcm_format_description(hwparams.format));
	INFO("Rate %d Hz, ", hwparams.rate);
	if (hwparams.channels == 1)
		INFO("Mono");
	else if (hwparams.channels == 2)
		INFO("Stereo");
	else
		INFO("Channels %i", hwparams.channels);
	INFO("\n");
}

static void print_pcm_available_sample_formats(snd_pcm_hw_params_t* params)
{
	snd_pcm_format_t format = 0;

	INFO("Available formats:\n");
	for (; format < SND_PCM_FORMAT_LAST; format++) {
		if (snd_pcm_hw_params_test_format(pcm_handle, params, format) == 0)
			INFO("- %s\n", snd_pcm_format_name(format));
	}
}

/* peak handler */
static void compute_max_peak(u_char *data, size_t count)
{
	signed int val, max, perc[2], max_peak[2];
	static	int	run = 0;
	size_t ocount = count;
	int	format_little_endian = snd_pcm_format_little_endian(hwparams.format);
	int ichans, c;
	static int maxperc[2];
	static time_t t=0;
	const time_t tt=time(NULL);

	if (vumeter == VUMETER_STEREO)
		ichans = 2;
	else
		ichans = 1;

	memset(max_peak, 0, sizeof(max_peak));
	switch (bits_per_sample) {
	case 8: {
		signed char *valp = (signed char *)data;
		signed char mask = snd_pcm_format_silence(hwparams.format);
		c = 0;
		while (count-- > 0) {
			val = *valp++ ^ mask;
			val = abs(val);
			if (max_peak[c] < val)
				max_peak[c] = val;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	case 16: {
		signed short *valp = (signed short *)data;
		signed short mask = snd_pcm_format_silence_16(hwparams.format);
		signed short sval;

		count /= 2;
		c = 0;
		while (count-- > 0) {
			if (format_little_endian)
				sval = __le16_to_cpu(*valp);
			else
				sval = __be16_to_cpu(*valp);
			sval = abs(sval) ^ mask;
			if (max_peak[c] < sval)
				max_peak[c] = sval;
			valp++;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	case 24: {
		unsigned char *valp = data;
		signed int mask = snd_pcm_format_silence_32(hwparams.format);

		count /= 3;
		c = 0;
		while (count-- > 0) {
			if (format_little_endian) {
				val = valp[0] | (valp[1]<<8) | (valp[2]<<16);
			} else {
				val = (valp[0]<<16) | (valp[1]<<8) | valp[2];
			}
			/* Correct signed bit in 32-bit value */
			if (val & (1<<(bits_per_sample-1))) {
				val |= 0xff<<24;	/* Negate upper bits too */
			}
			val = abs(val) ^ mask;
			if (max_peak[c] < val)
				max_peak[c] = val;
			valp += 3;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	case 32: {
		signed int *valp = (signed int *)data;
		signed int mask = snd_pcm_format_silence_32(hwparams.format);

		count /= 4;
		c = 0;
		while (count-- > 0) {
			if (format_little_endian)
				val = __le32_to_cpu(*valp);
			else
				val = __be32_to_cpu(*valp);
			val = abs(val) ^ mask;
			if (max_peak[c] < val)
				max_peak[c] = val;
			valp++;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	default:
		if (run == 0) {
			ERR("Unsupported bit size %d.\n", (int)bits_per_sample);
			run = 1;
		}
		return;
	}
	max = 1 << (bits_per_sample-1);
	if (max <= 0)
		max = 0x7fffffff;

	for (c = 0; c < ichans; c++) {
		if (bits_per_sample > 16)
			perc[c] = max_peak[c] / (max / 100);
		else
			perc[c] = max_peak[c] * 100 / max;
	}
	if(tt>t) {
		t=tt;
		maxperc[0] = 0;
		maxperc[1] = 0;
	}
	for (c = 0; c < ichans; c++)
		if (perc[c] > maxperc[c])
			maxperc[c] = perc[c];

	putchar('\r');
	print_vu_meter(perc, maxperc);
	fflush(stdout);
}

/*  read function*/
static snd_pcm_uframes_t pcm_read(u_char *data, snd_pcm_uframes_t rcount)
{
	ssize_t r;
	size_t result = 0;
	snd_pcm_uframes_t count = rcount;
	if(data == 0){
		ERR("data is null\n");
		return 0;
	}
	if (count != chunk_size) {
		count = chunk_size;
	}
	while (count > 0) {
		r = snd_pcm_readi(pcm_handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
				snd_pcm_wait(pcm_handle, 1000);
		} else if (r == -EPIPE) {
			io_xrun_handler();
		} else if (r == -ESTRPIPE) {
			io_suspend_handler();
		} else if (r < 0) {
			ERR("read error: %s \n", snd_strerror(r));
			return -1;
		}
		if (r > 0) {
			if (vumeter)
				compute_max_peak(data, r * hwparams.channels);
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	return rcount;
}

/*  write function */
static ssize_t pcm_write(u_char *data, size_t count)
{
	ssize_t r;
	ssize_t result = 0;

	if (count < chunk_size) {
		snd_pcm_format_set_silence(hwparams.format, data + count * bits_per_frame / 8, (chunk_size - count) * hwparams.channels);
		count = chunk_size;
	}
	while (count > 0) {
		r = snd_pcm_writei(pcm_handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(pcm_handle, 1000);
		} else if (r == -EPIPE) {
			io_xrun_handler();
		} else if (r == -ESTRPIPE) {
			io_suspend_handler();
		} else if (r < 0) {
			ERR("write error: %s", snd_strerror(r));
			return -1;
		}
		if (r > 0) {
			if (vumeter)
				compute_max_peak(data, r * hwparams.channels);
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	return result;
}

static int pcm_set_params(void)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t buffer_size;
	int err = 0;
	int dir;
	static bool printable = true; /* print hw params for first time*/
	/* Allocate hw parameters object. */
	snd_pcm_hw_params_alloca(&params);
	/* Fill it in with default values. */
	err = snd_pcm_hw_params_any(pcm_handle, params);
	if (err < 0) {
		ERR("%s Broken configuration for this PCM: no configurations available %s\n", __FUNCTION__, snd_strerror(err));
		return err;
	}

	//set access right
	err = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		ERR("%s snd_pcm_hw_params_set_access cannot set access type:%s\n",__FUNCTION__, snd_strerror(err));
		return err;
	}
	//set sample format
	err = snd_pcm_hw_params_set_format(pcm_handle, params, hwparams.format);
	if (err < 0) {
		ERR("%s Sample format non available:%s\n",__FUNCTION__, snd_strerror(err));
		print_pcm_available_sample_formats(params);
		return err;
	}
	//set channel number
	err = snd_pcm_hw_params_set_channels(pcm_handle, params, hwparams.channels);
	if (err < 0) {
		ERR("%s Channels count %d non available: %s\n", __FUNCTION__, hwparams.channels, snd_strerror(err));
		return err;
	}
	//set sample rate
	err = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &hwparams.rate, 0);
	if (err < 0) {
		ERR("%s sample rate %d set fail:%s\n", __FUNCTION__, hwparams.rate, snd_strerror(err));
		return err;
	}

	chunk_size = FRAME_PER_PERIOD;
	/* Set period size to frames. */
	snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &chunk_size, &dir);

	/* Write the parameters to the driver */
	err = snd_pcm_hw_params(pcm_handle, params);
	if (err < 0) {
		ERR("%s unable to set hw parameters:%s\n", __FUNCTION__, snd_strerror(err));
		return err;
	}
	monotonic = snd_pcm_hw_params_is_monotonic(params);
	/* stereo VU-meter isn't always available... */
	if (vumeter == VUMETER_STEREO) {
		if (hwparams.channels != 2 )
			vumeter = VUMETER_MONO;
	}

	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
	bits_per_frame = bits_per_sample * hwparams.channels;
	chunk_bytes = chunk_size * bits_per_frame / 8;
	audiobuf = realloc(audiobuf, chunk_bytes);
	if (audiobuf == NULL) {
		ERR("not enough memory");
		return -ENOMEM;
	}

	snd_pcm_hw_params_get_period_time(params,  &period_time, &dir);
	if(printable){
		printable = false;
		INFO("Sample Rate:       %d\n", hwparams.rate);
		INFO("Sample Format:     %s\n", (char*)snd_pcm_format_name(hwparams.format));
		INFO("Number of Channels:%d\n", hwparams.channels);
		INFO("Bytes per Sample:  %d\n", bits_per_sample / 8);
	}
	return 0;
}

/* playing data */
static void playback_go(int fd, size_t loaded, off64_t count)
{
	int l, r;
	off64_t written = 0;
	off64_t c;

	while (loaded > chunk_bytes && written < count) {
		if (pcm_write(audiobuf + written, chunk_size) <= 0)
			return;
		written += chunk_bytes;
		loaded -= chunk_bytes;
	}
	if (written > 0 && loaded > 0)
		memmove(audiobuf, audiobuf + written, loaded);

	l = loaded;

	while (written < count) {
		do {
			c = count - written;
			if (c > chunk_bytes)
				c = chunk_bytes;
			c -= l;

			if (c == 0)
				break;
			r = safe_read(fd, audiobuf + l, c);
			if (r < 0) {
				ERR("safe_read ERR\n");
				return;
			}
			if (r == 0)
				break;
			l += r;
		} while ((size_t)l < chunk_bytes);
		l = l * 8 / bits_per_frame;
		r = pcm_write(audiobuf, l);
		if (r != l){
			break;
		}

		r = r * bits_per_frame / 8;
		written += r;
		l = 0;
	}
	INFO("\n");
	snd_pcm_drain(pcm_handle);
}

static int amixer_controls(void)
{
	int err;
	snd_hctl_t *handle;
	snd_hctl_elem_t *elem;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_info_alloca(&info);

	if ((err = snd_hctl_open(&handle, card_name, 0)) < 0) {
		ERR("Control %s open error: %s", card_name, snd_strerror(err));
		return err;
	}
	if ((err = snd_hctl_load(handle)) < 0) {
		ERR("Control %s local error: %s\n", card_name, snd_strerror(err));
		return err;
	}
	INFO("controls:%s\n", card_name);
	for (elem = snd_hctl_first_elem(handle); elem; elem = snd_hctl_elem_next(elem)) {
		if ((err = snd_hctl_elem_info(elem, info)) < 0) {
			ERR("Control %s snd_hctl_elem_info error: %s\n", card_name, snd_strerror(err));
			return err;
		}
		if (snd_ctl_elem_info_is_inactive(info))
			continue;
		snd_hctl_elem_get_id(elem, id);
		amixer_show_control_id(id);
		INFO("\n");
		amixer_show_control("  ", elem, 1);
	}
	snd_hctl_close(handle);
	return 0;
}

static int amixer_cset(void)
{
	int err;
	static snd_ctl_t *handle = NULL;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;
	char *ptr;
	unsigned int idx, count;
	long tmp;
	snd_hctl_t *hctl;
	snd_hctl_elem_t *elem;
	snd_ctl_elem_type_t type;
	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);	/* default */
	snd_ctl_elem_id_set_numid(id, ctrl_num);

	if (handle == NULL &&
	    (err = snd_ctl_open(&handle, card_name, 0)) < 0) {
		ERR("Control %s open error: %s\n", card_name, snd_strerror(err));
		return err;
	}
	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		ERR("Cannot find the given element from control %s\n", card_name);
		snd_ctl_close(handle);
		handle = NULL;
		return err;
	}
	snd_ctl_elem_info_get_id(info, id);
	type = snd_ctl_elem_info_get_type(info);
	count = snd_ctl_elem_info_get_count(info);
	snd_ctl_elem_value_set_id(control, id);

	ptr = ctrl_val;
	for (idx = 0; idx < count && idx < 128 && ptr && *ptr; idx++) {
		switch (type) {
		case SND_CTL_ELEM_TYPE_BOOLEAN:
			tmp = 0;
			if (!strncasecmp(ptr, "on", 2) || !strncasecmp(ptr, "up", 2)) {
				tmp = 1;
				ptr += 2;
			} else if (!strncasecmp(ptr, "yes", 3)) {
				tmp = 1;
				ptr += 3;
			} else if (!strncasecmp(ptr, "toggle", 6)) {
				tmp = snd_ctl_elem_value_get_boolean(control, idx);
				tmp = tmp > 0 ? 0 : 1;
				ptr += 6;
			} else if (isdigit(*ptr)) {
				tmp = atoi(ptr) > 0 ? 1 : 0;
				while (isdigit(*ptr))
					ptr++;
			} else {
				while (*ptr && *ptr != ',')
					ptr++;
			}
			snd_ctl_elem_value_set_boolean(control, idx, tmp);
			break;
		case SND_CTL_ELEM_TYPE_INTEGER:
			tmp = amixer_get_integer(&ptr,
						snd_ctl_elem_info_get_min(info),
						snd_ctl_elem_info_get_max(info));
			snd_ctl_elem_value_set_integer(control, idx, tmp);
			break;
		case SND_CTL_ELEM_TYPE_INTEGER64:
			tmp = amixer_get_integer64(&ptr,
						  snd_ctl_elem_info_get_min64(info),
						  snd_ctl_elem_info_get_max64(info));
			snd_ctl_elem_value_set_integer64(control, idx, tmp);
			break;
		case SND_CTL_ELEM_TYPE_ENUMERATED:
			tmp = amixer_get_ctl_enum_item_index(handle, info, &ptr);
			if (tmp < 0)
				tmp = amixer_get_integer(&ptr, 0, snd_ctl_elem_info_get_items(info) - 1);
			snd_ctl_elem_value_set_enumerated(control, idx, tmp);
			break;
		case SND_CTL_ELEM_TYPE_BYTES:
			tmp = amixer_get_integer(&ptr, 0, 255);
			snd_ctl_elem_value_set_byte(control, idx, tmp);
			break;
		default:
			break;
		}
		if (!strchr(ctrl_val, ','))
			ptr = ctrl_val;
		else if (*ptr == ',')
			ptr++;
	}
	if ((err = snd_ctl_elem_write(handle, control)) < 0) {
		ERR("Control %s element write error: %s\n", card_name, snd_strerror(err));
		snd_ctl_close(handle);
		handle = NULL;
		return err;
	}

	snd_ctl_close(handle);
	handle = NULL;

	if ((err = snd_hctl_open(&hctl, card_name, 0)) < 0) {
		ERR("Control %s open error: %s\n", card_name, snd_strerror(err));
		return err;
	}
	if ((err = snd_hctl_load(hctl)) < 0) {
		ERR("Control %s load error: %s\n", card_name, snd_strerror(err));
		return err;
	}
	elem = snd_hctl_find_elem(hctl, id);
	if (elem)
		amixer_show_control("  ", elem, LEVEL_BASIC | LEVEL_ID);
	else
		INFO("Could not find the specified element\n");
	snd_hctl_close(hctl);

	return 0;
}

static int audio_play(const char* wav_file)
{
	int fd = -1;
	int err = 0;
	int ret = 0;
	size_t  head_size = 0;
	ssize_t load_size = 0;
	static bool is_first = true;
	if(wav_file == NULL){
		ERR("%s .wav file is NULL \n", __FUNCTION__);
		return -EINVAL;
	}

	chunk_size = 1024;
	pbrec_count = LLONG_MAX;

	/*Initial audiobuf*/
	audiobuf = (u_char *)malloc(1024);
	if (audiobuf == NULL) {
		ERR("%s not enough memory\n",__FUNCTION__);
		return -ENOMEM;
	}

	err = snd_pcm_open(&pcm_handle, PCM_NAME, pcm_stream, 0);
	if (err < 0) {
		ERR("%s snd_pcm_open error: %s",__FUNCTION__, snd_strerror(err));
		ret = err;
		goto __end;
	}

	if ((fd = open(wav_file, O_RDONLY)) == -1) {
		ERR("%s open file:%s ERROR!\n",__FUNCTION__, wav_file);
		ret = -EIO;
		goto __end;
	}

	/* read the file header */
	head_size = sizeof(WaveHeader) + sizeof(WaveChunkHeader) + sizeof(WaveFmtBody);
	if ((size_t)safe_read(fd, audiobuf, head_size) != head_size) {
		ERR("%s read error name=%s  len=%d \n", __FUNCTION__, wav_file, head_size);
		ret = -EIO;
		goto __end;
	}
	load_size = wav_test_file(fd, audiobuf, head_size);
	if ( load_size >= 0) {

		pbrec_count = calc_data_count();
		if(is_first){
			is_first = false;
			print_file_header(wav_file);
		}
		err = pcm_set_params();
		if (err >= 0){
			playback_go(fd, load_size, pbrec_count);
		}else{
			ret = err;
		}
	} else {
		ERR("We only test .wav file! Check File type please!\n");
		ret = -EINVAL;
	}
__end:
	if(audiobuf != NULL)
		free(audiobuf);

	if (fd != 0)
		close(fd);

	snd_pcm_close(pcm_handle);
	return 0;
}


static int audio_capture(const char* wav_file)
{
	int wav_fd = -1;
	int err = 0;
	int ret = 0;
	long loops = 0;
	off64_t count = 0;	      /* number of bytes to capture */

	/*Initialize variables*/
	pbrec_count = LLONG_MAX;
	hwparams.channels = record_channels;
	hwparams.format = DEFAULT_RECORD_FORMAT;
	hwparams.rate = record_rate;
	if(record_sec != 0){
		timelimit = record_sec*1000000;
	}else{
		timelimit = DEFAULT_RECORD_TIME*1000000;
	}

	/*open wav file in which data will be saved*/
	if(wav_file == NULL){
		wav_file = DEFAULT_RECORD_FILE_NAME;
	}

	INFO("Record data will be be saved in %s\n", wav_file);
	INFO("Record time is %d seconds.\n", timelimit/1000000);
	remove(wav_file);
	if ((wav_fd = open(wav_file, O_WRONLY | O_CREAT, 0644)) == -1) {
		ERR("%s Error open: [%s]\n", __FUNCTION__, wav_file);
		return -EIO;
	}

	/* get number of bytes to capture */
	count = calc_data_count();
	if (count == 0)
		count = LLONG_MAX;
	/* WAVE-file should be even (I'm not sure), but wasting one byte
	 * isn't a problem (this can only be in 8 bit mono) */
	if (count < LLONG_MAX)
		count += count % 2;
	else
		count -= count % 2;

	/* setup sample header */
	wav_writeheader(wav_fd, count);
	/* Open PCM device for recording (capture) */
	err = snd_pcm_open(&pcm_handle, PCM_NAME, pcm_stream, 0);
	if (err < 0) {
		ERR("%s unable to open pcm device: %s\n", __FUNCTION__, snd_strerror(err));
		ret = err;
		goto clean_code;
	}

	print_file_header(wav_file);

	err = pcm_set_params();

	if(err < 0){
		ret = err;
		goto clean_code;
	}
	loops = timelimit / period_time;
	/* capture */
	fdcount = 0;
	while (loops > 0) {
		loops--;
		if (pcm_read(audiobuf, chunk_size) != chunk_size)
			break;

		if (write(wav_fd, audiobuf, chunk_bytes) != (ssize_t)chunk_bytes) {
			ERR("%s snd_pcm_readi short write error!\n", __FUNCTION__);
			break;
		}
		fdcount += chunk_bytes;
	}
	INFO("\n");

	wav_writetail(wav_fd);
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);

clean_code:
	close(wav_fd);
	if(audiobuf != NULL)
		free(audiobuf);
	return ret;
}

static void pool_headset_state()
{
	int fd = -1;
	int err = 0;
	char cur_state = 0;
	char pre_state = 0;
	int is_first = 1;
	int index =0;

	if ((fd = open(HEADSET_STATE_PATH, O_RDONLY)) == -1) {
		ERR("%s open file:%s ERROR! Can not get headset state!\n",__FUNCTION__, HEADSET_STATE_PATH);
		return;
	}

	while(test_loop >= 0){
		if (0 != lseek (fd,0,SEEK_SET)){
			fprintf(stderr,"%s lseek error, %d %s\n", __FUNCTION__, errno, strerror(errno));
			close(fd);
			return;
		}

		if(read(fd, &cur_state, 1) == 1){
			if(cur_state != pre_state || is_first == 1)
			{
				if(is_first){
					is_first = 0;
					INFO("Current Headset:%s\n", (cur_state == '1') ? "IN" : "OUT" );
				}else{
					INFO("%d Current Headset:%s\n", ++index,(cur_state == '1') ? "IN" : "OUT" );
				}
				test_loop--;
			}
			pre_state = cur_state;
		}else{
			ERR("ERROR\n");
		}
	}
	close(fd);
}

int main(int argc, char **argv)
{
	char *cmd;
	int rval = -EINVAL;

	if (argc < 2) {
		usage();
		return rval;
	}

	cmd = argv[1];
	argv++;
	argc--;
	process_options(argc, argv);
	if (strcmp(cmd, "mix") == 0) {
		if(is_mix_list){
			amixer_controls();
		}else if(ctrl_num != 0){
			amixer_cset();
		}else{
			usage();		}
	} else if (strcmp(cmd, "play") == 0) {
		INFO("Testing device:%s\n", PCM_NAME);
		pcm_stream = SND_PCM_STREAM_PLAYBACK;

		if(file_name != NULL){
			for(; test_loop >0; test_loop--)
				audio_play(file_name);
		}else{
			usage();
		}
	}else if(strcmp(cmd, "capture") == 0){
		INFO("Testing device:%s\n", PCM_NAME);
		pcm_stream = SND_PCM_STREAM_CAPTURE;
		audio_capture(file_name);
	}else if(strcmp(cmd, "headset") == 0){
		pool_headset_state();
	}else{
		usage();
	}

	return rval;
}
