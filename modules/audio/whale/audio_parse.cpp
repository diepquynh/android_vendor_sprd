#define LOG_TAG "audio_hw_parse"
#define LOG_NDEBUG 0
#include "audio_hw.h"
#include "audio_control.h"
#include "audio_param/audio_param.h"
#include "audio_xml_utils.h"
#include <tinyxml.h>
#include <cutils/log.h>

#include<stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SPRDPHONE_CARD "sprdphone4adnc"

extern int get_snd_card_number(const char *card_name);

static struct mixer_control *alloc_mixer_control(struct device_control *control)
{
    struct mixer_control *new_ctl;
    new_ctl = (struct mixer_control *) realloc(control->ctl,
              (control->ctl_size + 1) * sizeof(struct mixer_control));
    if (!new_ctl) {
        LOG_E("failed to realloc mixer control");
        return NULL;
    } else {
        control->ctl = new_ctl;
    }
    control->ctl[control->ctl_size].ctl = NULL;
    control->ctl[control->ctl_size].name = NULL;
    control->ctl[control->ctl_size].value = 0;
    return &control->ctl[control->ctl_size++];
}

static int parse_mixer_control(struct mixer *mixer,
                               struct device_control *control, TiXmlElement *mixer_element)
{
    const char *ctl_name;
    const char *val_conut=NULL;
    int value;
    struct mixer_control *mixer_ctl;
    const char *str=NULL;
    mixer_ctl = alloc_mixer_control(control);
    if (mixer_ctl == NULL) {
        LOG_E("the mixer ctl is NULL");
        return -1;
    }
    ctl_name = mixer_element->Attribute("ctl");
    mixer_ctl->strval=NULL;

    str=mixer_element->Attribute("val");
    if(str!=NULL){
        /* This can be fooled but it'll do */
        value = atoi(str);
        if (!value && strcmp(str, "0") != 0)
            mixer_ctl->strval = strdup(str);
        else
            mixer_ctl->value=value;
    }
    mixer_element->Attribute("val", &value);
    mixer_ctl->name = strdup(ctl_name);
    mixer_ctl->val_count = 1;
    val_conut=mixer_element->Attribute("count");
    if(NULL!=val_conut){
        mixer_ctl->val_count=strtoul(val_conut,NULL,16);
    }
    mixer_ctl->ctl = mixer_get_ctl_by_name(mixer, ctl_name);
    mixer_ctl->value = value;
    //LOG_V("parse mixer control, Mixer=%s, Val=%d",ctl_name, value);
    return 0;
}

static int parse_route_control(struct mixer *mixer,
                               struct device_control *control, TiXmlElement *device)
{
    TiXmlElement *ctl = device->FirstChildElement();
    for (; ctl != NULL; ctl = ctl->NextSiblingElement()) {
        parse_mixer_control(mixer, control, ctl);
    }
    return 0;
}

static struct device_transition *alloc_transition(struct device_route *control)
{
    struct device_transition *new_trans;
    new_trans = (struct device_transition *) realloc(control->trans,
                (control->trans_size + 1) * sizeof(struct device_transition));
    if (!new_trans) {
        LOG_E("alloc device transition failed");
        return NULL;
    } else {
        control->trans = new_trans;
    }
    control->trans[control->trans_size].name = NULL;
    control->trans[control->trans_size].trans_ctl.name = NULL;
    control->trans[control->trans_size].trans_ctl.ctl_size = 0;
    control->trans[control->trans_size].trans_ctl.ctl = NULL;
    return &control->trans[control->trans_size++];
}

static int parse_trans_device(struct mixer *mixer, struct device_route *control,
                              TiXmlElement *device)
{
    TiXmlElement *ctls;
    struct device_transition *trans = alloc_transition(control);
    trans->name = strdup(device->Value());
    LOG_D("parse_trans_device,  Name=%s", trans->name);

    ctls = device->FirstChildElement();
    for (; ctls != NULL; ctls = ctls->NextSiblingElement()) {
        parse_mixer_control(mixer, &trans->trans_ctl, ctls);
    }
    return 0;
}

static int parse_transition_control(struct mixer *mixer,
                                    struct device_route *control, TiXmlElement *trans)
{
    TiXmlElement *device = trans->FirstChildElement();
    for (; device != NULL; device = device->NextSiblingElement()) {
        parse_trans_device(mixer, control, device);
    }
    return 0;
}

