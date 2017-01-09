/*
 * tfaContainer.c
 *
 *    functions for creating container file and parsing of ini file
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <assert.h>
#include <tfaContainer.h>
#include "tfaFieldnames.h"
#include "minIni.h"
#include <sys/stat.h>

//#define DUMP
#ifdef DUMP
static void dumpDsc1st(nxpTfaDescPtr_t * dsc);
static void dumpStrings(void);
static void dumpDevs(nxpTfaContainer_t * cont);
static void dump(nxpTfaContainer_t * cont, int len);
#else
//static void dumpDsc1st(nxpTfaDescPtr_t * dsc){};
//static void dumpStrings(void){};
//static void dumpDevs(nxpTfaContainer_t * cont){};
static void dump(nxpTfaContainer_t * cont, int len){};
#endif


void tfaContIni2Container(char *iniFile){
    char cntFile[FILENAME_MAX], *dot;
    int size,i;

    strcpy(cntFile,  basename(iniFile));
    dot  = strrchr(cntFile, '.');
    if(dot) {
        *dot='\0';
    }
    strcat(cntFile,  ".cnt");

    printf("Generating container using %s.\n", iniFile);
    size = tfaContParseIni(iniFile, cntFile);
    printf("Created container %s of %d bytes.\n", cntFile, size);
}
/* Calculating ZIP CRC-32 in 'C'
   =============================
   Reference model for the translated code */

#define CRCpoly 0xEDB88320
/* Some compilers need
   #define poly 0xEDB88320uL
 */

/* On entry, addr=>start of data
             num = length of data
             crc = incoming CRC     */
uint32_t tfaContCRC32(uint8_t * addr, uint32_t num, uint32_t crc)
{
    int i;

    for (; num > 0; num--) {    /* Step through bytes in memory */
        crc = crc ^ *addr++;    /* Fetch byte from memory, XOR into CRC */
        for (i = 0; i < 8; i++) {    /* Prepare to rotate 8 bits */
            if (crc & 1)    /* b0 is set... */
                crc = (crc >> 1) ^ CRCpoly;    /* rotate and XOR with ZIP polynomic */
            else    /* b0 is clear... */
                crc >>= 1;    /* just rotate */
            /* Some compilers need:
               crc &= 0xFFFFFFFF;
             */
        }        /* Loop for 8 bits */
    }            /* Loop until num=0 */
    return (crc);        /* Return updated CRC */
}

static int fsize(const char *name)
{
    struct stat st;
    stat(name, &st);
    return st.st_size;
}

#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))


typedef enum keyType {
    kDevice,        // device list
    kProfile,        // profile list
    kRegister,        // register patch
    kString,        // ascii, zero terminated string
    kFilePatch,        // filename + file contents
    kFileConfig,        // filename + file contents
    kFilePreset,        // filename + file contents
    kFileEq,        // filename + file contents
    kFileDrc,        // filename + file contents
    kFileVolstep,        // filename + file contents
    //dscBitfieldBase=0x80 // start of bitfield enums
} keyType_t;

nxpTfaDescriptorType_t parseKeyType(char *key)
{
    if (strcmp("file", key) == 0)
        return dscFile;
    if (strcmp("patch", key) == 0)
        return dscFile;
    if (strcmp("config", key) == 0)
        return dscFile;
    if (strcmp("preset", key) == 0)
        return dscFile;
    if (strcmp("speaker", key) == 0)
        return dscFile;
    if (strcmp("drc", key) == 0)
        return dscFile;
    if (strcmp("eq", key) == 0)
        return dscFile;
    if (strcmp("volstep", key) == 0)
        return dscFile;
    if (strcmp("device", key) == 0)
        return dscDevice;
//      if (strcmp("profile", key)==0) return dscProfile;
    if (key[0] == '&')
        return dscProfile;
    if (key[0] == '$')
        return dscRegister;
    if (key[0] == '_')
        return dscBitfieldBase;
    if (strcmp("bus", key) == 0)
        return -1;    // skip
    if (strcmp("dev", key) == 0)
        return -1;    // skip
    if (strcmp("func", key) == 0)
        return -1;    // skip

    return dscString;    //unknown, assume dscString
}

