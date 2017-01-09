#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/delay.h>
#include <sys/socket.h>
#include <dlfcn.h>
//for test

unsigned char *keybox = "abcdefghijklmnopqrstuvwxyz123456abcdefgh12345678aaaaaaaaaabbbbbb"
"bbbbccccccccccddddddddddeeeeeeeeeeffffffffffgggggggggg12kboxcrcc";
                
      
#include "sprdoemcrypto.h"

int main(int argc, char *argv[]){

    int i = 0;
    unsigned char id[33] = {0};
    unsigned char kd[73] = {0};
    unsigned char rd[12] = {0};
    OEMCrypto_EncryptAndStoreKeyBox(keybox,128);
    
    if(OEMCrypto_IdentifyDevice(id, 32) == OEMCrypto_SUCCESS) {
        printf("id = %s\n",(char *)id);
    }

    OEMCrypto_GetkeyboxData(kd, 48, 72);

    printf("kd = %s\n", (char *) kd);

    OEMCrypto_GetRandom(rd, 10);

    for (i =0; i < 10; i++) printf("%d", rd[i]);
    return 0;
}