static struct device_route *alloc_route_device(struct device_route_handler
        *route)
{
    struct device_route *new_route;
    new_route = (struct device_route *)realloc(route->route,
                (route->size + 1) * sizeof(struct device_route));
    if (new_route == NULL) {
        LOG_E("alloc device_route failed");
        return NULL;
    } else {
        route->route = new_route;
    }
    route->route[route->size ].name = NULL;
    route->route[route->size ].ctl_on.name = NULL;
    route->route[route->size ].ctl_on.ctl = NULL;
    route->route[route->size ].ctl_on.ctl_size = 0;
    route->route[route->size ].ctl_off.name = NULL;
    route->route[route->size ].ctl_off.ctl = NULL;
    route->route[route->size ].ctl_off.ctl_size = 0;
    route->route[route->size ].trans_size = 0;
    route->route[route->size ].trans = NULL;
    return &route->route[route->size++];
}

static int parse_route_device(struct device_route_handler *route_hanler,
                              struct mixer *mixer, TiXmlElement *device)
{
    struct device_route *route = NULL;
    const char *tmp=NULL;

    route_hanler->route = NULL;
    route_hanler->size = 0;
    for (; device != NULL; device = device->NextSiblingElement()){
        route = alloc_route_device(route_hanler);
        if (route == NULL) {
            LOG_E("the device route is NULL");
            return -1;
        }
        route->name = strdup(device->Value());
        tmp=device->Attribute("device");
        if(tmp!=NULL){
            route->devices = strtoul(device->Attribute("device"),NULL,16);
        }else{
            LOG_E("parse_route_device, Device=%s ERR", route->name);
            route->devices=-1;
        }

        LOG_V("parse_route_device, Device=%s", route->name);
        TiXmlElement *ctl_on = device->FirstChildElement("on");
        if (ctl_on != NULL) {
            parse_route_control(mixer, &route->ctl_on, ctl_on);
        } else {
            LOG_E("ctl on is NULL, device=%s", device->Value());
        }
        TiXmlElement *ctl_off = device->FirstChildElement("off");
        if (ctl_off != NULL) {
            parse_route_control(mixer, &route->ctl_off, ctl_off);
        } else {
            LOG_E("ctl off is NULL, device=%s", device->Value());
        }
        TiXmlElement *ctl_trans = device->FirstChildElement("transition");
        if (ctl_trans != NULL) {
            LOG_D("parse transition,devices:%s", route->name);
            parse_transition_control(mixer, route, ctl_trans);
        } else {
            LOG_W("ctl transition is NULL device=%s", device->Value());
        }
    }
    return 0;
}


static struct device_control *alloc_device_control( struct private_control
        *priv_ctl)
{
    struct device_control *new_priv_ctl;
    new_priv_ctl = (struct device_control *)realloc(priv_ctl->priv,
                   (priv_ctl->size + 1) * sizeof(struct device_control));
    if (!new_priv_ctl) {
        LOG_E("alloc private control failed, error");
        return NULL;
    } else {
        priv_ctl->priv = new_priv_ctl;
    }

    priv_ctl->priv[priv_ctl->size].ctl_size = 0;
    priv_ctl->priv[priv_ctl->size].name = NULL;
    priv_ctl->priv[priv_ctl->size].ctl = NULL;
    return &priv_ctl->priv[priv_ctl->size++];
}

static void do_parse_private_control(struct private_control   *priv_ctl,
                                     struct mixer *mixer, TiXmlElement *priv)
{
    struct device_control *dev_ctl = alloc_device_control(priv_ctl);
    if (dev_ctl == NULL) {
        return;
    }
    dev_ctl->name = strdup(priv->Value());
    TiXmlElement *ctl = priv->FirstChildElement();
    for (; ctl != NULL; ctl = ctl->NextSiblingElement()) {
        parse_mixer_control(mixer, dev_ctl, ctl);
    }
}

static int parse_private_control(struct private_control   *priv_ctl,
                                 struct mixer *mixer, TiXmlElement *privs)
{
    priv_ctl->size = 0;
    priv_ctl->priv = NULL;
    TiXmlElement *ctl = privs->FirstChildElement();
    for (; ctl != NULL; ctl = ctl->NextSiblingElement()) {
        do_parse_private_control(priv_ctl, mixer, ctl);
    }
    return 0;
}