//typedef enum nxpTfaBitEnum {
//       bfDCCV  = 0x0991,    /* Coil Value                                         */
//       bfNiks=0xffff
//} nxpTfaBitEnum_t;
static uint16_t getBitfield(char *name)
{
    if (strcmp(name, "DCCV") == 0)
        return bfDCCV;
    else
        return -1;
}

/*
 * lookup and fill the bit descriptor
 */
typedef union nxpTfaDscBit {
    nxpTfaDescPtr_t dsc;
    nxpTfaBitfield_t bf;
    uint32_t u32;
    uint8_t b[4];
} nxpTfaDscBit_t;
static int setBitfieldDsc(nxpTfaBitfield_t * bf, char *str, uint16_t value)
{
    nxpTfaDscBit_t *uni = (nxpTfaDscBit_t *) bf;
    // pre-formatted bitfield looks like _nnnCCCC : _001DCCV
    uint16_t bfEnum = getBitfield(&str[4]);

    if (bfEnum < 0xffff) {
        //      bf->field = bfEnum + 0x8000;
        uni->b[3] = (bfEnum >> 8) | 0x80;
        uni->b[2] = (uint8_t) bfEnum;
    } else {
        //printf("skipped unknown bitfield:%s\n", str);
        return 1;
    }
    //bf->value = value;
    uni->b[1] = value >> 8;
    uni->b[0] = (uint8_t) value;

    return 0;
}

/*
 * file processing key tables
 */


#define MAXKEYLEN 64
#define MAXLINELEN 256
#define MAXCONTAINER (256*1024)    // TODO make a smart size  estimator

static char *inifile; // global used in browse functions
typedef struct namelist {
    int n;
    int len;
    char names[64][MAXKEYLEN];
} namelist_t;

static int findDevices(const char *section, const char *key, const char *value,
               const void *userdata)
{
    namelist_t *keys;

    keys = (namelist_t *) userdata;
    keys->len++;
    // printf("    [%s]\t%s=%s\n", section, key, value);
    if (strcmp(section, "system") == 0 && strcmp(key, "device") == 0) {
        strncpy(keys->names[keys->n], value, MAXKEYLEN);
        keys->n++;
        keys->len--;    //don't count system items
        return 1;
    } else
        return 1;
}

static char *currentSection;
static int findProfiles(const char *section, const char *key, const char *value,
            const void *userdata)
{
    namelist_t *keys;
    char tmp[10];

    keys = (namelist_t *) userdata;
    keys->len++;
    // printf("    [%s]\t%s=%s\n", section, key, value);
    if (strcmp(section, currentSection) == 0 && strcmp(&key[4], "profile") == 0) {    // skip preformat
        // check if it exists
        if (ini_getkey(value, 0, tmp, 10, inifile) == 0) {
            printf("no profile section:%s\n", value);
            return 1;
        }

        strncpy(keys->names[keys->n], value, MAXKEYLEN);
        keys->n++;
        keys->len--;    //don't count system items
        return 1;
    } else
        return 1;
}

typedef struct browser {
    int idx;
    int cnt;
    int value;
    char section[MAXKEYLEN];
    char key[MAXKEYLEN];
    char str[MAXKEYLEN];
} browser_t;


static char **stringList;
static int *offsetList;        // for storing offsets of items to avoid duplicates
static uint16_t listLength = 0;
static char noname[] = "NONAME";
//static char nostring[] = "NOSTRING";

/*
 *  add string to list
 *   search for duplicate, return index if found
 *   malloc mem and copy string
 *   return index
 */
