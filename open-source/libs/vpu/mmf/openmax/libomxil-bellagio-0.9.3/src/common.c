/**
  src/common.c

  This file implements some useful common functionalities for handling the register files used in Bellagio

  Copyright (C) 2007-2011 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef ANDROID
#include "config_android.h"
#else
#endif
#include "common.h"

#define REGISTRY_FILENAME ".omxregister"

#ifdef ANDROID

#define TMP_DATA_DIRECTORY "/etc/"

#else
#define TMP_DATA_DIRECTORY "/tmp/"
#endif


/** @@ Modified code
 * if Android mode, fixed TMP_DATA_DIRECTORY
**/
#ifdef ANDROID
/** @brief Get registry filename
 * This function returns the name of the registry file for the components loaded with the default component loader.
 */
char* componentsRegistryGetFilename() {
    char *omxregistryfile = NULL;

    omxregistryfile  = malloc(strlen(TMP_DATA_DIRECTORY) + strlen(REGISTRY_FILENAME) + 2);
    if (!omxregistryfile) {
        printf("Insufficient memory in %s\n", __func__);
        return NULL;
    }

    strcpy(omxregistryfile, TMP_DATA_DIRECTORY);
    strcat(omxregistryfile, REGISTRY_FILENAME);

    return omxregistryfile;
}

/* This function return the absolute path of the registry_name file or
 * directory depending by the environment variable set.
 * the variables considered in order are:
 * $XDG_DATA_HOME
 * $HOME
 * TMP_DATA_DIRECTORY (/tmp by default on Linux)
 * The function does not check for file/dir existence
 */
char* loadersRegistryGetFilename(char* registry_name) {
    char *omxregistryfile = NULL;

    omxregistryfile  = malloc(strlen(TMP_DATA_DIRECTORY) + strlen(registry_name) + 2);
    if (!omxregistryfile) {
        printf("Insufficient memory in %s\n", __func__);
        return NULL;
    }

    strcpy(omxregistryfile, TMP_DATA_DIRECTORY);
    strcat(omxregistryfile, registry_name);

    return omxregistryfile;
}
#else
/** @brief Get registry filename
 * This function returns the name of the registry file for the components loaded with the default component loader.
 */
char* componentsRegistryGetFilename() {
    char *omxregistryfile = NULL;
    char *buffer;

    buffer=getenv("OMX_BELLAGIO_REGISTRY");
    if(buffer!=NULL&&*buffer!='\0') {
        omxregistryfile = strdup(buffer);
        return omxregistryfile;
    }

    buffer=getenv("XDG_DATA_HOME");
    if(buffer!=NULL&&*buffer!='\0') {
        omxregistryfile = malloc(strlen(buffer) + strlen(REGISTRY_FILENAME) + 2);
        if (!omxregistryfile) {
            printf("Insufficient memory in %s\n", __func__);
            return NULL;
        }
        strcpy(omxregistryfile, buffer);
        strcat(omxregistryfile, "/");
        strcat(omxregistryfile, REGISTRY_FILENAME);
        return omxregistryfile;
    }

    buffer=getenv("HOME");
    if(buffer!=NULL&&*buffer!='\0') {
        omxregistryfile  = malloc(strlen(buffer) + strlen(REGISTRY_FILENAME) + 3);
        if (!omxregistryfile) {
            printf("Insufficient memory in %s\n", __func__);
            return NULL;
        }
        strcpy(omxregistryfile, buffer);
        strcat(omxregistryfile, "/");
        strcat(omxregistryfile, REGISTRY_FILENAME);
    } else {
        omxregistryfile  = malloc(strlen(TMP_DATA_DIRECTORY) + strlen(REGISTRY_FILENAME) + 2);
        if (!omxregistryfile) {
            printf("Insufficient memory in %s\n", __func__);
            return NULL;
        }

        strcpy(omxregistryfile, TMP_DATA_DIRECTORY);
        strcat(omxregistryfile, REGISTRY_FILENAME);
    }
    return omxregistryfile;
}

/* This function return the absolute path of the registry_name file or
 * directory depending by the environment variable set.
 * the variables considered in order are:
 * $XDG_DATA_HOME
 * $HOME
 * TMP_DATA_DIRECTORY (/tmp by default on Linux)
 * The function does not check for file/dir existence
 */
char* loadersRegistryGetFilename(char* registry_name) {
    char *omxregistryfile = NULL;
    char *buffer;

    buffer=getenv("XDG_DATA_HOME");
    if(buffer!=NULL&&*buffer!='\0') {
        omxregistryfile = malloc(strlen(buffer) + strlen(registry_name) + 2);
        if (!omxregistryfile) {
            printf("Insufficient memory in %s\n", __func__);
            return NULL;
        }
        strcpy(omxregistryfile, buffer);
        strcat(omxregistryfile, "/");
        strcat(omxregistryfile, registry_name);
        return omxregistryfile;
    }

    buffer=getenv("HOME");
    if(buffer!=NULL&&*buffer!='\0') {
        omxregistryfile  = malloc(strlen(buffer) + strlen(registry_name) + 3);
        if (!omxregistryfile) {
            printf("Insufficient memory in %s\n", __func__);
            return NULL;
        }

        strcpy(omxregistryfile, buffer);
        strcat(omxregistryfile, "/");
        strcat(omxregistryfile, registry_name);
    } else {
        omxregistryfile  = malloc(strlen(TMP_DATA_DIRECTORY) + strlen(registry_name) + 2);
        if (!omxregistryfile) {
            printf("Insufficient memory in %s\n", __func__);
            return NULL;
        }

        strcpy(omxregistryfile, TMP_DATA_DIRECTORY);
        strcat(omxregistryfile, registry_name);
    }
    return omxregistryfile;
}
#endif

/** This function creates the directory specified by newdir
 * It returns 1 if is not possible to create it
 * It returns 0 if the directory is created or it already exists
 */
int makedir (const char *newdir) {
    char *buffer;
    char *p;
    int err;
    size_t len;

  /** @@ Modified code
    * added checking validation of a pointer variable once free
    **/
    buffer = strdup(newdir);
    len = strlen(buffer);

    if (len == 0) {
        if (buffer) {
            free(buffer);
        }
        return 1;
    }
    if (buffer[len-1] == '/') {
        buffer[len-1] = '\0';
    }

    err = mkdir(buffer, 0755);
    if (err == 0 || (( err == -1) && (errno == EEXIST))) {
        if (buffer) {
            free(buffer);
        }
        return 0;
    }

    p = buffer+1;
    while (1) {
        char hold;

        while(*p && *p != '\\' && *p != '/')
            p++;
        hold = *p;
        *p = 0;
        if ((mkdir(buffer, 0755) == -1) && (errno == ENOENT)) {
            if (buffer) {
                free(buffer);
            }
            return 1;
        }
        if (hold == 0)
            break;
        *p++ = hold;
    }
    if (buffer) {
        free(buffer);
    }
    return 0;

}

int exists(const char* fname) {
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

#ifdef COMMON_TEST
int main (int argc, char*argv[]) {
    printf (componentsRegistryGetFilename ());
}
#endif