static struct dsploop_control *alloc_dsploop_control( struct private_dsploop_control *dsp_ctl)
{
    struct dsploop_control *new_priv_ctl;
    new_priv_ctl = (struct dsploop_control *)realloc(dsp_ctl->dsp_ctl,
                   (dsp_ctl->size + 1) * sizeof(struct dsploop_control));
    if (!new_priv_ctl) {
        LOG_E("alloc private control failed, error");
        return NULL;
    } else {
        dsp_ctl->dsp_ctl = new_priv_ctl;
    }

    dsp_ctl->dsp_ctl[dsp_ctl->size].ctl_size = 0;
    dsp_ctl->dsp_ctl[dsp_ctl->size].type = -1;
    dsp_ctl->dsp_ctl[dsp_ctl->size].ctl = NULL;
    return &dsp_ctl->dsp_ctl[dsp_ctl->size++];
}

static void do_parse_dsploop_control(struct private_dsploop_control *dsploop_ctl,
                                     struct mixer *mixer, TiXmlElement *priv)
{
    struct dsploop_control *dev_ctl = alloc_dsploop_control(dsploop_ctl);
    struct device_control control;
    const char *type_str=NULL;
    if (dev_ctl == NULL) {
        return;
    }

    control.ctl_size=0;
    control.ctl=NULL;
    control.name=NULL;

    type_str=priv->Attribute("val");
    if(NULL==type_str){
        LOG_E("do_parse_dsploop_control not fined %s Attribute:val",priv->Value());
        return ;
    }

    dev_ctl->type = strtoul(type_str,NULL,16);

    TiXmlElement *ctl = priv->FirstChildElement();
    for (; ctl != NULL; ctl = ctl->NextSiblingElement()) {
        parse_mixer_control(mixer, &control, ctl);
        dev_ctl->ctl_size=control.ctl_size;
        dev_ctl->ctl=control.ctl;
    }
}

static int parse_dsploop_control(struct private_dsploop_control *dsploop_ctl,
                                 struct mixer *mixer, TiXmlElement *privs)
{
    dsploop_ctl->size = 0;
    dsploop_ctl->dsp_ctl = NULL;
    TiXmlElement *ctl = privs->FirstChildElement();
    for (; ctl != NULL; ctl = ctl->NextSiblingElement()) {
        do_parse_dsploop_control(dsploop_ctl, mixer, ctl);
    }
    return 0;
}


static int parse_compress_config(struct _compr_config *compress, TiXmlElement *priv)
{
    const char *tmp=NULL;
    tmp=priv->Attribute("fragment_size");
    if(NULL!=tmp){
        compress->fragment_size=strtoul(tmp,NULL,16);
    }

    tmp=priv->Attribute("fragments");
    if(NULL!=tmp){
        compress->fragments=strtoul(tmp,NULL,16);
    }

    tmp=priv->Attribute("device");
    if(NULL!=tmp){
        compress->devices=strtoul(tmp,NULL,16);
    }

    return 0;
}