static int addString(char *strIn)
{
    int len;
    uint16_t idx;
    char *str;

    // skip pre format
    if (strIn[0] == '$' || strIn[0] == '&')
        str = &strIn[4];
    else
        str = strIn;

    len = strlen(str);
    for (idx = 0; idx < listLength; idx++) {
        if (strncmp(stringList[idx], str, len) == 0)
            return idx;
    }
    // new string
    idx = listLength++;
    stringList[idx] = malloc(len + 1);    //include \0
    assert(stringList[idx] != 0);
    strcpy(stringList[idx], str);

    return idx;
}
#ifdef DUMP
/*
 * dump a descriptor depending on it's type
 *  after 1st pass processing
 */
static void dumpDsc1st(nxpTfaDescPtr_t * dsc)
{
    uint32_t *ptr = (uint32_t *) dsc;
    uint32_t num = *ptr;

    switch (dsc->type) {
    case dscDevice:    // device list
        printf("device\n");
        break;
    case dscProfile:    // profile list
        printf("profile: %s\n", tfaContGetString(dsc));
        break;
    case dscRegister:    // register patch
        printf("register patch: %s\n", tfaContGetString(dsc));
        break;
    case dscString:    // ascii: zero terminated string
        printf("string: %s\n", tfaContGetString(dsc));
        break;
    case dscFile:        // filename + file contents
        printf("file: %s\n", tfaContGetString(dsc));
        break;
    default:
        if (dsc->type & dscBitfieldBase) {
            printf("dscBitfieldBase: 0x%08x\n", num);
            //dscBitfieldBase=0x80 // start of bitfield enums
        }
        break;

    }

}
static void dumpStrings(void)
{
    uint16_t idx;
    for (idx = 0; idx < listLength; idx++) {
        printf("[0x%0x] = %s\n", idx, stringList[idx]);
    }

}
static void dumpDevs(nxpTfaContainer_t * cont)
{
    nxpTfaDeviceList_t *dev;
    nxpTfaDescPtr_t *dsc;
    int i, d = 0;

    while ((dev = tfaContGetDevList(cont, d++)) != NULL) {
        dsc = &dev->name;
        if (dsc->type != dscString)
            printf("expected string: %s:%d\n", __FUNCTION__,
                   __LINE__);
        else
            printf("[%s] ", tfaContGetString(dsc));
        printf("devnr:%d length:%d bus=%d dev=0x%02x func=%d\n",    //TODO strings
               dsc->offset, dev->length, dev->bus, dev->dev, dev->func);
        for (i = 0; i < dev->length; i++) {
            dsc = &dev->list[i];
            dumpDsc1st(dsc);
        }
    }
}

/*
 * dump container file
 */
static void dump(nxpTfaContainer_t * cont, int len)
{
    nxpTfaDescPtr_t *dsc = (nxpTfaDescPtr_t *) cont->index;
    uint32_t *ptr;
    int i;

    printf("id:%.2s version:%.2s subversion:%.2s\n", cont->id,
           cont->version, cont->subversion);
    printf("size:%d CRC:0x%08x rev:%d\n", cont->size, cont->CRC, cont->rev);
    printf("customer:%.8s application:%.8s type:%.8s\n", cont->customer,
           cont->application, cont->type);
    printf("base=%p\n", cont);
    for (i = 0; i < len; i++) {
        ptr = (uint32_t *) dsc;
        printf("%p=dsc[%d]:type:0x%02x 0x%08x\n", ptr, i, dsc->type,
               *ptr);
//              printf("dsc[%d]:type=0x%02x offset:0x%06x\n",i, dsc->type,  dsc->offset[0] );
        dsc++;
    }
}
#endif //DUMP
/*
 * process system section from ini file
 */
