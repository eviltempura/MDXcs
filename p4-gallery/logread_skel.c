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
  int is_emp;
  char *name;
  struct Roster *previous;
  struct Roster *next;
};

struct Room {
  int room_id;
  struct Roster *roster;
  struct Room *previous;
  struct Room *next;
};

void print_roster(struct Roster *l1, int flag);
void print_room(struct Room *l1);

int search_roster(struct Roster *l1, int is_emp, char *name) {
  if(l1 == NULL) {
    return 0;
  }
  if(l1->name == NULL) {
    return 0;
  }
  if(strcmp(l1->name,name) == 0 && l1->is_emp == is_emp) {
    return 1;
  } else {
    return search_roster(l1->next,is_emp,name);
  }
}

int search_room(struct Room *l1, int room_id) {
  if(l1 == NULL) {
    return 0;
  }
  if(l1->room_id == room_id) {
    return 1;
  } else {
    return search_room(l1->next,room_id);
  }
}

struct Roster *insert_roster(struct Roster *head, struct Roster *l2) {
  struct Roster *l1 = head;

  while(l1 != NULL) {
    if(strcmp(l1->name,l2->name) >= 0) {
      if(l1->previous == NULL) {
        l1->previous = l2;
        l2->next = l1;
        return l2;
      } else {
        l1->previous->next = l2;
        l2->previous = l1->previous;
        l1->previous = l2;
        l2->next = l1;
        return head;
      }
    } else {
      if(l1->next != NULL) {
        l1 = l1->next;
      } else {
        l1->next = l2;
        l2->previous = l1;
        return head;
      }
    }
  }

  return head;
}

struct Room *insert_room(struct Room *head, struct Room *l2) {
  struct Room *l1 = head;
  while(l1 != NULL) {
    if(l1->room_id >= l2->room_id) {
      if(l1->previous == NULL) {
        l1->previous = l2;
        l2->next = l1;
        return l2;
      } else {
        l1->previous->next = l2;
        l2->previous = l1->previous;
        l1->previous = l2;
        l2->next = l1;
        return head;
      }
    } else {
      if(l1->next != NULL) {
        l1 = l1->next;
      } else {
        l1->next = l2;
        l2->previous = l1;
        return head;
      }
    }
  }

  return head;
}

struct Roster *delete_roster(struct Roster *head, int is_emp, char *name) {
  struct Roster *l1 = head;
  struct Roster *l2;
  int comparison;

  while(l1 != NULL) {
    comparison = strcmp(l1->name,name);
    if(comparison > 0) {
      return head;
    } else if(comparison == 0 && l1->is_emp == is_emp) {
      if(l1->previous == NULL) {
        l2 = l1->next;
        if(l2 == NULL) {
          return NULL;
        } else {
          l1->next = NULL;
          l2->previous = NULL;
          return l2;
        }
      } else {
        l1->previous->next = l1->next;
        if(l1->next != NULL) {
          l1->next->previous = l1->previous;
        }
        l1->previous = NULL;
        l1->next = NULL;
      }
    }
    l1 = l1->next;
  }

  return head;
}

struct Room *delete_room(struct Room *head, int room_id) {
  struct Room *l1 = head;
  struct Room *l2;
  while(l1 != NULL) {
    if(l1->room_id > room_id) {
      return head;
    } else if(l1->room_id == room_id) {
      if(l1->previous == NULL) {
        l2 = l1->next;
        if(l2 == NULL) {
          return NULL;
        } else {
          l1->next = NULL;
          l2->previous = NULL;
          return l2;
        }
      } else {
        l1->previous->next = l1->next;
        if(l1->next != NULL) {
          l1->next->previous = l1->previous;
        }
        l1->previous = NULL;
        l1->next = NULL;
      }
    }
    l1 = l1->next;
  }

  return head;
}

struct Room *get_room(struct Room *l1, int room_id) {
  if(l1 == NULL) {
    return NULL;
  }
  if(l1->room_id < room_id) {
    return get_room(l1->next, room_id);
  }
  if(l1->room_id == room_id) {
    return l1;
  }
  return NULL;
}

void print_roster(struct Roster *l1, int flag) {
  if(l1 == NULL) {
    return;
  }
  if(l1->name == NULL) {
    return;
  }
  if(!flag) {
    printf(",%s",l1->name);
  } else {
    printf("%s",l1->name);
  }

  print_roster(l1->next,flag&&0);
}

