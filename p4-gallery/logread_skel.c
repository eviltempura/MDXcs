#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

struct Roster {
  char *name;
  struct Roster *next;
};

struct Room {
  int room_id;
  struct Roster *roster;
  struct Room *next;
};

int load_logs(char * alogs, struct Roster *emps, struct Roster *guests, struct Room *rooms) {
  char *token, *name;
  int is_emp, is_arr;
  int emp_flag = 1, guest_flag = 1, ans = 1;
  // struct Roster *ans_emps, *ans_guests;
  // struct Room *ans_rooms;

  token = strtok(alogs,",");
  while(token != NULL) {
    token = strtok(NULL,",");
    is_emp = atoi(token);
    token = strtok(NULL,",");
    is_arr = atoi(token);
    token = strtok(NULL,",");
    name = token;
    token = strtok(NULL,",");

    /*if the person is an employee*/
    if(is_emp) {
      /*if the person is the first employee to be logged*/
      if(emp_flag) {
        /*if it is an arrival*/
        if(is_arr) {
          printf("name1: %s\n",name);
          emp_flag = 0;
          // /*add the person to roster*/
          // emps->name = malloc(strlen(name));
          // strncpy(emps->name,name,strlen(name));
          // emps->next = NULL;
          // /*set emp_flag to 0*/
          // emp_flag = 0;
        /*if it is a departure*/
        } else {
          /*invalid since the person has to enter first*/
          printf("invalid\n");
          exit(255);
        }
      /*if the person is not the first employee to be logged*/
      } else {
        /*if it is an arrival*/
        if(is_arr) {
          printf("name2: %s\n",name);
          // /*add the person to roster*/
          // temp_roster = malloc(sizeof(struct Roster));
          // temp_roster->name = malloc(strlen(name));
          // strncpy(temp_roster->name,name,strlen(name));
          // insert_roster(emps,temp_roster);
        /*if it is a departure*/
        } else {
          printf("name3 :%s\n",name);
          // /*delete the person from roster*/
          // delete_roster(emps,name);
          // /*if roster becomes empty*/
          // if(emps->name == NULL && emps->next == NULL) {
          //   /*set emp_flag to 1*/
          //   emp_flag = 1;
          // }
        }
      }
    /*if the person is a guest*/
    } else {
      /*if the person is the first guest to be logged*/
      if(guest_flag) {
        /*if it is an arrival*/
        if(is_arr) {
          printf("name4: %s\n",name);
          guest_flag = 0;
          // guests->name = malloc(strlen(name));
          // strncpy(guests->name,name,strlen(name));
          // guests->next = NULL;
          // /*set guest_flag to 0*/
          // guest_flag = 0;
        /*if it is an departure*/
        } else {
          /*invalid since the person has to enter first*/
          printf("invalid\n");
          exit(255);
        }
      /*if the person is not the first guest to be logged*/
      } else {
        /*if it is an arrival*/
        if(is_arr) {
          printf("name5: %s\n",name);
          // /*add the person to roster*/
          // temp_roster = malloc(sizeof(struct Roster));
          // temp_roster->name = malloc(strlen(name));
          // strncpy(temp_roster->name,name,strlen(name));
          // insert_roster(guests,temp_roster);
        /*if it a departure*/
        } else {
          printf("name6: %s\n",name);
          // /*delete the person from roster*/
          // delete_roster(guests,name);
          // /*if roster becomes empty*/
          // if(guests->name == NULL && guests->next == NULL) {
          //   /*set emp_flag to 1*/
          //   guest_flag = 1;
          // }
        }
      }
    }

    /*tokenize for next loop*/
    token = strtok(NULL,",");
  }

  return ans;
}