static int systemSection(nxpTfaContainer_t * head)
{

    head->id[0] = (char)paramsHdr;
    head->id[1] = (char)(paramsHdr >> 8);
    head->version[0] = '1';
    head->version[1] = '_';
    head->subversion[0] = '0';
    head->subversion[1] = '0';

//    printf("id:%.2s version:%.2s subversion:%.2s\n", head->id,
//           head->version, head->subversion);
    head->rev = (uint16_t) ini_getl("system", "rev", 0, inifile);
    ini_gets("system", "customer", "", head->customer,
         sizeof head->customer, inifile);
    ini_gets("system", "customer", "", head->customer,
         sizeof head->customer, inifile);
    ini_gets("system", "application", "", head->application,
         sizeof head->application, inifile);
    ini_gets("system", "type", "", head->type, sizeof head->type, inifile);

    return 0;

}

/*
 * fill the offset(
 */
nxpTfaDescPtr_t *tfaContSetOffset(nxpTfaContainer_t * cont,
                  nxpTfaDescPtr_t * dsc, int idx)
{
    int offset = sizeof(nxpTfaContainer_t) + idx * sizeof(nxpTfaDescPtr_t);

    dsc->offset = offset;

    return dsc;
}

/*
 * return device list dsc from index
 */
nxpTfaDeviceList_t *tfaContGetDevList(nxpTfaContainer_t * cont, int idx)
{
    uint8_t *base = (uint8_t *) cont;

    if (cont->index[idx].type != dscDevice)
        return NULL;
    base += cont->index[idx].offset;
    return (nxpTfaDeviceList_t *) base;
}

/*
 * get the Nth profile for the Nth device
 */
nxpTfaProfileList_t *tfaContGetDevProfList(nxpTfaContainer_t * cont, int devIdx,
                       int profIdx)
{
    nxpTfaDeviceList_t *dev;
    int idx, hit;
    uint8_t *base = (uint8_t *) cont;

    dev = tfaContGetDevList(cont, devIdx);
    if (dev) {
        for (idx = 0, hit = 0; idx < dev->length; idx++) {
            if (dev->list[idx].type == dscProfile) {
                if (profIdx == hit++)
                    return (nxpTfaProfileList_t *) (dev->
                                    list
                                    [idx].
                                    offset +
                                    base);
            }
        }
    }

    return NULL;
}

/*
 * return 1st profile list
 */
nxpTfaProfileList_t *tfaContGet1stProfList(nxpTfaContainer_t * cont)
{
//      nxpTfaDescPtr_t *dsc = (nxpTfaDescPtr_t *)(cont->index + idx*(sizeof(nxpTfaDescPtr_t)/4));
    nxpTfaProfileList_t *prof;
    uint8_t *b = (uint8_t *) cont;

    int maxdev = 0;
    nxpTfaDeviceList_t *dev;

    // get nr of devlists+1
    while (tfaContGetDevList(cont, maxdev++)) ;
    maxdev--;
    // get last devlist
    dev = tfaContGetDevList(cont, maxdev - 1);
    // the 1st profile starts after the last device list
    b = (uint8_t *) dev + sizeof(nxpTfaDescPtr_t) +
        dev->length * (sizeof(nxpTfaDescPtr_t));
    prof = (nxpTfaProfileList_t *) b;
    return prof;
}

/*
 * lookup the string and return a ptr
 *
 */
char *tfaContGetString(nxpTfaDescPtr_t * dsc)
{
//      if ( dsc->type != dscString)
//              return nostring;
    if (dsc->offset > listLength)
        return noname;
    return stringList[dsc->offset];
}

nxpTfaProfileList_t *tfaContFindProfile(nxpTfaContainer_t * cont,
                    const char *name)
{
    int idx = 0, lastIdx, profIdx;
    nxpTfaProfileList_t *prof;
    char *profName;

    prof = tfaContGet1stProfList(cont);
    profIdx =
        (uint8_t *) cont - sizeof(nxpTfaContainer_t) - (uint8_t *) prof;
    profIdx /= 4;        //int32
    lastIdx = cont->size - profIdx;    // idx count is (now) in container size
    while (idx < lastIdx) {
        profName = tfaContGetString(&prof->name);
        if (strcmp(profName, name) == 0) {
            return prof;
        }
        // next
        idx += prof->length - 1;    // point after this profile list
        prof = (nxpTfaProfileList_t *) & prof->list[idx];
    }

    return NULL;
}

