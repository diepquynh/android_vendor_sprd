1. key.db 
   Store the rsa keys.

2. RSAKeyGen.exe
   Generate rsa key pair, key pair will be stored in the key.db

   usage:
   RSAKeyGen.exe -pw <"password"> -pn <"product-name">

   -pw <"password">      --password must be 8 ASCII.
   -pn <"product-name">  --not more than 49 chars


2. VLRSign.exe
   Sign image file.

   usage:
   VLRSign.exe -img <"vlr-image-file"> -out <"signed-file"> -pw <"password"> -pn <"product-name"> [-slen signed-length] [-plen padded-leng
th] [-ipbk [true|false]] [-ha [sha1|md5]]

   -img <"vlr-image-file">     --input vlr-image file.
   -out <"signed-file">        --output signed image file
   -pw <"password">            --password must be 8 ASCII.
   -pn <"product-name">        --not more than 49 chars
   -slen signed-length         --signed length of the vlr-image, default the whole image is signed.  
   -plen padded-length         --pad the vlr-image
   -ipbk [true|false]          --if true, insert RSA public key,default is true
			         if not set, it's value depend on -pw2 and -pn2 if not set these two param, it is false, otherwise it is true.
   -ha [sha1|md5]              --hash algorithm, sha1 or md5, default is sha1
   -pw2 <"password">           --password must be 8 ASCII.
   -pn2 <"product-name">       --not more than 49 chars


3. BscGen.exe
   Generate BSC file.
   
   usage:
   BscGen.exe -img <"code-image-file"> -out <"bsc-file"> -pw <"password"> -pn <"product-name"> -pw <"password"> -pn <"product-name"> [-ha [sha1|md5]]

   -img <"code-image-file">  --input code-image file.
   -out <"bsc-file">         --output bsc image file
   -pw <"password">          --password must be 8 ASCII.
   -pn <"product-name">      --not more than 49 chars
   -pw2 <"password">         --password must be 8 ASCII.
   -pn2 <"product-name">     --not more than 49 chars
   -ha [sha1|md5]            --hash algorithm, sha1 or md5, default is sha1
