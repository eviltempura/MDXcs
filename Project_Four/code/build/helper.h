#ifndef _HELPER_H_
#define _HELPER_H_
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <wordexp.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <ctype.h>
#define  do_encrypt 1

typedef struct LogArgs LogArgs;
struct LogArgs {
    char name[256];
    int64_t time;
    int8_t isGuest;
    int8_t isEnter;
    int8_t isLeave;
    int64_t roomID;
};


int exists(const char *fname)
{
    FILE *tempfile;
    
    if ((tempfile = fopen(fname, "r")))
    {
        fclose(tempfile);
        return 1;
    }
    return 0;
}
int write_key(char * filename,char * str,unsigned char* userKey)
{
   
    EVP_CIPHER_CTX ctx;
    FILE *cipher_fp;
    cipher_fp = fopen(filename,"w+");
    int ret,num;
    if (cipher_fp == NULL)
    {
        printf("invalid\n");
        return 255;
    }

    unsigned char iv[16];
    memset(iv, 0x0, 16);
    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit_ex(&ctx, EVP_aes_128_cbc(), NULL, userKey, iv);


        //unsigned char in[1024 * 10] = { 0 }, out[1024 * 10] = { 0 };
        num=strlen(str);

        //num++;
      //  printf("max is %d\n",EVP_MAX_BLOCK_LENGTH );
      
       
        unsigned char *in= calloc(1024 + EVP_MAX_BLOCK_LENGTH * num +1,sizeof(unsigned char));
      


        int temp_len = 0;
        int out_len = 0;
        ///////////////////////
        strncpy(in,str,strlen(str));


        // int num=strlen(in)+1;
      

        //int num = fread(in, sizeof(char), 1024 * 1000, plain_fp);
        //////////////////////
        int count = num / AES_BLOCK_SIZE;
          unsigned char *out= calloc(1024 + EVP_MAX_BLOCK_LENGTH * num +1 ,sizeof(unsigned char));

        if (count * AES_BLOCK_SIZE < num)
        {
            int start = num;
            count += 1;
            for (; start < count * AES_BLOCK_SIZE; start++)
            {
                in[start] = '\0';
            }

        }

        ret = EVP_EncryptUpdate(&ctx, out, &temp_len, in, count * AES_BLOCK_SIZE);

        if (ret != 1)
        {
            printf("invalid\n");
            fclose(cipher_fp);

            EVP_CIPHER_CTX_cleanup(&ctx);
            return 255;
        }
        out_len += temp_len;

        ret = EVP_EncryptFinal_ex(&ctx, out + out_len, &temp_len);
        out_len += temp_len;


        fwrite(out, sizeof(char), out_len, cipher_fp);

    
    EVP_CIPHER_CTX_cleanup(&ctx);
    if(in!=NULL){
        free(in);
    }
    if(out!=NULL){
        free(out);
    }
     fclose(cipher_fp);

    return 0;
}

