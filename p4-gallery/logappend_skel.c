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
  char *logpath;
  struct Log *next;
};

int append(char *outbuf, ) {
  
}

/*log is updated as following:
log->timestamp becomes that of the latest log regardless of if person is found
log->token is unchanged
log->is_emp gets updated only if the person is found
log->name gets updated only if the person is found
log->is_arr gets updated only if the person if found
log->int_room gets updated only if the person is found
log->logpath is unchanged*/
int find_person(char *alogs, struct Log *log, char *name) {
  char *token, *tmpname;
  int tmptimestamp, tmpis_emp, tmpis_arr, tmpint_room;
  int ans = 0;

  /*tokenize alogs*/
  token = strtok(alogs,"#");
  while(token != NULL) {
    token = strtok(token,",");
    tmptimestamp = atoi(token);
    token = strtok(NULL,",");
    tmpis_emp = atoi(token);
    token = strtok(NULL,",");
    tmpis_arr = atoi(token);
    token = strtok(NULL,",");
    tmpname = token;
    token = strtok(NULL,",");
    tmpint_room = atoi(token);

    /*update log accordingly*/
    log->timestamp = tmptimestamp;
    if(strcmp(tmpname,name)==0) {
      log->is_emp = tmpis_emp;
      log->is_arr = tmpis_arr;
      log->name = tmpname;
      log->int_room = tmpint_room;
      ans = 1;
    }

    /*tokenize for next loop*/
    token = strtok(NULL,"#");
  }

  return ans;
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
unsigned char *iv, int iv_len, unsigned char* ciphertext, unsigned char* tag) {
    int outlen,tmplen;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,NULL,NULL);
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_IVLEN,iv_len,NULL);
    EVP_EncryptInit_ex(ctx,NULL,NULL,key,iv);
    EVP_EncryptUpdate(ctx,ciphertext,&outlen,plaintext,plaintext_len);
    EVP_EncryptFinal_ex(ctx,ciphertext+outlen,&tmplen);
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_GET_TAG,16,tag);
    EVP_CIPHER_CTX_free(ctx);

    return outlen+tmplen;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
unsigned char *iv, int iv_len, unsigned char *plaintext, unsigned char *tag) {
    int inlen,tmplen,success;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    EVP_DecryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,NULL,NULL);
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_IVLEN,iv_len,NULL);
    EVP_DecryptInit_ex(ctx,NULL,NULL,key,iv);
    EVP_DecryptUpdate(ctx,plaintext,&inlen,ciphertext,ciphertext_len);
    EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,16,tag);
    success = EVP_DecryptFinal_ex(ctx,plaintext+inlen,&tmplen);
    EVP_CIPHER_CTX_free(ctx);

    return success;
}

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
  unsigned char iv[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  unsigned char *outbuf;
  unsigned char enc_tag[32];
  unsigned char dec_tag[32];

  /*initialize log for later arugments check*/
  struct Log log = {.timestamp = -999,
                    .token     = "@",
                    .is_emp    = -999,
                    .name      = "@",
                    .is_arr    = -999,
                    .int_room  = -1,
                    .logpath   = "@",
                    .next      = NULL};
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
  sprintf(itoa_buffer,"%d",log.int_room==-999?-1:log.int_room);
  strncat(output,itoa_buffer,strlen(itoa_buffer));

  /*if log is newly created, encrypt -> append -> exit*/
  if(init_token) {
    /*allocate memory for outbuf*/
    outbuf = malloc(strlen(output)+32);

    /*encrypt*/
    encrypt((unsigned char *)output,strlen(output),key,iv,sizeof(iv),outbuf,enc_tag);

    /*null-terminate tag*/
    enc_tag[16] = '\0';

    /*write the ciphertext*/
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
    alogs = malloc(fsize+1);
    /*read the entire file into buffer*/
    fread(input,1,fsize-16,fp);
    fread(dec_tag,1,16,fp);
    rewind(fp);

    /*null-terminate the tag*/
    dec_tag[16] = '\0';

    /*temporary log for further arguments check*/
    struct Log tmplog = {.timestamp = -999,
                         .token     = "@",
                         .is_emp    = -999,
                         .name      = "@",
                         .is_arr    = -999,
                         .int_room  = -999,
                         .logpath   = "@",
                         .next      = NULL};

    /*decrypt and store plaintext into algos*/
    if(decrypt((unsigned char *)input,strlen(input),key,iv,sizeof(iv),(unsigned char *) alogs,dec_tag)) {
      /*try find the person and update tmplog*/
      if(find_person(alogs,&tmplog,log.name)) {
        /*if the log file is empty*/
        if(log.timestamp == -999) {
          printf("invalid\n");
          exit(255);
        }

        /*check timestamp*/
        if(log.timestamp <= tmplog.timestamp) {
          printf("invalid\n");
          exit(255);
        }

        /*a person cannot be employee and guest at the same time*/
        if(log.is_emp != tmplog.is_emp) {
          printf("invalid\n");
          exit(255);
        }

        printf("Last: -T %d, %s, %s, %s, %d\n", tmplog.timestamp, tmplog.is_emp==1?"E":"G", tmplog.is_arr==1?"A":"L", tmplog.name, tmplog.int_room);
        printf("This: -T %d, %s, %s, %s, %d\n", log.timestamp, log.is_emp==1?"E":"G", log.is_arr==1?"A":"L", log.name, log.int_room);

        /*the person just left*/
        if(tmplog.is_arr == 0) {
          /*the person just left the gallery*/
          if(tmplog.int_room == -1) { 
            /*the person is trying to leave again*/
            if(log.is_arr == 0) {
              printf("invalid\n");
              exit(255);
            } else {
              /*the person is trying to enter a room*/
              if(log.int_room != -1) {
                printf("invalid\n");
                exit(255);
              /*the person is trying to enter the gallery*/
              } else {
                /*TODO: append log*/
              }
            }
          /*the person just left a room*/
          } else {
            /*the person is trying to leave a room again*/
            if(log.int_room != -1) {
              printf("invalid\n");
              exit(255);
            /*the person is trying to leave the gallery*/
            } else {
              /*TODO: append log*/
            }
          }
        /*the person just entered*/
        } else {
          /*the person just entered a room*/
          if(tmplog.int_room != -1) {
            /*the person is trying to enter again*/
            if(log.is_arr == 1) {
              printf("invalid\n");
              exit(255);
            /*the person is trying to exit*/
            } else {
              /*the person is trying to exit
              from a different room*/
              if(log.int_room != tmplog.int_room) {
                printf("invalid\n");
                exit(255);
              /*the person is trying exit
              from the current room*/
              } else {
                /*TODO: append log*/
              }
            }
          /*the person just entered the gallery*/
          } else {
            /*the person is trying to enter again*/
            if(log.is_arr == 1) {
              /*the person is trying to enter
              the gallery again*/
              if(log.int_room == -1) {
                printf("invalid\n");
                exit(255);
              /*the person is trying to enter a room*/
              } else {
                /*TODO: append log*/
              }
            /*the person is trying to leave*/
            } else {
              /*the perons is trying to leave from
              a room but he's in the gallery*/
              if(log.int_room != -1) {
                printf("invalid\n");
                exit(255);
              /*the person is trying to leave the gallery*/
              } else {
                /*TODO: append log*/
              }
            }
          }
        }
      } else {
        printf("Person not found\n");
      }
    } else {
      printf("invalid\n");
      exit(255);
    }
    

    /*release memory for input*/
    free(input);
  }

  EVP_cleanup();
  fclose(fp);
  return 0;
}
