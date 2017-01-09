/*
 * Copyright (C) 2012 The Android Open Source Project
 *  Written by Ken.kuang at Spreadtrum Inc.
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

#define LOG_TAG "audio_pga"
/*#define LOG_NDEBUG 0*/

#include <errno.h>
#include <expat.h>
#include <stdbool.h>
#include <stdio.h>

#include <cutils/log.h>

#include <tinyalsa/asoundlib.h>

#define BUF_SIZE 1024
#define PGA_XML_PATH "/system/etc/codec_pga.xml"
#define INITIAL_PROFILE_SIZE 8
#define INITIAL_PROFILE_ITEM_SIZE 2

struct pga_attribute_item {
	struct mixer_ctl *ctl;
	int max;
	int inverse;
};

struct pga_attribute {
	int length;
	int size;
	struct pga_attribute_item *item;
};

struct pga_item {
	struct mixer_ctl *ctl;
	int bit;
};

struct pga_profile {
	char *name;
	int length;
	int size;
	struct pga_item *item;
};

struct audio_pga {
	struct mixer *mixer;
	int num_pga_profiles;
	int pga_profile_size;
	struct pga_profile *profile;
	struct pga_attribute attribute;
};

struct config_parse_state {
	struct audio_pga *pga;
	struct pga_profile *profile;
	struct pga_attribute_item *attribute_item;
};

static inline int fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

const char *mixer_ctl_get_name_d(struct mixer_ctl *ctl)
{
	/*
	static char name[256] = {0};
	mixer_ctl_get_name(ctl, name, 255);
	return name;*/

	return mixer_ctl_get_name(ctl);
}

static struct pga_profile *profile_get_by_name(struct audio_pga *pga,
		const char *name)
{
	int i;

	for (i = 0; i < pga->num_pga_profiles; i++)
		if (strcmp(pga->profile[i].name, name) == 0)
			return &pga->profile[i];

	return NULL;
}

static struct pga_profile *profile_create(struct audio_pga *pga, const char *name)
{
	struct pga_profile *new_profile = NULL;

	if (profile_get_by_name(pga, name)) {
		ALOGE("Profile name '%s' already exists", name);
		return NULL;
	}

	/* check if we need to allocate more space for pga profile */
	if (pga->pga_profile_size <= pga->num_pga_profiles) {
		if (pga->pga_profile_size == 0)
			pga->pga_profile_size = INITIAL_PROFILE_SIZE;
		else
			pga->pga_profile_size *= 2;

		new_profile = realloc(pga->profile, pga->pga_profile_size *
				sizeof(struct pga_profile));
		if (new_profile == NULL) {
			ALOGE("Unable to allocate more profiles");
			return NULL;
		} else {
			pga->profile = new_profile;
		}
	}

	/* initialise the new profile */
	pga->profile[pga->num_pga_profiles].name = strdup(name);
	pga->profile[pga->num_pga_profiles].length = 0;
	pga->profile[pga->num_pga_profiles].size = 0;
	pga->profile[pga->num_pga_profiles].item = NULL;

	/* return the profile just added, then increment number of them */
	return &pga->profile[pga->num_pga_profiles++];
}

static bool profile_item_exists(struct pga_profile *profile,
		struct pga_item *item)
{
	int i;

	for (i = 0; i < profile->length; i++)
		if (profile->item[i].ctl == item->ctl)
			return true;

	return false;
}

static int profile_add_item(struct pga_profile *profile,
		struct pga_item *item)
{
	struct pga_item *new_item;

	if (profile_item_exists(profile, item)) {
		ALOGE("Duplicate profile item '%s'",
				mixer_ctl_get_name_d(item->ctl));
		return -1;
	}

	/* check if we need to allocate more space for profile items */
	if (profile->size <= profile->length) {
		if (profile->size == 0)
			profile->size = INITIAL_PROFILE_ITEM_SIZE;
		else
			profile->size *= 2;

		new_item = realloc(profile->item,
				profile->size * sizeof(struct pga_item));
		if (new_item == NULL) {
			ALOGE("Unable to allocate more profile items");
			return -1;
		} else {
			profile->item = new_item;
		}
	}

	/* initialise the new profile item */
	profile->item[profile->length].ctl = item->ctl;
	profile->item[profile->length].bit = item->bit;
	profile->length++;

	return 0;
}

static struct pga_attribute_item *attribute_get_by_ctl(struct pga_attribute *attribute,
		struct mixer_ctl *ctl)
{
	int i;

	for (i = 0; i < attribute->length; i++)
		if (attribute->item[i].ctl == ctl)
			return &attribute->item[i];