char* decrypy(FILE *fp, const char* key) {


    unsigned char* token =key;

    unsigned char iv[16] = {0};
    FILE* f = fp;
    unsigned char in[1024], out[1024 + EVP_MAX_BLOCK_LENGTH];

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, token, iv, 0);
    int suminlen = 0;
    char *result = NULL;
    int outlen;
    while (1) {
        int inlen = fread(in, 1, 1024, f);
        if (inlen <= 0) {
            break;
        }
        if (!EVP_CipherUpdate(ctx, out, &outlen, in, inlen)) {
            EVP_CIPHER_CTX_free(ctx);

            return NULL;
        }
        result = (char*)realloc(result, suminlen + outlen);
        strncpy(result + suminlen, out, outlen);
        suminlen += outlen;
    }

    if(!EVP_CipherFinal_ex(ctx, out, &outlen)) {
       
        EVP_CIPHER_CTX_free(ctx);
        // printf("wrong\n");
        return NULL;
    }
    result = (char*)realloc(result, suminlen + outlen);
    strncpy(result + suminlen, out, outlen);
    EVP_CIPHER_CTX_free(ctx);
    //free(token);
    return result;
}
void sha256(char *string, char outputBuffer[65])
{

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}
int create_log(char *timestamp,unsigned char * userKey, char *eorg, char * name, char* aorl, char *room, char *filename)
{

    if(aorl[1]=='A' && room[0]=='\0')
    {

    }
    else
    {
        printf("%s\n","invalid" );
        return(255);
    }

    char tempstr[2000];
    char finalstr[2000];
    char sha[257];
    memset(tempstr,0x0,sizeof(tempstr));
    memset(finalstr,0x0,sizeof(finalstr));
    memset(sha,0x0,sizeof(sha));
    strcat(tempstr,"-T ");
    strcat(tempstr,timestamp);
    strcat(tempstr," -A ");
    strcat(tempstr,eorg);
    strcat(tempstr," ");
    strcat(tempstr,name);
    strcat(tempstr,"\n");
    sha256(tempstr,sha);
    strcpy(finalstr,sha);
    strcat(finalstr,"\n");
    strcat(finalstr,tempstr);
    // printf("%s\n", finalstr);
   int res= write_key(filename,finalstr,userKey);
   return res;
}

LogArgs opt_parser(int32_t argc, char** argv) {
    int32_t c;

    LogArgs arg;
    arg.isGuest = 0;
    arg.roomID = -1;
    arg.isEnter = 0;
    arg.isLeave = 0;
    arg.time = 0;
    optind = 1;

    while ((c = getopt(argc, argv, "R:T:AILE:G:")) != -1) {
        switch (c) {
        case 'R':
            arg.roomID = atoi(optarg);
            break;
        case 'T':
        //printf("in T\n");
            arg.time = atoi(optarg);
            break;
        case 'A':
            arg.isEnter = 1;
            break;
        case 'E':
            strcpy(arg.name, optarg);
            break;
        case 'G':
            strcpy(arg.name, optarg);
            arg.isGuest = 1;
            break;
        case 'L':
            arg.isLeave = 1;
            break;
        }
    }

    return arg;
}

int split_line(char*** lines) {
   // char* line = strtok(result, "\n");
   char *line = strtok(NULL, "\n");
   int count = 0;
   while (line != NULL) {
       (*lines) = (char**)realloc(*lines, (count + 1) * sizeof(char*));
       (*lines)[count] = (char*)malloc(strlen(line)+1);
       strcpy((*lines)[count], line);
       count += 1;
       line = strtok(NULL, "\n");
   }
   return count;
}




int isOuside(char** lines,int count,char * name,char *room,char *eorg,char* timestamp)
{

    for (int i = count-1; i >=0 ; i--)
    {
        int32_t argc = 1;
        char** argv = NULL;
        char* r = strtok(lines[i], " ");
        while (r != NULL)
        {
            argv = (char**) realloc(argv, (argc + 1) * sizeof(char*));
            argv[argc] = (char*) malloc(strlen(r)+1);
            strcpy(argv[argc], r);
            r = strtok(NULL, " ");
            argc += 1;
        }
        argv[0] = (char*) malloc(3);
        strcpy(argv[0], "la");
        LogArgs arg = opt_parser(argc, argv);
        for(int i = 0; i < argc; i++)
        {
            free(argv[i]);
        }
        free(argv);
        LogArgs* temp_arg=&arg;
        if(i==count-1){
             long temp_time=atol(timestamp);
            if(temp_time<=temp_arg->time){
            return 0;
            }
        }

        if(strcmp(name,temp_arg->name)==0){
            if(temp_arg->isLeave==1 && temp_arg->roomID==-1){
                if(eorg[1]=='E' && temp_arg->isGuest){
                    return 0;
                }
                if(eorg[1]=='G' && temp_arg->isGuest==0){
                    return 0;
                }
                return 1;               
            }
            else{
                return 0;
            }

        }
        
    }

    return 1;
}