/*
 * all lists are parsed now so the offsets to profile lists can be filled in
 *  the device list has a dsc with the profile name, this need to become the byte
 *  offset of the profile list
 */
static void tfaContFixProfOffsets(nxpTfaContainer_t * cont)
{
    int i = 0, profIdx;
    nxpTfaDeviceList_t *dev;
    nxpTfaProfileList_t *prof;

    // walk through all device lists
    while ((dev = tfaContGetDevList(cont, i++)) != NULL) {
        // find all profiles in this dev
        for (profIdx = 0; profIdx < dev->length; profIdx++) {
            if (dev->list[profIdx].type == dscProfile) {
                //dumpDsc1st(&dev->list[profIdx]);
                prof = tfaContFindProfile(cont, tfaContGetString(&dev->list[profIdx]));    // find by name
                if (prof) {
                    dev->list[profIdx].offset = (uint8_t *) prof - (uint8_t *) cont;    // fix the offset into the container
                } else {
                    printf("Can't find profile:%s \n",
                           tfaContGetString(&dev->
                                list[profIdx]));
                }
            }
        }
    }
}

/*
 *   append and fix the offset for this item
 *   the item will be appended to the container at 'size' offset
 *   returns the byte size of the item added
 */
static int tfaContAddItem(nxpTfaContainer_t * cont, nxpTfaDescPtr_t * dsc)
{
    int size = 0, stringOffset = dsc->offset;
    char *str;
    FILE *f;
    uint8_t *dest = (uint8_t *) cont + cont->size;
    str = tfaContGetString(dsc);
    nxpTfaRegpatch_t pat;

    /*
     * check if this tiem is already in the offset list
     *  of so then the contents has already been appended
     *  and we only need to update the offset
     */
    if (stringOffset < listLength) {    //TODO check if this is good enough for check
        if (offsetList[stringOffset]) {
            dsc->offset = offsetList[stringOffset];    // only fix the offset
            return 0;
        }
    }
    switch (dsc->type) {
    case dscRegister:    // register patch : "$53=0x070,0x050"
        //printf("register patch: %s\n", str);
        if (sscanf
            (str, "$%hhx=%hx,%hx", &pat.address, &pat.mask,
             &pat.value) == 3) {
            size = sizeof(nxpTfaRegpatch_t);
            memcpy(dest, &pat, size);
        } else
            return 0;
        break;
    case dscString:    // ascii: zero terminated string
        strcpy((char*)dest, str);
        size = strlen(str) + 1;    // include \n
        //      printf("string: %s\n", tfaContGetString(dsc));
        break;
    case dscFile:        // filename + file contents
        f = fopen(str, "rb");
        if (!f) {
            printf("Unable to open %s\n", str);
            return 0;
        }
        size = fread(dest, 1, MAXCONTAINER - cont->size, f);
        fclose(f);
        //printf("file: %s\n", str);
        break;
    default:
        return 0;
        break;
    }
    offsetList[stringOffset] = dsc->offset = cont->size;
    cont->size += size;
    return size;
}

/*
 * walk through device lists and profile list
 *  if to-be fixed
 */
static void tfaContFixItemOffsets(nxpTfaContainer_t * cont)
{
    int i, j = 0, maxdev = 0, idx;
    nxpTfaDeviceList_t *dev;
    nxpTfaProfileList_t *prof;

    offsetList = malloc(sizeof(int) * listLength);
    bzero(offsetList, sizeof(int) * listLength);    // make all entries 0 first
    // walk through all device lists
    while ((dev = tfaContGetDevList(cont, maxdev++)) != NULL) {
        // fix name first
        tfaContAddItem(cont, &dev->name);
        for (idx = 0; idx < dev->length; idx++)
            tfaContAddItem(cont, &dev->list[idx]);
    }
    maxdev--;
    // walk through all profile lists
    for (i = 0; i < maxdev; i++) {
        while ((prof = tfaContGetDevProfList(cont, i, j++)) != NULL) {
            // fix name first
            tfaContAddItem(cont, &prof->name);
            for (idx = 0; idx < prof->length; idx++)
                tfaContAddItem(cont, &prof->list[idx]);
        }
    }
    free(offsetList);
}



