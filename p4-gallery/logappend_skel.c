#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/evp.h>

struct Log{
  int timestamp;
  char *token;
  int is_emp;
  char *name;
  int is_arr;
  int int_room;
  char *char_room;
  char *logpath;
};

int strcheck(char *str, int lc, int uc, int num, int path_chars) {
  int lc_ans = lc, uc_ans = uc, num_ans = num, path_chars_ans = path_chars;

  for(int i = 0; i < strlen(str); i++) {
    /*check a-z*/
    if(str[i] >= 97 && str[i] <= 122) {
      lc_ans = lc_ans | 1;
    /*check A-Z*/
    } else if(str[i] >= 65 && str[i] <= 90) {
      uc_ans = uc_ans | 1;
    /*check 0-9*/
    } else if(str[i] >= 48 && str[i] <= 57) {
      num_ans = num_ans | 1;
    /*check '_', '.' and '/'*/
    } else if(str[i] == 95 || str[i] == 46 || str[i] == 47){
      path_chars_ans = path_chars_ans | 1;
    } else {
      return 0;
    }
  }

  return (lc == lc_ans) && (uc == uc_ans) && (num == num_ans) && (path_chars_ans == path_chars);
}

int parse_cmdline(int argc, char *argv[], struct Log *log) {

  int opt = -1;
  int is_good = 1;
  int is_emp = -1;
  int is_arr = -1;

  //pick up the switches
  /*is_good is set to 0 when arguments are invalid*/
  while ((opt = getopt(argc, argv, "T:K:E:G:ALR:B:")) != -1) {
    switch(opt) {
      case 'B':
        //batch file
        break;

      case 'T':
        //timestamp
        /*check if arg contains only 0-9*/
        if(strcheck(argv[optind-1],0,0,1,0)) {
          int timestamp = atoi(argv[optind-1]);
          /*check if timestamp within range*/
          if(timestamp < 1 || timestamp > 1073741823) {
            is_good = 0;
          } else {
            log->timestamp = timestamp;
          }
        } else {
          is_good = 0;
        }
        break;

      case 'K':
        //secret token
        /*check if arg is alphanumeric*/
        if(strcheck(argv[optind-1],1,1,1,0)) {
          log->token = argv[optind-1];
        } else {
          is_good = 0;
        }
        break;

      case 'A':
        //arrival
        /*set is_arr to 1*/
        if(is_arr == -1 || is_arr == 1) {
          is_arr = 1;
          log->is_arr = 1;
        } else {
          is_good = 0;
        }
        break;

      case 'L':
        //departure
        /*set is_arr to 0*/
        if(is_arr == -1 || is_arr == 0) {
          is_arr = 0;
          log->is_arr = 0;
        } else {
          is_good = 0;
        }
        break;

      case 'E':
        //employee name
        /*check if arg contains only letters*/
        if(strcheck(argv[optind-1],1,1,0,0)) {
          /*set is_emp to 1*/
          if(is_emp == -1 || is_emp == 1) {
            is_emp = 1;
            log->is_emp = 1;
            log->name = argv[optind-1];
          } else {
            is_good = 0;
          }
        } else {
          is_good = 0;
        }
        break;

      case 'G':
        //guest name
        /*check if arg contains only letters*/
        if(strcheck(argv[optind-1],1,1,0,0)) {
          /*set is_emp to 0*/
          if(is_emp == -1 || is_emp == 0) {
            is_emp = 0;
            log->is_emp = 0;
            log->name = argv[optind-1];
          } else {
            is_good = 0;
          }
        } else {
          is_good = 0;
        }
        break;

      case 'R':
        //room ID
        /*check if room id contains only 0-9*/
        if(strcheck(argv[optind-1],0,0,1,0)) {
          int int_room = atoi(argv[optind-1]);
          /*check if room id is within range*/
          if(int_room < 0 || int_room > 1073741823) {
            is_good = 0;
          } else {
            log->int_room = int_room;
          }
        } else {
          is_good = 0;
        }
        break;

      default:
        //unknown option, leave
        is_good = 0;
        break;
    }

  }


  //pick up the positional argument for log path
  if(optind < argc) {
    /*check arg format*/
    if(strcheck(argv[optind],1,1,1,1)) {
      log->logpath = argv[optind];
    } else {
      is_good = 0;
    }
  } else {
    is_good = 0;
  }

  return is_good;
}

void debug_print(struct Log log) {
  printf("Timestamp: %d, Token: %s\n", log.timestamp, log.token);
  printf("%s is a(n) %s ", log.name, log.is_emp==1?"employee":"guest");
  printf("who just %s room no.%d\n", log.is_arr==1?"arrived":"left",log.int_room);
  printf("Log has been appended to %s\n", log.logpath);
}

