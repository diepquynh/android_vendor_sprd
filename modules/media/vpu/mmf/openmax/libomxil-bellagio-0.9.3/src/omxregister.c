/**
	src/omxregister.c

	Register OpenMAX components. This application registers the installed OpenMAX
	components and stores the list in the file:
	$HOME/.omxregistry

	It must be run before using components.
	The components are searched in the default directory:
	OMXILCOMPONENTSPATH
	If the components are installed in a different location, specify:

	omxregister-bellagio installation_path

	If the installation path parameter is not set also the environment variable
	BELLAGIO_SEARCH_PATH is checked.
	If set it contains the locations of the components, separated by colons

	Copyright (C) 2007-2010  STMicroelectronics
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

#include <dlfcn.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "st_static_component_loader.h"
#include "common.h"

#define DEFAULT_LINE_LENGHT 500
/** String element to be put in the .omxregister file to indicate  an
 * OpenMAX component and its roles
 */
static const char arrow[] =  " ==> ";

int int2strlen(int value) {
    int ret = 0;
    if (value<0) return -1;
    while(value>0) {
        value = value/10;
        ret++;
    }
    return ret;
}
/** This function shows all the components and related rules already registered
 * and described in the omxregister file
 */
static int showComponentsList(FILE* omxregistryfp) {
    char* buffer;
    char* temp_buffer, *temp_rules;
    char *comp_name, *temp_name, *comp_rules;
    char* checkChar;
    int data_read;
    int allocation_length = DEFAULT_LINE_LENGHT;
    long int start_pos, end_pos;
    long int offset;
    int i;

    buffer = malloc(allocation_length+1);
    comp_name = malloc(DEFAULT_LINE_LENGHT);
    temp_name = malloc(DEFAULT_LINE_LENGHT);
    comp_rules = malloc(DEFAULT_LINE_LENGHT);
    checkChar = malloc(2);

    printf("*********************************\n");
    printf("* List of registered components *\n");
    printf("*********************************\n");
    while(1) {
        //read line
        start_pos = ftell(omxregistryfp);
        do {
            data_read = fread(checkChar, 1, 1, omxregistryfp);
        } while ((*checkChar != '\n') && (data_read > 0));
        if (feof(omxregistryfp)) {
            break;
        }
        end_pos = ftell(omxregistryfp);
        offset = (end_pos - start_pos);
        fseek(omxregistryfp, start_pos, SEEK_SET);
        data_read = fread(buffer, offset, 1, omxregistryfp);
        buffer[offset] = '\0';
        if (buffer[0] == '/') {
            continue;
        }
        temp_buffer = buffer+5;
        i = 0;
        while ((temp_buffer[i] != '\0') && (temp_buffer[i] != ' ')) {
            i++;
        }
        strncpy(comp_name, temp_buffer, i);
        comp_name[i] = '\0';
        temp_buffer += i;
        if (*temp_buffer != '\0') {
            temp_buffer += 5;
            i = 0;
            while ((temp_buffer[i] != '\n') && (temp_buffer[i] != ' ')) {
                i++;
            }
            strncpy(comp_rules, temp_buffer, i);
            comp_rules[i] = '\0';
        } else {
            comp_rules[0] = '\0';
        }
        printf("Component %s\n", comp_name);
        if (comp_rules[0] != '\0') {
            temp_rules = comp_rules;
            printf("          supported formats:\n");
            i = 0;
            while (*(temp_rules+i) != '\0') {
                i++;
                if (*(temp_rules+i) == ':') {
                    strncpy(temp_name, temp_rules, i);
                    temp_name[i] = '\0';
                    temp_rules += i+1;
                    printf("             %s\n", temp_name);
                    i = 0;
                }
            }
        }
        printf("\n");
    }

    free(buffer);
    free(comp_name);
    free(temp_name);
    free(comp_rules);
    free(checkChar);

    return 0;
}
/** @brief Creates a list of components on a registry file
 *
 * This function
 *  - reads for the given directory(ies) any library contained
 *  - check if the library belongs to OpenMAX ST static component loader
 *    (it must contain the function omx_component_library_Setup for the initialization)
 *  - write the openmax names and related libraries to the registry file
 */