/*
 * pre-format ini file
 *  This is needed for creating unique key names for bitfield
 *  and register keys.
 *  It simplifies the processing of multiple entries.
 */
static int preFormatInitFile(const char *infile, const char *outfile)
{
    FILE *in, *out;
    char buf[MAXLINELEN], *ptr;
    int cnt = 0;

    in = fopen(infile, "r");
    if (in == 0) {
        printf("error: can't open %s for reading\n", infile);
        exit(1);
    }
    out = fopen(outfile, "w");
    if (in == 0) {
        printf("error: can't open %s for writing\n", outfile);
        exit(1);
    }

    ptr = buf;
    while (!feof(in)) {
        fgets(ptr, sizeof(ptr), in);
        // skip space
        while (isblank(*ptr))
            ptr++;
        // ch pre format
        if (ptr[0] == '$')    // register
            fprintf(out, "$%03d%s", cnt++, ptr);
        else if (isupper(ptr[0]) && isupper(ptr[1]))    // bitfield
            fprintf(out, "_%03d%s", cnt++, ptr);
        else if (strcmp(ptr, "profile") == 0)
            fprintf(out, "&%03d%s", cnt++, ptr);
        else
            fputs(ptr, out);
    }
    fclose(out);
    fclose(in);

    return 0;
}

/*
 *
 */
int tfaContainerSave(nxpTfaContainer_t * cont, char *filename)
{
    FILE *f;
    int c;

    f = fopen(filename, "wb");
    if (!f) {
        printf("Unable to open %s\n", filename);
        return 0;
    }
    c = fwrite(cont, cont->size, 1, f);
    fclose(f);

    return c;
}

/*
 * create a big buffer to hold the entire container file
 *  return final file size
 */
int tfaContCreateContainer(nxpTfaContainer_t * contIn, char *outFileName)
{
    nxpTfaContainer_t *contOut;
    int size;

    contOut = malloc(MAXCONTAINER);
    if (contOut == 0) {
        printf("Can't allocate %d bytes.\n", MAXCONTAINER);
    }

    size = sizeof(nxpTfaContainer_t) + contIn->size * sizeof(nxpTfaDescPtr_t);    // still has the list count
    memcpy(contOut, contIn, size);
    //dump(contOut, contOut->size);
    contOut->size = size;    // now it's the actual size
    /*
     * walk through device lists and profile list
     *  if to-be fixed
     */
    tfaContFixItemOffsets(contOut);
    contOut->CRC =
        tfaContCRC32((uint8_t *) & contOut->CRC + 4, contOut->size, 0);
    tfaContainerSave(contOut, outFileName);
    dump(contOut, 29);
    size = contOut->size;
    free(contOut);

    return size;
}

/*
 * create the containerfile as descibred by the input 'ini' file
 *
 * return the of the new file size
 */