int main(int argc, char *argv[]) {

  ////////////////////////////////////////////////
  /////////////////variables//////////////////////
  ////////////////////////////////////////////////
  /*general purpose variables*/
  int result;
  int init_token = 0;
  char itoa_buffer[32];
  char *input, *output, *alogs;
  long fsize;
  FILE *fp;

  /*openssl<hashing> variables*/
  EVP_MD_CTX *mdctx;
  unsigned char key[EVP_MAX_MD_SIZE]; //symmetric key
  unsigned int md_len;

  /*openssl<block cipher> variables*/
  EVP_CIPHER_CTX *ctx;
  int inlen,outlen,tmplen;
  unsigned char iv[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  unsigned char *outbuf;
  unsigned char enc_tag[32] = {'\0'};
  unsigned char dec_tag[32] = {'\0'};

  /*initialize log for later arugments check*/
  struct Log log = {.timestamp = -999,
                    .token     = "@",
                    .is_emp    = -999,
                    .name      = "@",
                    .is_arr    = -999,
                    .int_room  = -999,
                    .char_room = "@",
                    .logpath   = "@"};
  ////////////////////////////////////////////////
  /////////////////variables//////////////////////
  ////////////////////////////////////////////////

  /*parse arguments*/
  result = parse_cmdline(argc, argv, &log);

  /*print invalid and exit with 255
  if argument chekc didn't pass*/
  if(result == 0) {
    printf("invalid\n");
    exit(255);
  }

  /*print invalid and exit with 255
  if some necessary arguments are missing
  optional argument: -B,-R*/
  if(log.timestamp == -999 || strcmp(log.token,"@") == 0
  || log.is_emp == -999    || strcmp(log.name,"@") == 0
  || log.is_arr == -999    || strcmp(log.logpath,"@") == 0) {
    printf("invalid\n");
    exit(255);
  }

  /*check if log already exists*/
  if(access(log.logpath,F_OK) < 0) {
    init_token = 1;
  }

  /*try openning the log*/
  fp = fopen(log.logpath, "a+");
  if(fp == NULL) {
    printf("invalid\n");
    exit(255);
  }

  /*hash the token to generate symmetric key*/
  mdctx = EVP_MD_CTX_create();
  EVP_DigestInit_ex(mdctx,EVP_sha256(),NULL);
  EVP_DigestUpdate(mdctx,log.token,strlen(log.token));
  EVP_DigestFinal_ex(mdctx,key,&md_len);
  EVP_MD_CTX_destroy(mdctx);

  /*format log*/
  output = malloc(sizeof(char)*strlen(log.name)+64);
  strncat(output,"#",1);
  sprintf(itoa_buffer,"%d",log.timestamp);
  strncat(output,itoa_buffer,strlen(itoa_buffer));
  strncat(output,",",1);
  strncat(output,log.is_emp==1?"1":"0",1);
  strncat(output,",",1);
  strncat(output,log.is_arr==1?"1":"0",1);
  strncat(output,",",1);
  strncat(output,log.name,strlen(log.name));
  strncat(output,",",1);
  sprintf(itoa_buffer,"%d",log.int_room);
  strncat(output,itoa_buffer,strlen(itoa_buffer));

  /*initialize context*/
  ctx = EVP_CIPHER_CTX_new();

  /*if log is newly created, encrypt -> append -> exit*/
  if(init_token) {
    /*allocate memory for outbuf*/
    outbuf = malloc(2*sizeof(char)*strlen(output));

    EVP_EncryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,key,iv);
    EVP_EncryptUpdate(ctx,outbuf,&outlen,(unsigned char*)output,strlen(output));
    EVP_EncryptFinal_ex(ctx,outbuf+outlen,&tmplen);
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_GET_TAG,16,enc_tag);

    for(int i = 0; i < strlen((char *)enc_tag); i++) {
      printf("%02x",enc_tag[i]);
    }
    printf("\n");

    EVP_CIPHER_CTX_free(ctx);
    fprintf(fp,"%s%s",outbuf,enc_tag);

    /*release memory for outbuf*/
    free(outbuf);

  /*if log already exists, decrypt -> encrypt -> append -> exit*/
  } else {
    /*find the size of the file*/
    fseek(fp,0L,SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    /*allocate memory for buffer*/
    input = malloc(fsize+1);
    alogs = malloc(2*(fsize+1));
    /*read the entire file into buffer*/
    fread(input,1,fsize-16,fp);
    fread(dec_tag,1,16,fp);

    for(int i = 0; i < strlen((char *)dec_tag); i++) {
      printf("%02x",dec_tag[i]);
    }
    printf("\n");

    EVP_DecryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,key,iv);
    EVP_DecryptUpdate(ctx,(unsigned char*)alogs,&inlen,(unsigned char*)input,strlen(input));
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,16,dec_tag);
    printf("Success: %d\n",EVP_DecryptFinal_ex(ctx,(unsigned char*)alogs+inlen,&tmplen));
    EVP_CIPHER_CTX_free(ctx);

    printf("%s\n",alogs);

    /*release memory for input*/
    free(input);
  }

  EVP_cleanup();
  fclose(fp);
  return 0;
}