static int parse_pcm_config(struct pcm_config *config, int *devices, TiXmlElement *priv)
{
    struct TiXmlAttribute    *attr_ptr = NULL;
    struct TiXmlElement *pcm_tmp = NULL;
    struct TiXmlElement *pre_pcm_tmp = NULL;
    unsigned char *tmp = NULL;
    int pcm_config_buf[AUD_PCM_ATTRIBUTE_MAX] = {0};
    int i, j;

    int config_id = -1;
    int  attribute_id = -1;
    LOG_D("parse_pcm_config\n");
    pcm_tmp = NULL;
    for(i = 0; i < AUD_PCM_MAX; i++) {
        if(0 == i) {
            pcm_tmp = priv->FirstChildElement(pcm_config_name[i]);
        } else {
            pcm_tmp =  pcm_tmp->NextSiblingElement(pcm_config_name[i]);
        }

        if(pcm_tmp == NULL) {
            LOG_E("find  failed string:%s", pcm_config_name[i]);
            pcm_tmp = pre_pcm_tmp;
            continue;
        } else {
            pre_pcm_tmp = pcm_tmp;
            attr_ptr = pcm_tmp->FirstAttribute();
            for(j = 0; j < AUD_PCM_ATTRIBUTE_MAX; j++) {
                tmp = (unsigned char *)pcm_tmp->Attribute(pcm_config_attribute[j]);
                if(tmp != NULL) {
                    pcm_config_buf[j] = strtoul((const char *)tmp, NULL, 10);
                } else {
                    pcm_config_buf[j] = 0;
                }
            }

            config_id = i;
            config[config_id].channels = pcm_config_buf[AUD_PCM_ATTRIBUTE_CHANNELS];
            config[config_id].rate = pcm_config_buf[AUD_PCM_ATTRIBUTE_RATE];
            config[config_id].period_size = pcm_config_buf[AUD_PCM_ATTRIBUTE_PERIOD_SIZE];
            config[config_id].period_count = pcm_config_buf[AUD_PCM_ATTRIBUTE_PERIOD_COUNT];
            config[config_id].format = (enum pcm_format )pcm_config_buf[AUD_PCM_ATTRIBUTE_FORMAT];
            config[config_id].start_threshold = pcm_config_buf[AUD_PCM_ATTRIBUTE_START_THRESHOLD];
            config[config_id].stop_threshold = pcm_config_buf[AUD_PCM_ATTRIBUTE_STOP_THRESHOLD];
            config[config_id].silence_threshold = pcm_config_buf[AUD_PCM_ATTRIBUTE_SILENCE_THRESHOLD];
            config[config_id].avail_min = pcm_config_buf[AUD_PCM_ATTRIBUTE_AVAIL_MIN];
            devices[config_id] = pcm_config_buf[AUD_PCM_ATTRIBUTE_DEVICES];

            LOG_I("pcm config[%d] channels:%d rate:%d period_size:%d period_count:%d format:%d start:%d stop:%d silence:%d avail_min:%d device:%d",
                  config_id,
                  config[config_id].channels,
                  config[config_id].rate,
                  config[config_id].period_size,
                  config[config_id].period_count,
                  config[config_id].format,
                  config[config_id].start_threshold,
                  config[config_id].stop_threshold,
                  config[config_id].silence_threshold,
                  config[config_id].avail_min,
                  devices[config_id]
                 );
        }

    }

    return 0;
}

int parse_audio_pcm_config(struct pcm_handle_t *pcm)
{
    TiXmlElement *root;
    TiXmlElement *pcm_tmp;
    TiXmlElement *playback;
    TiXmlElement *record;

    int i = 0;
    int config_size = 7;
    LOG_D("parse_audio_pcm_config\n");
    memset(pcm, 0, sizeof(struct pcm_handle_t));
    TiXmlDocument *doc = new TiXmlDocument();
    if (!doc->LoadFile(AUDIO_PCM_MAGAGER_PATH)) {
        LOG_E("failed to load the %s", AUDIO_PCM_MAGAGER_PATH);
        delete doc;
        return -1;
    }

    root = doc->FirstChildElement();
    if(root == NULL) {
        LOG_E("find  root failed string:");
        return -1;
    }

    pcm_tmp = NULL;
    pcm_tmp = root->FirstChildElement("playback");
    if(pcm_tmp == NULL) {
        LOG_E("find  playback failed string:%s", root->GetText());
    } else {
        LOG_D("parse_audio_pcm_config :%s", pcm_tmp->Value());
        parse_pcm_config(pcm->play, pcm->playback_devices,pcm_tmp);
    }

    pcm_tmp = NULL;
    pcm_tmp = root->FirstChildElement("record");
    if(pcm_tmp == NULL) {
        LOG_E("find  record failed string:%s", root->GetText());
    } else {
        LOG_D("parse_audio_pcm_config :%s", pcm_tmp->Value());
        parse_pcm_config(pcm->record,pcm->record_devices, pcm_tmp);
    }

    pcm_tmp = NULL;
    pcm_tmp = root->FirstChildElement("compress");
    if(pcm_tmp == NULL) {
        LOG_E("find  record failed string:%s", root->GetText());
    } else {
        LOG_D("parse_compress_config :%s", pcm_tmp->Value());
        parse_compress_config(&(pcm->compress),pcm_tmp);
    }
    return 0;
}