static int buildComponentsList(FILE* omxregistryfp, char *componentspath, int verbose) {
    DIR *dirp;
    struct dirent *dp;
    void *handle = NULL;
    int i, num_of_comp, k, qi;
    int num_of_libraries = 0;
    unsigned int j;
    char *buffer = NULL;
    int (*fptr)(void *);
    stLoaderComponentType **stComponents;
    int ncomponents = 0, nroles=0;
    int pathconsumed = 0;
    int currentgiven;
    int index;
    char* currentpath = componentspath;
    char* actual = NULL;
    int err = 0;
    nameList *allNames = NULL;
    nameList *currentName = NULL;
    nameList *tempName = NULL;
    char* qualityString = NULL;
    int index_string;
    /* the componentpath contains a single or multiple directories
     * and is is colon separated like env variables in Linux
     */
    err = err;
    qualityString = malloc(4096);
    buffer = malloc(8192);
    while (!pathconsumed) {
        index = 0;
        currentgiven = 0;
        while (!currentgiven) {
            if (*(currentpath + index) == '\0') {
                pathconsumed = 1;
            }
            if ((*(currentpath + index) == ':') || (*(currentpath + index) =='\0')) {
                currentgiven = 1;
                if (*(currentpath + index - 1) != '/') {
                    actual = malloc(index + 2);
                    *(actual + index) = '/';
                    *(actual+index + 1) = '\0';
                } else {
                    actual = malloc(index + 1);
                    *(actual+index) = '\0';
                }
                strncpy(actual, currentpath, index);
                currentpath = currentpath + index + 1;
            }
            index++;
        }
        /* Populate the registry file */
        dirp = opendir(actual);
        if (verbose) {
            printf("\n Scanning directory %s\n", actual);
        }
        if(dirp == NULL) {
            if (actual) {
                free(actual);
                DEBUG(DEB_LEV_SIMPLE_SEQ, "Cannot open directory %s\n", actual);
            }
            continue;
        }
        while((dp = readdir(dirp)) != NULL) {
            int len = strlen(dp->d_name);

            if(len >= 3) {


                if(strncmp(dp->d_name+len-3, ".so", 3) == 0) {
                    char lib_absolute_path[strlen(actual) + len + 1];

                    strcpy(lib_absolute_path, actual);
                    strcat(lib_absolute_path, dp->d_name);

                    if((handle = dlopen(lib_absolute_path, RTLD_NOW)) == NULL) {
                        DEBUG(DEB_LEV_ERR, "could not load %s: %s\n", lib_absolute_path, dlerror());
                    } else {
                        if (verbose) {
                            printf("\n Scanning library %s\n", lib_absolute_path);
                        }
                        if ((fptr = dlsym(handle, "omx_component_library_Setup")) == NULL) {
                            DEBUG(DEB_LEV_SIMPLE_SEQ, "the library %s is not compatible with ST static component loader - %s\n", lib_absolute_path, dlerror());
                            continue;
                        }
                        num_of_libraries++;
                        num_of_comp = fptr(NULL);
                        stComponents = malloc(num_of_comp * sizeof(stLoaderComponentType*));
                        for (i = 0; i<num_of_comp; i++) {
                            //stComponents[i] = calloc(1,sizeof(stLoaderComponentType));
                            stComponents[i] = malloc(sizeof(stLoaderComponentType));
                            memset(stComponents[i], 0x00, sizeof(stLoaderComponentType));
                            stComponents[i]->nqualitylevels = 0;
                            stComponents[i]->multiResourceLevel = NULL;
                        }
                        fptr(stComponents);
                        err = fwrite(lib_absolute_path, 1, strlen(lib_absolute_path), omxregistryfp);
                        err = fwrite("\n", 1, 1, omxregistryfp);


                        for (i = 0; i<num_of_comp; i++) {
                            tempName = allNames;
                            if (tempName != NULL) {
                                do  {
                                    if (!strcmp(tempName->name, stComponents[i]->name)) {
                                        DEBUG(DEB_LEV_ERR, "Component %s already registered. Skip\n", stComponents[i]->name);
                                        break;
                                    }
                                    tempName = tempName->next;
                                } while(tempName != NULL);
                                if (tempName != NULL) {
                                    continue;
                                }
                            }
                            if (allNames == NULL) {
                                allNames = malloc(sizeof(nameList));
                                currentName = allNames;
                            } else {
                                currentName->next = malloc(sizeof(nameList));
                                currentName = currentName->next;
                            }
                            currentName->next = NULL;
                            currentName->name = malloc(strlen(stComponents[i]->name) + 1);
                            strcpy(currentName->name, stComponents[i]->name);
                            *(currentName->name + strlen(currentName->name)) = '\0';

                            DEBUG(DEB_LEV_PARAMS, "Found component %s version=%d.%d.%d.%d in shared object %s\n",
                                  stComponents[i]->name,
                                  stComponents[i]->componentVersion.s.nVersionMajor,
                                  stComponents[i]->componentVersion.s.nVersionMinor,
                                  stComponents[i]->componentVersion.s.nRevision,
                                  stComponents[i]->componentVersion.s.nStep,
                                  lib_absolute_path);
                            if (verbose) {
                                printf("Component %s registered with %i quality levels\n", stComponents[i]->name, (int)stComponents[i]->nqualitylevels);
                            }
                            if (stComponents[i]->nqualitylevels > 0) {
                                index_string = 0;
                                sprintf((qualityString + index_string), "%i ", (int)stComponents[i]->nqualitylevels);
                                index_string = index_string + int2strlen(stComponents[i]->nqualitylevels) + 1;
                                for (qi=0; qi<(int)(stComponents[i]->nqualitylevels); qi++) {
                                    sprintf((qualityString + index_string), "%i,%i ",
                                            stComponents[i]->multiResourceLevel[qi]->CPUResourceRequested,
                                            stComponents[i]->multiResourceLevel[qi]->MemoryResourceRequested);
                                    index_string = index_string + 2 +
                                                   int2strlen(stComponents[i]->multiResourceLevel[qi]->CPUResourceRequested) +
                                                   int2strlen(stComponents[i]->multiResourceLevel[qi]->MemoryResourceRequested);
                                }
                                index_string--;
                                *(qualityString + index_string) = '\0';
                            }
                            // insert first of all the name of the library
                            strcpy(buffer, arrow);
                            strcat(buffer, stComponents[i]->name);
                            if (stComponents[i]->name_specific_length>0) {
                                nroles += stComponents[i]->name_specific_length;
                                strcat(buffer, arrow);
                                for(j=0; j<stComponents[i]->name_specific_length; j++) {
                                    if (verbose) {
                                        printf("  Specific role %s registered\n", stComponents[i]->name_specific[j]);
                                    }
                                    strcat(buffer, stComponents[i]->name_specific[j]);
                                    strcat(buffer, ":");
                                }
                            }

                            if ((qualityString != NULL) && (qualityString[0] != '\0')) {
                                strcat(buffer, arrow);
                                strcat(buffer, qualityString);
                            }
                            qualityString[0] = '\0';
                            strcat(buffer, "\n");
                            err = fwrite(buffer, 1, strlen(buffer), omxregistryfp);
                            ncomponents++;
                        }
                        for (i = 0; i < num_of_comp; i++) {
                            /** @@ Modified code
                            * added checking validation of a pointer variable once free
                            **/
                            if (stComponents[i]->name) {
                                free(stComponents[i]->name);
                            }
                            for (k=0; k<(int)(stComponents[i]->name_specific_length); k++) {
                                if (stComponents[i]->name_specific[k]) {
                                    free(stComponents[i]->name_specific[k]);
                                }
                                if (stComponents[i]->role_specific[k]) {
                                    free(stComponents[i]->role_specific[k]);
                                }
                            }
                            if (stComponents[i]->name_specific_length > 0) {
                                if (stComponents[i]->name_specific) {
                                    free(stComponents[i]->name_specific);
                                }
                                if (stComponents[i]->role_specific) {
                                    free(stComponents[i]->role_specific);
                                }

                            }
                            for (k=0; k<(int)(stComponents[i]->nqualitylevels); k++) {
                                if (stComponents[i]->multiResourceLevel[k]) {
                                    free(stComponents[i]->multiResourceLevel[k]);
                                }

                            }
                            if (stComponents[i]->multiResourceLevel) {
                                free(stComponents[i]->multiResourceLevel);
                            }
                            free(stComponents[i]);
                        }
                        free(stComponents);
                    }
                }
            }
        }
        free(actual);
        actual = NULL;
        closedir(dirp);
    }
    if (verbose) {
        printf("\n %i OpenMAX IL ST static components in %i libraries succesfully scanned\n", ncomponents, num_of_libraries);
    } else {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "\n %i OpenMAX IL ST static components with %i roles in %i libraries succesfully scanned\n", ncomponents, nroles, num_of_libraries);
    }
    free(qualityString);
    free(buffer);
    return 0;
}