	return NULL;
}

static struct pga_attribute_item *attribute_create(struct audio_pga *pga,
		struct mixer_ctl *ctl)
{
	struct pga_attribute_item *new_item = NULL;

	if (attribute_get_by_ctl(&pga->attribute, ctl)) {
		ALOGE("Duplicate attribute item '%s'",
				mixer_ctl_get_name_d(ctl));
		return NULL;
	}

	/* check if we need to allocate more space for attribute items */
	if (pga->attribute.size <= pga->attribute.length) {
		if (pga->attribute.size == 0)
			pga->attribute.size = INITIAL_PROFILE_ITEM_SIZE;
		else
			pga->attribute.size *= 2;

		new_item = realloc(pga->attribute.item,
				pga->attribute.size * sizeof(struct pga_attribute_item));
		if (new_item == NULL) {
			ALOGE("Unable to allocate more attribute items");
			return NULL;
		} else {
			pga->attribute.item = new_item;
		}
	}

	/* initialise the new attribute item */
	pga->attribute.item[pga->attribute.length].ctl = ctl;
	pga->attribute.item[pga->attribute.length].max = 0;
	pga->attribute.item[pga->attribute.length].inverse = 0;

	return &pga->attribute.item[pga->attribute.length++];
}

static void audio_pga_start_tag(void *data, const XML_Char *tag_name,
		const XML_Char **attr)
{
	struct config_parse_state *state = data;
	struct audio_pga *pga = state->pga;
	unsigned int i;
	int value;
	struct mixer_ctl *ctl;
	struct pga_item item;

	/* Look at tags */
	if (strcmp(tag_name, "codec") == 0) {
		if (strcmp(attr[0], "name") == 0) {
			ALOGI("The codec name is %s", attr[1]);
		} else {
			ALOGE("Unnamed codec!");
		}
	}

	else if (strcmp(tag_name, "profile") == 0) {
		/* Obtain the profile name */
		if (strcmp(attr[0], "name") == 0) {
			ALOGV("profile name is '%s'", attr[1]);
			state->profile = profile_create(pga, attr[1]);
		} else {
			ALOGE("Unnamed profile!");
		}
	}

	else if (strcmp(tag_name, "pga") == 0) {
		if (state->profile) {
			/* Obtain the control name and pga bit filed */
			if (strcmp(attr[0], "name") != 0) {
				ALOGE("Unnamed control!");
				goto attr_err;
			}
			if (strcmp(attr[2], "bit") != 0) {
				ALOGE("'%s' No bit filed!", attr[0]);
				goto attr_err;
			}
			ALOGV("pga name is '%s', val is '%s'", attr[1], attr[3]);
			item.ctl = mixer_get_ctl_by_name(pga->mixer, attr[1]);
			if ( !item.ctl ) {
				ALOGE("'%s' can not access this sound card!", attr[1]);
			}
			item.bit = atoi((char *)attr[3]);

			profile_add_item(state->profile, &item);
		} else {
			ALOGE("error profile!");
		}
	}

	else if (strcmp(tag_name, "mixer") == 0) {
		/* Obtain the attribute name */
		if (strcmp(attr[0], "name") == 0) {
			ctl = mixer_get_ctl_by_name(pga->mixer, attr[1]);
			if ( !ctl ) {
				ALOGE("'%s' can not access this sound card!", attr[1]);
			}
			ALOGV("mixer name is '%s'", attr[1]);
			state->attribute_item = attribute_create(pga, ctl);
		} else {
			ALOGE("Unnamed attribute!");
		}
	}

	else if (strcmp(tag_name, "attr") == 0) {
		if (state->attribute_item) {
			/* Obtain the attribute max and inverse */
			if (strcmp(attr[0], "max") != 0) {
				ALOGE("'%s' No max field!",
						mixer_ctl_get_name_d(state->attribute_item->ctl));
				goto attr_err;
			}
			if (strcmp(attr[2], "inverse") != 0) {
				ALOGE("'%s' No inverse field!",
						mixer_ctl_get_name_d(state->attribute_item->ctl));
				goto attr_err;
			}
			ALOGV("attribute max is '%s', inverse is '%s'", attr[1], attr[3]);
			value = atoi((char *)attr[1]);
			state->attribute_item->max = value;
			value = atoi((char *)attr[3]);
			state->attribute_item->inverse = value;
		} else {
			ALOGE("error attribute!");
		}
	}
attr_err:
	return;
}

static void audio_pga_end_tag(void *data, const XML_Char *tag_name)
{
	struct config_parse_state *state = data;
}