int strcheck(char *str, int lc, int uc, int num, int path_chars) {
  int lc_ans = lc, uc_ans = uc, num_ans = num, path_chars_ans = path_chars;
  int i = 0;

  for(; i < strlen(str); i++) {
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


int main(int argc, char *argv[]) {

  ////////////////////////////////////////////////
  /////////////////variables//////////////////////
  ////////////////////////////////////////////////

  /*general purposese variables*/
  int opt,fsize;
  int sflag = 0, rflag = 0, is_emp = -999;
  char *logpath = NULL, *name = NULL;
  char *token, *input, *alogs;
  FILE *fp;

  /*openssl<hashing> variables*/
  EVP_MD_CTX *mdctx;
  unsigned char key[EVP_MAX_MD_SIZE]; //symmetric key
  unsigned int md_len;

  /*openssl<block cipher> variables*/
  EVP_CIPHER_CTX *ctx;
  int inlen,tmplen,success;
  unsigned char iv[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  unsigned char tag[32];

  /*rosters and rooms*/
  struct Roster e, g;
  struct Room r;
  struct Roster *emps = &e;
  struct Roster *guests = &g;
  struct Room *rooms = &r;

  ////////////////////////////////////////////////
  /////////////////variables//////////////////////
  ////////////////////////////////////////////////


  /*process arguments*/
  while ((opt = getopt(argc, argv, "K:PSRE:G:VT")) != -1) {
    switch(opt) {
      case 'T':
        printf("unimplemented\n");
        exit(255);

      case 'V':
        printf("unimplemented\n");
        exit(255);

      case 'P':
        printf("unimplemented\n");
        exit(255);

      case 'K':
        //secret token
        /*check if arg is alphanumeric*/
        if(strcheck(argv[optind-1],1,1,1,0)) {
          token = argv[optind-1];
        } else {
          printf("invalid\n");
          exit(255);
        }
        break;

      case 'S':
        sflag = 1;
        break;

      case 'R':
        rflag = 1;
        break;

      case 'E':
        is_emp = 1;
        name = argv[optind-1];
        break;

      case 'G':
        is_emp = 0;
        name = argv[optind-1];
        break;
    }
  }

  if(optind < argc) {
    logpath = argv[optind];
  }

  /*token and logpath must be provided*/
  if(token == NULL || logpath == NULL) {
    printf("invalid\n");
    exit(255);
  }

  /*-S and -R cannot be present at the same time*/
  if(sflag && rflag) {
    printf("invalid\n");
    exit(255);
  }

  /*Either -E or -G must be provided*/
  if(rflag == 1 && is_emp == -999) {
    printf("invalid\n");
    exit(255);
  }

  /*-R and name must be provided at the same time*/
  if(rflag == 1 && name == NULL) {
    printf("invalid\n");
    exit(255);
  }

  /*log must already exist*/
  if(access(logpath,F_OK) < 0) {
    printf("invalid\n");
    exit(255);
  }

  /*open the log*/
  fp = fopen(logpath, "r");
  if(fp == NULL) {
    printf("invalid\n");
    exit(255);
  }

  /*hash the token to generate symmetric key*/
  mdctx = EVP_MD_CTX_create();
  EVP_DigestInit_ex(mdctx,EVP_sha256(),NULL);
  EVP_DigestUpdate(mdctx,token,strlen(token));
  EVP_DigestFinal_ex(mdctx,key,&md_len);
  EVP_MD_CTX_destroy(mdctx);

  /*find the size of the file*/
  fseek(fp,0L,SEEK_END);
  fsize = ftell(fp);
  rewind(fp);
  /*allocate memory for buffer*/
  input = malloc(fsize);
  alogs = malloc(fsize);
  /*read the entire file into buffer*/
  fread(input,1,fsize-16,fp);
  fread(tag,1,16,fp);
  rewind(fp);

  /*null-terminate the tag*/
  tag[16] = '\0';

  /*initialize cipiher context*/
  ctx = EVP_CIPHER_CTX_new();

  EVP_DecryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,NULL,NULL);
  EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_IVLEN,sizeof(iv),NULL);
  EVP_DecryptInit_ex(ctx,NULL,NULL,key,iv);
  EVP_DecryptUpdate(ctx,(unsigned char *)alogs,&inlen,(unsigned char*)input,strlen(input));
  EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,16,tag);
  success = EVP_DecryptFinal_ex(ctx,(unsigned char*)alogs+inlen,&tmplen);
  EVP_CIPHER_CTX_free(ctx);

  if(success == 0) {
    printf("invalid\n");
    exit(255);
  }

  load_logs(alogs,emps,guests,rooms);

  EVP_cleanup();
  free(input);
  free(alogs);
  return 0;
}