static int parse_audio_route(struct audio_control *control)
{
    TiXmlElement *root;
    TiXmlElement *devices;
    TiXmlElement *private_control;
    TiXmlElement *dev;
    TiXmlDocument *doc = new TiXmlDocument();
    if (!doc->LoadFile(AUDIO_ROUTE_PATH)) {
        LOG_E("failed to load the route xml");
        return -1;
    }

    LOG_D("parse_audio_route");

    devices=NULL;
    root = doc->FirstChildElement();
    devices = root->FirstChildElement("devices");
    if(NULL==devices){
        LOG_E("parse_audio_route devices not find");
    }else{
        LOG_D("parse_audio_route devices");
        dev = devices->FirstChildElement();
        parse_route_device(&(control->route.devices_route), control->mixer, dev);
        control->route.pre_in_ctl=NULL;
        control->route.pre_out_ctl=NULL;
    }

    devices=NULL;
    devices = root->FirstChildElement("devices_call");
    if(NULL==devices){
        LOG_E("parse_audio_route devices_call not find");
    }else{
        LOG_D("parse_audio_route devices_call");
        dev = devices->FirstChildElement();
        parse_route_device(&(control->route.devices_route_call), control->mixer, dev);
    }

    private_control = root->FirstChildElement("private-control");
    if(NULL!=private_control){
        parse_private_control(&(control->route.priv_ctl), control->mixer,
                          private_control);
    }

    private_control = root->FirstChildElement("dsploop");
    if(NULL!=private_control){
        parse_dsploop_control(&(control->route.dsploop_ctl), control->mixer,
                          private_control);
    }

    LOG_I("parse_route_device vbc_iis");
    private_control = root->FirstChildElement("vbc_iis");
    if(NULL!=private_control){
        dev = private_control->FirstChildElement();
        if(NULL!=dev){
            parse_route_device(&(control->route.vbc_iis),control->mixer,
                          dev);
        }
    }
    delete doc;
    return 0;
}

static struct device_gain *alloc_device_gain(struct device_usecase_gain *gain)
{
    struct device_gain *new_gain;
    new_gain = (struct device_gain *) realloc(gain->dev_gain,
               (gain->gain_size + 1) * sizeof(struct device_gain));
    if (!new_gain) {
        LOG_E("alloc device gain failed");
        return NULL;
    } else {
        gain->dev_gain = new_gain;
        LOG_D("alloc_device_gain:%p", new_gain);
    }
    gain->dev_gain[gain->gain_size].name = NULL;
    gain->dev_gain[gain->gain_size].ctl_size = 0;
    gain->dev_gain[gain->gain_size].ctl = NULL;
    return & gain->dev_gain[gain->gain_size++];
}

static struct gain_mixer_control *alloc_gain_mixer_control(
    struct device_gain *gain)
{
    struct gain_mixer_control *new_gain_mixer;
    new_gain_mixer = (struct gain_mixer_control *) realloc(gain->ctl,
                     (gain->ctl_size + 1) * sizeof(struct gain_mixer_control));
    if (!new_gain_mixer) {
        return NULL;
    } else {
        gain->ctl = new_gain_mixer;
    }
    gain->ctl[gain->ctl_size].name = NULL;
    gain->ctl[gain->ctl_size].volume_size = 0;
    gain->ctl[gain->ctl_size].ctl = NULL;
    gain->ctl[gain->ctl_size].volume_value = NULL;
    return &gain->ctl[gain->ctl_size++];
}

static int *alloc_gain_volume(struct gain_mixer_control *gain_mixer)
{
    int *new_volume;
    new_volume = (int *) realloc(gain_mixer->volume_value,
                                 (gain_mixer->volume_size + 1) * sizeof(int));
    if (!new_volume) {
        LOG_E("alloc gain volume failed");
        return NULL;
    } else {
        gain_mixer->volume_value = new_volume;
    }
    return &gain_mixer->volume_value[gain_mixer->volume_size++];
}

static int parse_gain_volume(struct gain_mixer_control *gain_mixer,
                             TiXmlElement *mixer_element)
{
    int *volume = alloc_gain_volume(gain_mixer);
    *volume = strtoul(mixer_element->Attribute(VALUE),NULL,16);
    return 0;
}