static void usage(const char *app) {
    char *registry_filename;
    registry_filename = componentsRegistryGetFilename();

    printf(
        "Usage: %s [-l] [-v] [-h] [componentspath[:other_components_path]]...\n"
        "\n"
        "Version 0.9.2\n"
        "\n"
        "This programs scans for a given list of directory searching for any OpenMAX\n"
        "component compatible with the ST static component loader.\n"
        "The registry is saved under %s. (can be changed via OMX_BELLAGIO_REGISTRY\n"
        "environment variable)\n"
        "\n"
        "The following options are supported:\n"
        "\n"
        "        -v   display a verbose output, listing all the components registered\n"
        "        -l   list only the components already registered. If -l is specified \n"
        "             all the other parameters are ignored and only the register file\n"
        "             is checked\n"
        "        -h   display this message\n"
        "\n"
        "         componentspath: a searching path for components can be specified.\n"
        "         If this parameter is omitted, the components are searched in the\n"
        "         locations specified by the environment variable BELLAGIO_SEARCH_PATH.If it \n"
        "         is not defined the components are searched in the default %s directory \n"
        "\n",
        app, registry_filename, OMXILCOMPONENTSPATH);

    free(registry_filename);
}

/** @brief execution of registration function
 *
 * This register by default searches for OpenMAX libraries in OMXILCOMPONENTSPATH
 * If specified it can search in a different directory
 */