void print_room(struct Room *l1) {
  if(l1 == NULL) {
    return;
  }
  printf("%d: ",l1->room_id);
  print_roster(l1->roster,1);
  printf("\n");
  print_room(l1->next);
}

int load_logs(char * alogs, struct Roster **emps, struct Roster **guests, struct Room **rooms) {
  char *token, *name;
  int is_emp, is_arr, room_id;
  int emp_flag = 1, guest_flag = 1, ans = 1;
  struct Roster *temp_roster;
  struct Room *temp_room, *temp_room2;

  token = strtok(alogs,",");
  while(token != NULL) {
    /*timestamp = atoi(token);*/
    token = strtok(NULL,",");
    is_emp = atoi(token);
    token = strtok(NULL,",");
    is_arr = atoi(token);
    token = strtok(NULL,",");
    name = token;
    token = strtok(NULL,",");
    room_id = atoi(token);

    /*create a roster to be added*/
    temp_roster = malloc(sizeof(struct Roster));
    temp_roster->name = malloc(strlen(name));
    strncpy(temp_roster->name,name,strlen(name));
    temp_roster->previous = NULL;
    temp_roster->next = NULL;

    /*create a room to be added*/
    temp_room = malloc(sizeof(struct Room));
    temp_room->room_id = room_id;
    temp_room->roster = NULL;
    temp_room->previous = NULL;
    temp_room->next = NULL;

    /*if the person is an employee*/
    if(is_emp) {
      /*set temp_roster to employee*/
      temp_roster->is_emp = 1;
      /*if the person is the first employee to be logged*/
      if(emp_flag) {
        /*if it is an arrival*/
        if(is_arr) {
          /*add the person to roster*/
          (*emps)->name = malloc(strlen(name));
          strncpy((*emps)->name,name,strlen(name));
          (*emps)->is_emp = 1;
          (*emps)->previous = NULL;
          (*emps)->next = NULL;

          /*if room not found*/
          if(!search_room((*rooms),room_id)) {
            /*if no room exists*/
            if((*rooms)->room_id == -999) {
              (*rooms)->room_id = room_id;
              (*rooms)->roster = temp_roster;
              (*rooms)->previous = NULL;
              (*rooms)->next = NULL;
            /*if room does not match*/
            } else {
              /*add person into roster*/
              temp_room->roster = temp_roster;
              (*rooms) = insert_room((*rooms),temp_room);
            }
          /*if room found*/
          } else {
            temp_room = get_room((*rooms),room_id);
            temp_room->roster = insert_roster(temp_room->roster,temp_roster);
          }

          /*set emp_flag to 0*/
          emp_flag = 0;
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
          /*add if the person is not in the gallery*/
          if(!search_roster((*emps),is_emp,name)) {
            (*emps) = insert_roster((*emps),temp_roster);

            /*create another roster for room roster*/
            temp_roster = malloc(sizeof(struct Roster));
            temp_roster->name = malloc(strlen(name));
            strncpy(temp_roster->name,name,strlen(name));
            temp_roster->is_emp = is_emp;
            temp_roster->previous = NULL;
            temp_roster->next = NULL;
          }

          /*if room not found*/
          if(!search_room((*rooms),room_id)) {
            if(room_id != -1) {
              temp_room2 = get_room((*rooms),-1);
              temp_room2->roster = delete_roster(temp_room2->roster,is_emp,name);
              if(temp_room2->roster == NULL) {
                (*rooms) = delete_room((*rooms),-1);
              }
            }
            temp_room->roster = temp_roster;
            if((*rooms) == NULL) {
              (*rooms) = temp_room;
            } else {
              (*rooms) = insert_room((*rooms),temp_room);
            }
          /*if room found*/
          } else {
            if(room_id != -1) {
              temp_room2 = get_room((*rooms),-1);
              temp_room2->roster = delete_roster(temp_room2->roster,is_emp,name);
              if(temp_room2->roster == NULL) {
                (*rooms) = delete_room((*rooms),-1);
              }
            }
            temp_room = get_room((*rooms),room_id);
            temp_room->roster = insert_roster(temp_room->roster,temp_roster);
          }

        /*if it is a departure*/
        } else {
          /*delete if the person left the gallery*/
          if(room_id == -1) {
            (*emps) = delete_roster((*emps),is_emp,name);
          }

          temp_room = get_room((*rooms),room_id);
          temp_room->roster = delete_roster(temp_room->roster,is_emp,name);
          if(temp_room->roster == NULL) {
            (*rooms) = delete_room((*rooms),room_id);
            if(room_id != -1) {
              temp_room = get_room((*rooms),-1);
              temp_room->roster = insert_roster(temp_room->roster,temp_roster);
            }
          }

          /*if roster becomes empty*/
          if((*emps) == NULL) {
            /*set emp_flag to 1*/
            emp_flag = 1;
          }
        }
      }
    /*if the person is a guest*/
    } else {
      /*set temp_roster to guest*/
      temp_roster->is_emp = 0;
      /*if the person is the first guest to be logged*/
      if(guest_flag) {
        /*if it is an arrival*/
        if(is_arr) {
          (*guests)->name = malloc(strlen(name));
          strncpy((*guests)->name,name,strlen(name));
          (*guests)->is_emp = 0;
          (*guests)->previous = NULL;
          (*guests)->next = NULL;
          /*if room not found*/
          if(!search_room((*rooms),room_id)) {
            /*if no room exists*/
            if((*rooms)->room_id == -999) {
              (*rooms)->room_id = room_id;
              (*rooms)->roster = temp_roster;
              (*rooms)->previous = NULL;
              (*rooms)->next = NULL;
            /*if room does not match*/
            } else {
              temp_room->roster = temp_roster;
              (*rooms) = insert_room((*rooms),temp_room);
            }
          /*if room found*/
          } else {
            temp_room = get_room((*rooms),room_id);
            temp_room->roster = insert_roster(temp_room->roster,temp_roster);
          }

          /*set guest_flag to 0*/
          guest_flag = 0;
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
          /*add if the person is not already in the gallery*/
          if(!search_roster((*guests),is_emp,name)) {
            (*guests) = insert_roster((*guests),temp_roster);

            /*create a new roster to room roster*/
            temp_roster = malloc(sizeof(struct Roster));
            temp_roster->name = malloc(strlen(name));
            strncpy(temp_roster->name,name,strlen(name));
            temp_roster->is_emp = is_emp;
            temp_roster->previous = NULL;
            temp_roster->next = NULL;
          }

          /*if room not found*/
          if(!search_room((*rooms),room_id)) {
            if(room_id != -1) {
              temp_room2 = get_room((*rooms),-1);
              temp_room2->roster = delete_roster(temp_room2->roster,is_emp,name);
              if(temp_room2->roster == NULL) {
                (*rooms) = delete_room((*rooms),-1);
              }
            }

            temp_room->roster = temp_roster;
            if((*rooms) == NULL) {
              (*rooms) = temp_room;
            } else {
              (*rooms) = insert_room((*rooms),temp_room);
            }
          /*if room found*/
          } else {
            if(room_id != -1) {
              temp_room2 = get_room((*rooms),-1);
              temp_room2->roster = delete_roster(temp_room2->roster,is_emp,name);
              if(temp_room2->roster == NULL) {
                (*rooms) = delete_room((*rooms),-1);
              }
            }
            temp_room = get_room((*rooms),room_id);
            temp_room->roster = insert_roster(temp_room->roster,temp_roster);
          }
        /*if it is a departure*/
        } else {
          /*delete if the person left the gallery*/
          if(room_id == -1) {
            (*guests) = delete_roster((*guests),is_emp,name);
          }

          temp_room = get_room((*rooms),room_id);
          temp_room->roster = delete_roster(temp_room->roster,is_emp,name);
          if(temp_room->roster == NULL) {
            (*rooms) = delete_room((*rooms),room_id);
            if(room_id != -1) {
              temp_room = get_room((*rooms),-1);
              temp_room->roster = insert_roster(temp_room->roster,temp_roster);
            }
          }

          /*if roster becomes empty*/
          if((*guests) == NULL) {
            /*set emp_flag to 1*/
            guest_flag = 1;
          }
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
  unsigned char *iv = (unsigned char *)"0123456789012345";
  unsigned char tag[32];

  /*rosters and rooms*/
  struct Roster e, g;
  struct Room r = {.room_id = -999,
                   .roster = NULL,
                   .previous = NULL,
                   .next = NULL};
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

  /*null-terminate the key*/
  key[32] = '0';

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

  load_logs(alogs,&emps,&guests,&rooms);

  if(sflag) {
    print_roster(emps,1);
    printf("\n");
    print_roster(guests,1);
    printf("\n");
    print_room(rooms);
  }

  if(rflag) {

  }

  EVP_cleanup();
  free(input);
  free(alogs);
  return 0;
}