int isInside(char** lines,int count,char * name,char *room,char *eorg,char* timestamp){

    for (int i = count-1; i >=0 ; i--)
    {
        int32_t argc = 1;
        char** argv = NULL;
        char* r = strtok(lines[i], " ");
        while (r != NULL)
        {
            argv = (char**) realloc(argv, (argc + 1) * sizeof(char*));
            argv[argc] = (char*) malloc(strlen(r)+1);
            strcpy(argv[argc], r);
            r = strtok(NULL, " ");
            argc += 1;
        }
        argv[0] = (char*) malloc(3);
        strcpy(argv[0], "la");
        LogArgs arg = opt_parser(argc, argv);
        for(int i = 0; i < argc; i++)
        {
            free(argv[i]);
        }
        free(argv);
        LogArgs* temp_arg=&arg;
        if(i==count-1){
             long temp_time=atol(timestamp);
            if(temp_time<=temp_arg->time){
            return 0;
            }
        }

        if(strcmp(name,temp_arg->name)==0){

            if(   (temp_arg->isLeave==1 && temp_arg->roomID!=-1) || (temp_arg->roomID==-1  && temp_arg->isEnter)    ){
                if(eorg[1]=='E' && temp_arg->isGuest){
                    return 0;
                }
                if(eorg[1]=='G' && temp_arg->isGuest==0){
                    return 0;
                }
                return 1;               
            }
            else{
                return 0;
            }
        }
          
    }

    return 0;

}

int isInroom(char** lines,int count,char * name,char *room,char *eorg,char* timestamp){

    for (int i = count-1; i >=0 ; i--)
    {
        int32_t argc = 1;
        char** argv = NULL;
        char* r = strtok(lines[i], " ");
        while (r != NULL)
        {
            argv = (char**) realloc(argv, (argc + 1) * sizeof(char*));
            argv[argc] = (char*) malloc(strlen(r)+1);
            strcpy(argv[argc], r);
            r = strtok(NULL, " ");
            argc += 1;
        }
        argv[0] = (char*) malloc(3);
        strcpy(argv[0], "la");
        LogArgs arg = opt_parser(argc, argv);
        for(int i = 0; i < argc; i++)
        {
            free(argv[i]);
        }
        free(argv);
        LogArgs* temp_arg=&arg;
        if(i==count-1){
             long temp_time=atol(timestamp);
            if(temp_time<=temp_arg->time){
            return 0;
            }
        }

        if(strcmp(name,temp_arg->name)==0){
             if(eorg[1]=='E' && temp_arg->isGuest){
                    return 0;
                }
                if(eorg[1]=='G' && temp_arg->isGuest==0){
                    return 0;
                }

            if(temp_arg->roomID!=-1  && temp_arg->isEnter){
                long temp_room=atol(room);
                if(temp_room==temp_arg->roomID){
                    return 1;
                }
                else{
                    return 0;
                }
               
                              
            }
            else{
                return 0;
            }
        }
          
    }

    return 0;

}