static void audio_pga_mixer_set(struct mixer_ctl *ctl, int new_value)
{
	unsigned int i;
	for (i = 0; i < mixer_ctl_get_num_values(ctl); i++)
		mixer_ctl_set_value(ctl, i, new_value);
}

static void audio_pga_set(struct mixer_ctl *ctl, int value,
		struct pga_attribute_item *attribute_item)
{
	unsigned int mask;

	mask = (1 << fls(attribute_item->max)) - 1;
	value &= mask;
	if (attribute_item->inverse) {
		value = attribute_item->max - value;
	}
	ALOGI("'%s' set to %d", mixer_ctl_get_name_d(ctl), value);
	audio_pga_mixer_set(ctl, value);
}

/* Initialises and frees the audio PGA */
struct audio_pga *audio_pga_init(struct mixer *mixer)
{
	struct config_parse_state state;
	XML_Parser parser;
	FILE *file;
	int bytes_read;
	void *buf;
	int i;
	struct audio_pga *pga;

	pga = calloc(1, sizeof(struct audio_pga));
	if (!pga)
		goto err_calloc;

	pga->mixer = mixer;
	if (!pga->mixer) {
		ALOGE("Unable to open the mixer, aborting.");
		goto err_mixer_open;
	}

	pga->profile = NULL;
	pga->num_pga_profiles = 0;
	pga->pga_profile_size = 0;

	file = fopen(PGA_XML_PATH, "r");
	if (!file) {
		ALOGE("Failed to open %s", PGA_XML_PATH);
		goto err_fopen;
	}

	parser = XML_ParserCreate(NULL);
	if (!parser) {
		ALOGE("Failed to create XML parser");
		goto err_parser_create;
	}

	memset(&state, 0, sizeof(state));
	state.pga = pga;
	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, audio_pga_start_tag, audio_pga_end_tag);

	for (;;) {
		buf = XML_GetBuffer(parser, BUF_SIZE);
		if (buf == NULL)
			goto err_parse;

		bytes_read = fread(buf, 1, BUF_SIZE, file);
		if (bytes_read < 0)
			goto err_parse;

		if (XML_ParseBuffer(parser, bytes_read,
					bytes_read == 0) == XML_STATUS_ERROR) {
			ALOGE("Error in codec PGA xml (%s)", PGA_XML_PATH);
			goto err_parse;
		}

		if (bytes_read == 0)
			break;
	}

	XML_ParserFree(parser);
	fclose(file);
	return pga;

err_parse:
	XML_ParserFree(parser);
err_parser_create:
	fclose(file);
err_fopen:
err_mixer_open:
	free(pga);
	pga = NULL;
err_calloc:
	return NULL;
}

void audio_pga_free(struct audio_pga *pga)
{
	int i;

	if (!pga) {
		ALOGE("PGA is NULL");
		return;
	}

	for (i = 0; i < pga->num_pga_profiles; i++) {
		if (pga->profile[i].name)
			free(pga->profile[i].name);
		if (pga->profile[i].item)
			free(pga->profile[i].item);
	}
	free(pga->profile);
	free(pga->attribute.item);
	free(pga);
}

/* Applies an audio pga by name */
int audio_pga_apply(struct audio_pga *pga, int val, const char *name)
{
	struct pga_profile *profile = NULL;
	int i;

	if (!pga) {
		ALOGE("PGA is NULL");
		return -1;
	}

	ALOGD("'%s' apply", name);

	profile = profile_get_by_name(pga, name);
	if (!profile) {
		ALOGE("Profile name '%s' is not exists", name);
		return -1;
	}

	for (i = 0; i < profile->length; i++) {
		audio_pga_set(profile->item[i].ctl, (val >> profile->item[i].bit),
				attribute_get_by_ctl(&pga->attribute, profile->item[i].ctl));
	}

	return 0;
}

void audio_pga_test(struct audio_pga *pga)
{
	if (!pga) {
		ALOGE("PGA is NULL");
		return ;
	}

#if 0
	audio_pga_apply(pga, 0x00, "speaker");
	audio_pga_apply(pga, 0x00, "headphone");
	audio_pga_apply(pga, 0x00, "headphone-spk");
	audio_pga_apply(pga, 0x00, "earpiece");
	audio_pga_apply(pga, 0x00, "linein-spk");
	audio_pga_apply(pga, 0x00, "linein-hp");
	audio_pga_apply(pga, 0xff, "capture");
	audio_pga_apply(pga, 0xff, "linein-capture");
	audio_pga_apply(pga, 0xff, "test-fail");
#endif
}