static int parse_gain_mixer(struct mixer *mixer, struct device_gain *gain,
                            TiXmlElement *mixer_element)
{
    const char *svalue = NULL;
    const char *ctl_name = NULL;
    struct gain_mixer_control *gain_mixer = alloc_gain_mixer_control(gain);
    if (gain_mixer == NULL) {
        LOG_E("gain_mixer_control is NULL");
        return -1;
    }
    ctl_name = mixer_element->Attribute("ctl");
    if (ctl_name == NULL) {
        LOG_E("can not find the clt_name");
        return -1;
    }
    gain_mixer->name = strdup(ctl_name);
    gain_mixer->ctl = mixer_get_ctl_by_name(mixer, ctl_name);
    if ((svalue = mixer_element->Attribute(VALUE)) != NULL) {
        int *volume = alloc_gain_volume(gain_mixer);
        volume[0] = strtoul(svalue,NULL,16);
    } else {
        TiXmlElement *volume;
        volume = mixer_element->FirstChildElement();
        for (; volume != NULL; volume = volume->NextSiblingElement()) {
            parse_gain_volume(gain_mixer, volume);
        }
    }

    return 0;
}

int parse_device_gain(struct device_usecase_gain *use_gain,
                             struct mixer *mixer, TiXmlElement *device)
{
    struct device_gain *gain = NULL;
    if(NULL==use_gain->dev_gain){
        LOG_I("parse_device_gain dev_gain is null");
    }
    gain = alloc_device_gain(use_gain);
    gain->name = strdup(device->Value());
    gain->id=use_gain->gain_size-1;


    TiXmlElement *_device_gain = device->FirstChildElement();
    for (; _device_gain != NULL; _device_gain = _device_gain->NextSiblingElement()) {
        parse_gain_mixer(mixer, gain, _device_gain);
    }

    LOG_D("parse_device_gain %p %d %s gain_size:%d %p %p",
          gain, gain->ctl_size,    gain->name, use_gain->gain_size, use_gain->dev_gain,
          &(use_gain->dev_gain[0]));
    return 0;
}


static void free_mixer_control(struct mixer_control *ctl_mixer)
{
    if(ctl_mixer->name) {
        free(ctl_mixer->name);
        ctl_mixer->name = NULL;
    }

    if(ctl_mixer->strval) {
        free(ctl_mixer->strval);
        ctl_mixer->strval = NULL;
    }
#if 0
    if(ctl_mixer->ctl) {
        free(ctl_mixer->ctl);
        ctl_mixer->ctl = NULL;
    }
#endif
}

static void free_device_control(struct device_control *ctl)
{
    if (ctl->name) {
        free(ctl->name);
        ctl->name = NULL;
    }

    for(int i = 0; i < ctl->ctl_size; i++) {
        free_mixer_control(&(ctl->ctl[i]));
    }
}

static void free_transition(struct device_route *route)
{
    for (int i = 0; i < route->trans_size; i++) {
        if (route->trans[i].name) {
            free(route->trans[i].name);
            route->trans[i].name = NULL;
        }
        free_device_control(&route->trans[i].trans_ctl);
    }
}

void free_private_control(struct private_control *pri)
{
    for (int i = 0; i < pri->size; i++) {
        free_device_control(&pri->priv[i]);
    }
}


static void free_device_route(struct device_route_handler *reoute_handler)
{
    struct device_route *route = NULL;
    for (unsigned int i = 0; i < reoute_handler->size; i++) {
        route = &(reoute_handler->route[i]);
        if (route->name) {
            free(route->name);
            route->name = NULL;
        }
        free_device_control(&(route->ctl_on));
        free_device_control(&(route->ctl_off));
        free_transition(&route[i]);
    }
    //    free_private_control(control);

}

static void free_gain_control(struct gain_mixer_control *ctl)
{
    if (ctl->name) {
        free(ctl->name);
        ctl->name = NULL;
    }
    if (ctl->volume_value) {
        free(ctl->volume_value);
        ctl->volume_value = NULL;
    }
}