int main(int argc, char *argv[]) {
    int found;
    int err, i;
    int verbose=0;
    FILE *omxregistryfp;
    char *registry_filename;
    char *dir,*dirp;
    char *buffer;
    int isListOnly = 0;

    for(i = 1; i < argc; i++) {
        if(*(argv[i]) != '-') {
            continue;
        }
        if (*(argv[i]+1) == 'v') {
            verbose = 1;
        } else if (*(argv[i]+1) == 'l') {
            isListOnly = 1;
        } else {
            usage(argv[0]);
            exit(*(argv[i]+1) == 'h' ? 0 : -EINVAL);
        }
    }

    registry_filename = componentsRegistryGetFilename();

    /* make sure the registry directory exists */
    dir = strdup(registry_filename);
    if (dir == NULL) {
        exit(EXIT_FAILURE);
    }
    dirp = strrchr(dir, '/');
    if (dirp != NULL) {
        *dirp = '\0';
        if (makedir(dir)) {
            DEBUG(DEB_LEV_ERR, "Cannot create OpenMAX registry directory %s\n", dir);
            exit(EXIT_FAILURE);
        }
    }
    free(dir);

    if (isListOnly) {
        omxregistryfp = fopen(registry_filename, "r");
    } else {
        omxregistryfp = fopen(registry_filename, "w");
    }
    if (omxregistryfp == NULL) {
        DEBUG(DEB_LEV_ERR, "Cannot open OpenMAX registry file %s\n", registry_filename);
        exit(EXIT_FAILURE);
    }

    free(registry_filename);
    if (isListOnly) {
        err = showComponentsList(omxregistryfp);
        if(err) {
            DEBUG(DEB_LEV_ERR, "Error reading omxregister file\n");
        }
        exit(0);
    }

    for(i = 1, found = 0; i < argc; i++) {
        if(*(argv[i]) == '-') {
            continue;
        }

        found = 1;
        err = buildComponentsList(omxregistryfp, argv[i], verbose);
        if(err) {
            DEBUG(DEB_LEV_ERR, "Error registering OpenMAX components with ST static component loader %s\n", strerror(err));
            continue;
        }
    }

    if (found == 0) {
        buffer=getenv("BELLAGIO_SEARCH_PATH");
        if (buffer!=NULL&&*buffer!='\0') {
            err = buildComponentsList(omxregistryfp, buffer, verbose);
            if(err) {
                DEBUG(DEB_LEV_ERR, "Error registering OpenMAX components with ST static component loader %s\n", strerror(err));
            }
        } else {
            err = buildComponentsList(omxregistryfp, OMXILCOMPONENTSPATH, verbose);
            if(err) {
                DEBUG(DEB_LEV_ERR, "Error registering OpenMAX components with ST static component loader %s\n", strerror(err));
            }
        }
    }

    fclose(omxregistryfp);

    return 0;
}