int append_log(char *timestamp,char * userKey, char *eorg, char * name, char* aorl, char *room, FILE * file,char *filename)
{
   
    char *str=decrypy(file, userKey);
   
       //printf("strlen=  %d\n str=%s\n",strlen(str),str );
    if(str==NULL){
        printf("invalid\n");
        return 255;
    } char *new_str=calloc(strlen(str)+10,sizeof(char));
        strcpy(new_str,str);

     char *hash=strtok(new_str,"\n");
    fclose(file);

     char sha[257];
     char first[257];
    memset(sha,0x0,sizeof(sha));
     memset(first,0x0,sizeof(first));
    //  for(int i=0;i<strlen(str);i++){
    //     printf("%d\n",str[i] );
    // }
    // return 0;
   //printf("decrypted\n%s",str);
    // return 0;

    
    // if (str!=NULL){
    //     free(str);
    // }
  //  char str[3000];
    char *str2=calloc(strlen(str)+500,sizeof(char));
     char *finalstr=calloc(strlen(str)+500,sizeof(char));
    char *tempstr=calloc(strlen(str)+500,sizeof(char));
   
    //memset(str,0x0,sizeof(str));
    //memset(str2,0x0,sizeof(str2));
   
    // FILE *fp;
    // fp=fopen("plain.txt","r");
    // fread(str,sizeof(char),sizeof(str),fp);
     //printf("str\n%s", str);
     strcpy(str2,str+65);
    // printf("strlen=  %d\n str=%s\n",strlen(str),str );
     strncpy(first,str,64);
     sha256(str2,sha);
     if(strcmp(first,sha)!=0){
        printf("%s\n", "invalid");
        free(str);
        free(new_str);

    free(str2);
    free(tempstr);
    free(finalstr);

        return 255;
     }






     char** lines = NULL;
    int count = split_line(&lines);
    free(str);
     free(new_str);
    int flag=0;
    /* char* left = result + strlen(hash) + 1; */
    /* printf("%s\n\n", hash); */
    /* printf("%s", left); */
    /* unsigned char* my_hash = sha256(left); */
    /* printf("%s\n", tohex(my_hash)); */
    /* if (strcmp(tohex(my_hash), hash) == 0) { */
        /* printf("---\n"); */
    /* } */
    int i = 0;
    // for (i = 0; i < count; i++) {
    //   process_line(lines[i]);
    // }

    if(aorl[1]=='A' && room[0]=='\0'){ //enter from outside
        int res;
      res=isOuside(lines,count,name,room,eorg,timestamp);
      if(res){
        //printf("isOuside %s\n","YES" );
        flag=1;
      }
      else{
       // printf("isOuside %s\n","NO" );
      }
    }
    else if(aorl[1]=='A' && room[0]!='\0'){// enter a room
         int res;
      res=isInside(lines,count,name,room,eorg,timestamp);
      if(res){
       // printf("isInside %s\n","YES" );
        flag=1;
      }
      else{
      //  printf("isInside %s\n","NO" );
      }

    }
    else if (aorl[1]=='L' && room[0]=='\0'){//leave from inside
        int res;
      res=isInside(lines,count,name,room,eorg,timestamp);
      if(res){
      //  printf("isLeave %s\n","YES" );
        flag=1;
      }
      else{
       // printf("isLeave %s\n","NO" );
      }

    }
    else if (aorl[1]=='L' && room[0]!='\0'){//leave a room
         int res;
      res=isInroom(lines,count,name,room,eorg,timestamp);
      if(res){
       // printf("isInroom %s\n","YES" );
        flag=1;
      }
      else{
       // printf("isInroom %s\n","NO" );
      }

   
    }

    // if(checkTime(lines,count,timestamp)==0){
    //     flag=0;
    // }
    int resss;
    for (i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);







     if(flag){
    strcat(tempstr,"-T ");
    strcat(tempstr,timestamp);
    strcat(tempstr," ");
    strcat(tempstr,aorl);
    strcat(tempstr," ");
    strcat(tempstr,eorg);
    strcat(tempstr," ");
    strcat(tempstr,name);
    if(room[0]!='\0'){
        strcat(tempstr," -R ");
          strcat(tempstr,room);
    }
    strcat(tempstr,"\n");
    // for(int i=0;i<strlen(str2);i++){
    //     printf("%d\n",str2[i] );
    // }
   strcat(str2,tempstr);

    //printf("%s", str2);
    memset(sha,0x0,sizeof(sha));
     sha256(str2,sha);
     strcpy(finalstr,sha);
     strcat(finalstr,"\n");
     strcat(finalstr,str2);
    // printf("final str\n%s", finalstr);
     // printf("final\n%s",finalstr);
     // printf("filename \n%s\n",filename);

      resss=write_key(filename,finalstr,userKey);
   
   
   // printf("str2\n%s", str2);


    }
    else{
        printf("invalid\n");
    free(str2);
    free(tempstr);
    free(finalstr);

        return 255;
    }

    free(str2);
    free(tempstr);
    free(finalstr);
    return 0;

}

#endif