int tfaContParseIni( char *iniFile, char *outFileName) {
    int k, i, idx = 0;
    char value[64], key[64];
    nxpTfaContainer_t *headAndDescs;
    namelist_t keys, profiles;
    nxpTfaDescPtr_t *dsc;
    nxpTfaDeviceList_t *dev;
    nxpTfaProfileList_t *profhead;
    char preformatIni[FILENAME_MAX];
    /*
     *
     */
    strcpy(preformatIni,  basename(iniFile));
    strcat(preformatIni,  "_preformat.ini");
    preFormatInitFile(iniFile, preformatIni);
    inifile = preformatIni; // pint to the current inifile (yes, this is dirty ..)
    /*
     * process system section
     */
    // find devices in system section
    keys.n = 0;        // total nr of devices
    keys.len = 0;        // maximum length of all desc lists
    ini_browse(findDevices, &keys, preformatIni);    //also counts entries
    // get storage for container header and dsc lists
    headAndDescs = malloc(sizeof(nxpTfaContainer_t) + 2 * keys.len * 4);    //2*nr of entry line should enough
    bzero(headAndDescs, sizeof(nxpTfaContainer_t) + 2 * keys.len * 4);
    stringList = malloc(fsize(preformatIni));    //strings can't be longer the the ini file
    assert(headAndDescs != 0);
    /*
     * system settings
     */
    systemSection(headAndDescs);

    /*
     * next is the creation of device and profile initlists
     *  these lists consists of index into a stringtable (NOT the final value!)
     *  processing to actual values will be the last step
     */
    // devices
    dsc = (nxpTfaDescPtr_t *) headAndDescs->index;
    // create idx initlist with names
    for (i = 0; i < keys.n; i++) {
        dsc->type = dscDevice;
        dsc->offset = addString(keys.names[i]);
        dsc++;
    }
    dsc->type = dscMarker;    // mark end of devlist
    dsc->offset = 0xa5a5a5;    // easy to read
    dsc++;

    // create device initlist
    for (i = 0; i < keys.n; i++) {
        dev = (nxpTfaDeviceList_t *) dsc;    //1st device starts afted idx list
        currentSection = keys.names[i];
        dev->bus = (uint8_t) ini_getl(keys.names[i], "bus", 0, preformatIni);
        dev->dev = (uint8_t) ini_getl(keys.names[i], "dev", 0, preformatIni);
        dev->func =
            (uint8_t) ini_getl(keys.names[i], "func", 0, preformatIni);
        dsc++;
        // add the name
        dev->name.type = dscString;
        dev->name.offset = addString(currentSection);
        dsc++;
        // get the dev keys
        for (k = 0;
             ini_getkey(keys.names[i], k, key, sizearray(key),
                preformatIni) > 0; k++) {
            //printf("\t%d %s \n", k, key);
            dsc->type = parseKeyType(key);
            switch ((int)dsc->type) {
            case dscFile:    // filename + file contents
            case dscString:    // ascii: zero terminated string
            case dscProfile:
                if (ini_gets
                    (keys.names[i], key, "", value,
                     sizeof(value), preformatIni)) {
                    dsc->offset = addString(value);
                    dsc++;
                }
                break;
            case dscRegister:    // register patch e.g. $53=0x070,0x050
                strcpy(value, key);    //store value with key
                strcat(value, "=");
                if (ini_gets
                    (keys.names[i], key, "",
                     &value[strlen(key) + 1],
                     sizeof(value) - strlen(key) - 1,
                     preformatIni)) {
                    dsc->offset = addString(value);
                    dsc++;
                }
                break;
            case dscBitfieldBase:    // start of bitfield enums
                if (setBitfieldDsc
                    ((nxpTfaBitfield_t *) dsc, key,
                     (uint16_t) ini_getl(keys.names[i], key, 0,
                             preformatIni)) == 0)
                    dsc++;
                break;
//
//                      case dscProfile:        // profile skip, do duplicate key names with browse
//                                      dsc->offset = addString(value);
//                                      dsc++;
//                              break;
            case dscDevice:    // device list
                printf("error: skipping illegal key:%s\n", key);
                break;
            case -1:
                continue;
                break;
            default:
                //printf("skipping unknown key:%s\n", key);
                break;
            }
        }
        idx =
            (uint32_t *) dsc - (uint32_t *) (dev) -
            sizeof(nxpTfaDeviceList_t) / 4;
        dev->length = idx;    //store the total list length (inc name dsc)

//              // get the profiles names
        profiles.n = 0;
        ini_browse(findProfiles, &profiles, preformatIni);    //also counts entries
//              for(prof=0; prof<profiles.n; prof++) {
//                      dsc->type = dscProfile;
//                      dsc->offset[0] = addString(profiles.names[prof]);
//                      dsc++;
//              } /* device lists for loop end */

    }            /* device lists for loop end */
    /*
     * fix devices offset
     */
    dsc = (nxpTfaDescPtr_t *) headAndDescs->index;
    tfaContSetOffset(headAndDescs, dsc, keys.n + 1);    //+ marker first is after index

    for (i = 0; i < keys.n - 1; i++) {    // loop one less from total, 1st is done
        dev = tfaContGetDevList(headAndDescs, i);
        idx = dev->length;    // after this one
        idx++;        // skip marker
        dsc++;        // the next dev list
        tfaContSetOffset(headAndDescs, dsc,
                 idx + keys.n + sizeof(nxpTfaDeviceList_t) / 4);
    }
    /*
     * create profile initlists
     */
    dsc = (nxpTfaDescPtr_t *) tfaContGet1stProfList(headAndDescs);    // after the last dev list
    for (i = 0; i < profiles.n; i++) {
        profhead = (nxpTfaProfileList_t *) dsc;    //
        profhead->ID = 0x1234;
        dsc++;
        currentSection = profiles.names[i];
        // start with name
        profhead->name.type = dscString;
        profhead->name.offset = addString(currentSection);
        dsc++;
        // get the profile keys and process them
        for (k = 0;
             ini_getkey(profiles.names[i], k, key, sizearray(key),
                preformatIni) > 0; k++) {
            //printf("\t%s\n", key);
            dsc->type = parseKeyType(key);
            switch (dsc->type) {
            case dscFile:    // filename + file contents
            case dscString:    // ascii: zero terminated string
                if (ini_gets
                    (profiles.names[i], key, "", value,
                     sizeof(value), preformatIni)) {
                    dsc->offset = addString(value);
                    dsc++;
                }
                break;
            case dscRegister:    // register patch e.g. $53=0x070,0x050
                strcpy(value, key);    //store value with key
                strcat(value, "=");
                if (ini_gets
                    (profiles.names[i], key, "",
                     &value[strlen(key) + 1],
                     sizeof(value) - strlen(key) - 1,
                     preformatIni)) {
                    dsc->offset = addString(value);
                    dsc++;
                }
                break;
            case dscBitfieldBase:    // start of bitfield enums
                if (setBitfieldDsc
                    ((nxpTfaBitfield_t *) dsc, key,
                     (uint16_t) ini_getl(profiles.names[i], key,
                             0, inifile)) == 0)
                    dsc++;
                break;

            case dscProfile:    // profile
            case dscDevice:    // device list
                printf("error: skipping illegal key:%s\n", key);
                break;
            default:
                //printf("skipping unknown key:%s\n", key);
                break;
            }
        }

        idx = ((uint32_t *) dsc - (uint32_t *) (profhead));
        profhead->length = idx - 1;    //store the total list length (exc header, inc name dsc)

    }            /* device lists for loop end */

/*
 * all lists are parsed now so the offsets to profile lists can be filled in
 *  the device list has a dsc with the profile name, this need to become the byte
 *  offset of the profile list
 */
    tfaContFixProfOffsets(headAndDescs);

    headAndDescs->size = ((uint32_t *) dsc - (uint32_t *) (headAndDescs->index));    //total

    /*
     * remaining for offset fixing:
     *              2 dscRegister,  // register patch
     *              3 dscString,            // ascii, zero terminated string
     *              4 dscFile,              // filename + file contents
     */
    /*
     * create a big buffer to hold the entire container file
     */
    i = tfaContCreateContainer(headAndDescs, outFileName);
    free(headAndDescs);
    free(stringList);
    return i; // return size
}