static int init_sprd_codec_mixer(struct sprd_codec_mixer_t *codec,struct mixer *mixer){

    codec->mic_boost=mixer_get_ctl_by_name(mixer, "MIC Boost");
    codec->auxmic_boost=mixer_get_ctl_by_name(mixer, "AUXMIC Boost");
    codec->headmic_boost=mixer_get_ctl_by_name(mixer, "HEADMIC Boost");

    codec->adcl_capture_volume=mixer_get_ctl_by_name(mixer, "ADCL Capture Volume");
    codec->adcr_capture_volume=mixer_get_ctl_by_name(mixer, "ADCR Capture Volume");

    codec->spkl_playback_volume=mixer_get_ctl_by_name(mixer, "SPKL Playback Volume");
    codec->spkr_playback_volume=mixer_get_ctl_by_name(mixer, "SPKR Playback Volume");

    codec->hpl_playback_volume=mixer_get_ctl_by_name(mixer, "HPL Playback Volume");
    codec->hpr_playback_volume=mixer_get_ctl_by_name(mixer, "HPR Playback Volume");

    codec->ear_playback_volume=mixer_get_ctl_by_name(mixer, "EAR Playback Volume");

    codec->inner_pa=mixer_get_ctl_by_name(mixer, "Inter PA Config");

    codec->hp_inner_pa=mixer_get_ctl_by_name(mixer, "Inter HP PA Config");

    codec->dacs_playback_volume=mixer_get_ctl_by_name(mixer, "DACS Playback Volume");

    codec->dacl_playback_volume=mixer_get_ctl_by_name(mixer, "DACL Playback Volume");

    codec->dacr_playback_volume=mixer_get_ctl_by_name(mixer, "DACR Playback Volume");

    codec->switch_route=3;

    return 0;
 }

struct audio_control *init_audio_control(struct tiny_audio_device *adev)
{
    int card_num, ret;
    int try_count=0;
    struct audio_control *control;
    struct mixer_ctl * codec_infor=NULL;
    int i=0;
    control = (struct audio_control *) malloc(sizeof(struct audio_control));
    if (!control) {
        LOG_E("init_audio_control malloc audio route failed");
        goto err_calloc;
    }

    memset(control,0,sizeof(struct audio_control));

    LOG_I("init_audio_control");
    pthread_mutex_init(&control->lock, NULL);
    control->adev = adev;
    control->usecase = UC_MUSIC;
    control->dg_gain.dev_gain=NULL;
    control->dg_gain.gain_size=0;
try_open:
    if(NULL!=adev->debug.card_name){
        card_num = get_snd_card_number(adev->debug.card_name);
    }else{
        card_num = get_snd_card_number(CARD_SPRDPHONE);
    }

    adev->control = control;
    control->mixer = mixer_open(card_num);
    if (!control->mixer) {
        LOG_E("init_audio_control Unable to open the mixer, aborting.");
        try_count++;
        if(try_count<=3){
            sleep(1);
            goto try_open;
        }
        goto err_mixer_open;
    }
    control->dg_gain.mixer=control->mixer;
    adev->mixer=control->mixer;

    ret = parse_audio_route(control);

    ret = parse_audio_pcm_config(&(control->pcm_handle));

    control->cards.s_tinycard = card_num;

    ret = stream_routing_manager_create(control);
    if (ret != 0) {
        LOG_E("init_audio_control stream_routing_manager_create failed ");
        goto err_mixer_open;
    }

    control->voice_ul_mute_ctl = mixer_get_ctl_by_name(control->mixer, VBC_UL_MUTE);
    control->voice_dl_mute_ctl = mixer_get_ctl_by_name(control->mixer, VBC_DL_MUTE);
    codec_infor=mixer_get_ctl_by_name(control->mixer, CODEC_INFOR_CTL);
    control->codec_type=-1;
    if(NULL!=codec_infor){
        const char *chip_str=NULL;
        chip_str=tinymix_get_enum(codec_infor);
        if(NULL!=chip_str){
            for(i=0;i<AUD_CODEC_TYPE_MAX;i++){
                if(strncmp(chip_str,audio_codec_chip_name[i],strlen(audio_codec_chip_name[i]))==0){
                    control->codec_type=i;
                    break;
                }
            }
        }
    }

    if(AUD_SPRD_2731S_CODEC_TYPE == control->codec_type){
        ret = init_sprd_codec_mixer(&(control->codec),control->mixer);
    }

    return control;

err_mixer_open:
    free(control);
    control = NULL;
err_calloc:
    return NULL;
}

void free_audio_control(struct audio_control *control)
{
    if (control->mixer) {
        mixer_close(control->mixer);
        control->mixer = NULL;
    }
    free_private_control(&(control->route.priv_ctl));
    free_device_route(&(control->route.devices_route));
}


//TODO we may add some folders for audio codec param
#ifdef __cplusplus
}
#endif
