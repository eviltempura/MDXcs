#define MSG_LEN 50000
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
#include <openssl/evp.h>

#include <regex.h>

void applyS(char*);
char** seperate_by_comma(char*);
char** get_emps_guests_rooms_in_gallary(char*);
char* get_room_persons(char** persons, int total_persons, char* roomid);
void applyR(char*, char*, char*);
void sort_lexicographically(char** persons, int total_persons);

void sort_rooms(char**);

void apply_time(char* name, char* person_type, char* msg_str);

void applyI(char*, char*, char*);
int is_listed(char**, char**, char*, char*);


int main(int argc, char *argv[]) {
  //printf("testing it works\n");
  int   opt,len, ret, total_len;
  int olen = 0;
  char  *logpath = NULL;
  size_t file_read;
  unsigned char token[100] = {'\0'};
  char *action = NULL;
  char *employee = NULL, *guest = NULL;

  char *iemps = NULL, *iguests = NULL;

  unsigned char *msg_str = calloc(MSG_LEN, 1);
  if (msg_str == NULL) {
    printf("Error mallocing1.\n");
    return 0;
  }
  unsigned char encrypted[MSG_LEN] = {'\0'};
  if (encrypted == NULL) {
    printf("Error mallocing2.\n");
    return 0;
  }

  unsigned char tag[17] = {'\0'};
  unsigned char iv[12] = {'\0'};
  EVP_CIPHER_CTX *ctx;




  while ((opt = getopt(argc, argv, "K:PSRE:G:VTI")) != -1) {
    switch(opt) {

      case 'I':
        action = "I";
        break;
      case 'T':
        action = "T";
        break;

      case 'V':
        printf("unimplemented");
        return 0;
        break;

      case 'P':
        printf("unimplemented");
        return 0;
        break;

      case 'K':
        strcpy(token, optarg);
        break;

      case 'S':
        if (action != NULL) {
          printf("invalid");
          return 255;
        }
        action = "S";
        break;

      case 'R':
        if (action != NULL) {
          printf("invalid");
          return 255;
        }
        action = "R";
        break;

      case 'E':
        if (action != NULL && strcmp(action, "I") == 0) {
          if (iemps == NULL) {
            iemps = malloc(strlen(optarg) + 1);
            iemps[0] = '\0';
            strcpy(iemps, optarg);
          } else {
            char *update_iemps = malloc(strlen(iemps) + strlen(optarg) + strlen(",") + 1);
            update_iemps[0] = '\0';
            strcat(update_iemps, iemps);
            strcat(update_iemps, ",");
            strcat(update_iemps, optarg);
            free(iemps);
            iemps = update_iemps;
          }
        } else {
          employee = optarg;
        }
        break;

      case 'G':
        if (action != NULL && strcmp(action, "I") == 0) {
          if (iguests == NULL) {
            iguests = malloc(strlen(optarg) + 1);
            iguests[0] = '\0';
            strcpy(iguests, optarg);
          } else {
            char *update_guests = malloc(strlen(iguests) + strlen(optarg) + strlen(",") + 1);
            update_guests[0] = '\0';
            strcat(update_guests, iguests);
            strcat(update_guests, ",");
            strcat(update_guests, optarg);
            free(iguests);
            iguests = update_guests;
          }
        } else {
          guest = optarg;
        }
        break;
    }
  }

  if(optind < argc) {
    logpath = argv[optind];
  }

  if (logpath == NULL) {
    printf("invalid");
    return 255;
  }
  FILE* log = fopen(logpath, "r");

  if (log == NULL || token == NULL || action == NULL) {
    printf("invalid");
    return 255;
  }
  int i;

  file_read = fread(encrypted, 1, sizeof(encrypted) - 1, log);
  
  for(i = 0; i < 16; i++){
    tag[i] = *(encrypted + file_read - 16 + i);
  }
  encrypted[file_read - 16] = '\0';
  
  ctx = EVP_CIPHER_CTX_new();
  EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
  
  EVP_DecryptInit_ex(ctx, NULL, NULL, token, iv);
  
  EVP_DecryptUpdate(ctx, msg_str, &olen, encrypted, file_read - 16);
  total_len = olen;
  
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
  
  ret = EVP_DecryptFinal_ex(ctx, msg_str + olen, &olen);
  
  msg_str[total_len] = '\0';
  
  EVP_CIPHER_CTX_free(ctx);


  if(ret > 0){
    total_len += olen;
  }else{
    printf("integrity violation");
    return 255;
  }
  
  fclose(log);


  /*unsigned char src[300] = {'\0'};
  while (fgets(src, 300, log) != NULL) {
    strcat(msg_str, src);
  }*/

  // FILE *saved_batch = fopen("saved_batch", "r");
  // if (saved_batch != NULL) {
  //   char buf[2000] = {'\0'};

  //   fread(buf, 1, sizeof(buf) - 1, saved_batch);
  //   fclose(saved_batch);

  //   printf("BATCH: %s\n", buf);
  //   return 0;
  // }
  
  if (strcmp(action, "I") == 0) {
    applyI(iguests, iemps, msg_str);
    free(iguests);
    free(iemps);
  } else if (strcmp(action, "S") == 0) {
    applyS(msg_str);
    //get_unique_rooms(msg_str);
  } else if (strcmp(action, "R") == 0) {
    if (employee != NULL && guest == NULL) {
      applyR(msg_str, employee, "E");
    } else if (guest != NULL && employee == NULL) {
      applyR(msg_str, guest, "G");
    } else {
      printf("invalid");
      return 255;
    }
  } else if (strcmp(action, "T") == 0) {
    if (employee != NULL && guest == NULL) {
      apply_time(employee, "E", msg_str);
    } else if (guest != NULL && employee == NULL) {
      apply_time(guest, "G", msg_str);
    } else {
      printf("invalid");
      return 255;
    }
  } else {
    printf("invalid");
    return 255;
  }

  free(msg_str);
  return 0;
}


void applyI(char* guests_str, char* emps_str, char* msg_str) {
  char *result = NULL;
  char ** guests_emps_rooms = get_emps_guests_rooms_in_gallary(msg_str);
  char *rooms_str = guests_emps_rooms[2];
  char *all_guests_str = guests_emps_rooms[0], *all_emps_str = guests_emps_rooms[1];
  char **guests = NULL, **emps = NULL, **rooms = NULL, **all_guests = NULL, **all_emps = NULL;
  int i, total_persons = 0;

  if (guests_str != NULL) {
    guests = seperate_by_comma(guests_str);
  }
  if (emps_str != NULL) {
    emps = seperate_by_comma(emps_str);
  }
  if (rooms_str != NULL) {
    rooms = seperate_by_comma(rooms_str);
  }

  if (all_guests_str != NULL) {
    all_guests = seperate_by_comma(all_guests_str);
  }

  if (all_emps_str != NULL) {
    all_emps = seperate_by_comma(all_emps_str);
  }


  for (i = 0; guests != NULL && guests[i] != NULL; i++) {
    total_persons++;
  }
  for (i = 0; emps != NULL && emps[i] != NULL; i++) {
    total_persons++;
  }

  for (i = 0; rooms != NULL && rooms[i] != NULL; i++) {
    char *target_room = rooms[i];
    int person_count = 0;
    int room_already_added = 0;
    unsigned char *msg_str2 = calloc(MSG_LEN, 1);
    if (msg_str2 == NULL) {
      printf("Error callocing.\n");
      exit(0);
    } 

    strcpy(msg_str2, msg_str);



    char *src_end;
    char *src = strtok_r(msg_str2, "\n", &src_end);
    while (src != NULL && room_already_added == 0) {
      regex_t re;
      regmatch_t groups[7];

      if (regcomp(&re, "^T: ([0-9]+) (A|L): (E|G) \"([A-Za-z]+)\"( R: ([0-9]+))?$", REG_EXTENDED)) {
        printf("Error compiling regex.");
        return;
      }
      if (regexec(&re, src, 7, groups, 0) == 0) {
        int abrupt_stop = 0;
        unsigned int g = 0;
        const char data_ret[7][100];
        for (g = 0; g < 7; g++) {
          if (groups[g].rm_so == (size_t)-1) {
            abrupt_stop = g;
            break;
          }
          char srcCpy[strlen(src) + 1];
          strcpy(srcCpy, src);
          srcCpy[groups[g].rm_eo] = 0;
          strcpy(data_ret[g], srcCpy+groups[g].rm_so);
        }

        if (abrupt_stop == 0) {
          char *curr_room = data_ret[6];
          char *name = data_ret[4];
          char *person_type = data_ret[3];
          char *enter_exit = data_ret[2];
          int is_listed_res = is_listed(guests, emps, name, person_type);

          //printf("%s\n", src);
          //printf("Target_room: %s, Curr_Room: %s, name: %s, persontype: %s, enter_exit: %s, is_listed: %d\n", target_room, curr_room, name, person_type, enter_exit, is_listed_res);

          if (strcmp(enter_exit, "A") == 0 && is_listed_res == 0 && strcmp(curr_room, target_room) == 0) {
            person_count++;
            if (person_count == total_persons) {
              room_already_added = 1;
              if (result == NULL) {
                result = malloc(strlen(target_room) + 1);
                result[0] = '\0';
                strcpy(result, target_room);
              } else {
                char *update_result = malloc(strlen(target_room) + strlen(result) + 2);
                update_result[0] = '\0';
                strcat(update_result, result);
                strcat(update_result, ",");
                strcat(update_result, target_room);
                free(result);
                result = update_result;
              }
            }
          } else if (strcmp(enter_exit, "L") && is_listed_res == 0 && strcmp(curr_room, target_room) == 0) {
            person_count--;
          }
        }
      }
      src = strtok_r(NULL, "\n", &src_end);
    }
    free(msg_str2);
  }

  if (result != NULL) {
    char **result_sep = seperate_by_comma(result);
    char *result_str = malloc(strlen(result) + 1);
    result_str[0] = '\0';
    sort_rooms(result_sep);

    for (i = 0; result_sep != NULL && result_sep[i] != NULL; i++) {
      strcat(result_str, result_sep[i]);
      strcat(result_str, ",");
    }

    result_str[strlen(result_str)-1] = '\0';
    printf("%s\n", result_str);
  }

}

int is_listed(char** guests, char** emps, char* name, char* type) {
  int i;
  if (strcmp(type, "E") == 0) {
    for (i=0; emps != NULL && emps[i] != NULL; i++) {
      if (strcmp(name, emps[i]) == 0) {
        return 0;
      }
    }
  } else if (strcmp(type, "G") == 0) {
    for (i=0; guests != NULL && guests[i] != NULL; i++) {
      if (strcmp(name, guests[i]) == 0) {
        return 0;
      }
    }
  }
  // not found
  return 1;
}


void applyS(char* msg_str) {
  unsigned char *msg_str2 = calloc(MSG_LEN, 1);
   if (msg_str2 == NULL) {
    printf("Error mallocing3.\n");
    exit(0);
  }
  char ** guests_emps_rooms = get_emps_guests_rooms_in_gallary(msg_str);
  //rewind(log);
  char *emps_str = guests_emps_rooms[1], *guests_str = guests_emps_rooms[0], *rooms_str = guests_emps_rooms[2];
  char **rooms = seperate_by_comma(rooms_str);
  char **guests = seperate_by_comma(guests_str);
  char **emps = seperate_by_comma(emps_str);
  int total_persons = 0;
  int i;

  if (emps_str != NULL) {
      printf("%s\n", emps_str);
  }
  if (guests_str != NULL) {
      printf("%s\n", guests_str);
  }

  //printf("%s\n", rooms_str);

  for (i = 0; guests != NULL && guests[i] != NULL; i++) {
    total_persons++;
  }
  for (i=0; emps != NULL && emps[i] != NULL; i++) {
    total_persons++;
  }

  // no work to do here since no rooms
  if (rooms == NULL) 
    return;

  sort_rooms(rooms);

  for (i = 0; rooms[i] != NULL; i++) {
    char **persons = calloc(total_persons, sizeof(char*));
    if (persons == NULL) {
      printf("Error callocing..\n");
      exit(0);
    }
    strcpy(msg_str2, msg_str);
    char *src_end;
    char *src = strtok_r(msg_str2, "\n", &src_end);

    while (src) {
      regex_t re;
      regmatch_t groups[7];

      if (regcomp(&re, "^T: ([0-9]+) (A|L): (E|G) \"([A-Za-z]+)\"( R: ([0-9]+))?$", REG_EXTENDED)) {
        printf("Error compiling regex.");
        return;
      } 
      //src[strlen(src)-1] = '\0';
      int abrupt_stop = 0;
      const char data_ret[7][100];
      if (regexec(&re, src, 7, groups, 0) == 0) {
        unsigned int g = 0;
        for (g = 0; g < 7; g++) {
          if (groups[g].rm_so == (size_t)-1) {
            abrupt_stop = g;
            break;
          }
          char srcCpy[strlen(src) + 1];
          strcpy(srcCpy, src);
          srcCpy[groups[g].rm_eo] = 0;
          strcpy(data_ret[g], srcCpy+groups[g].rm_so);
        }
      }
      // making sure theres a room in this line and its this one
      if (abrupt_stop == 0 && strcmp(data_ret[6], rooms[i]) == 0) {
        char *leave_arrive = data_ret[2];
        char *name = data_ret[4];
        if (strcmp(leave_arrive, "A") == 0) {
          //printf("inserting: %s into %s\n", name, rooms[i]);
          insert_person(persons, name, total_persons);
        } else if (strcmp(leave_arrive, "L") == 0) {
          //printf("removing: %s into %s\n", name, rooms[i]);
          remove_person(persons, name, total_persons);
        }
      }
      src = strtok_r(NULL, "\n", &src_end);
    }



    char* rooms_persons = get_room_persons(persons, total_persons, rooms[i]);
    if (rooms_persons != NULL) {
      printf("%s\n", rooms_persons);
    }
  }
  free(msg_str2);
}

// parameters: file log, person name, type(G or E)
void applyR(char *msg_str, char* person, char* type) {
  char *result = NULL;
  unsigned char *msg_str2 = calloc(MSG_LEN, 1);
  if (msg_str2 == NULL) {
    printf("Error mallocing1.\n");
    exit(0);
  }
  strcpy(msg_str2, msg_str);
  char *src_end;
  char *src = strtok_r(msg_str2, "\n", &src_end);

  while (src) {
    regex_t re;
    regmatch_t groups[7];

    if (regcomp(&re, "^T: ([0-9]+) (A|L): (E|G) \"([A-Za-z]+)\"( R: ([0-9]+))?$", REG_EXTENDED)) {
      printf("Error compiling regex.");
      return;
    } 
    //src[strlen(src)-1] = '\0';

    if (regexec(&re, src, 7, groups, 0) == 0) {
      int abrupt_stop = 0;
      unsigned int g = 0;
      const char data_ret[7][100];
      for (g = 0; g < 7; g++) {
        if (groups[g].rm_so == (size_t)-1) {
          abrupt_stop = g;
          break;
        }
        char srcCpy[strlen(src) + 1];
        strcpy(srcCpy, src);
        srcCpy[groups[g].rm_eo] = 0;
        strcpy(data_ret[g], srcCpy+groups[g].rm_so);
      }

      //add
      // line contains room id and is entering and the user specified is the given user
      if (abrupt_stop == 0 && strcmp(data_ret[2], "A") == 0 && strcmp(data_ret[4], person) == 0) {
        char *room = data_ret[6];
        if (result == NULL) {
          result = malloc(strlen(room) + 1);
          if (result == NULL) {
            printf("Error mallocing result\n");
            exit(0);
          }
          result[0] = '\0';
          strcpy(result, room);
        } else {
          char *new_str = malloc(strlen(result) + strlen(room) + 2);
          if (new_str == NULL) {
            printf("Error mallocing new_str\n");
            exit(0);
          }
          new_str[0] = '\0';
          strcpy(new_str, result);
          strcat(new_str, ",");
          strcat(new_str, room);
          free(result);
          result = new_str;
        }
      }
    }
    src = strtok_r(NULL, "\n", &src_end);
  }


  if (result != NULL) {
    printf("%s\n", result);
  }
  free(msg_str2);
}






char** get_emps_guests_rooms_in_gallary(char *msg_str) {
  char **guests_arr = malloc(1000*sizeof(char*)), **emps_arr = malloc(1000*sizeof(char*));
  char *guests = NULL;
  char *emps = NULL;
  char *rooms = NULL;
  unsigned char *msg_str2 = calloc(MSG_LEN, 1);
  if (msg_str2 == NULL) {
    printf("Error mallocing1.\n");
    exit(0);
  }
  if (guests_arr == NULL) {
    printf("Error mallocing1.\n");
    exit(0);
  }
  if (emps_arr == NULL) {
    printf("Error mallocing1.\n");
    exit(0);
  }
  strcpy(msg_str2, msg_str);
  char *end_str;
  char *src = strtok_r(msg_str2, "\n", &end_str);

  while (src != NULL) {
    //printf("%s\n", src);
    regex_t re;
    regmatch_t groups[7];

    if (regcomp(&re, "^T: ([0-9]+) (A|L): (E|G) \"([A-Za-z]+)\"( R: ([0-9]+))?$", REG_EXTENDED)) {
      printf("Error compiling regex.");
      return -1;
    } 

    //src[strlen(src)-1] = '\0';
    if (regexec(&re, src, 7, groups, 0) == 0) {
      int abrupt_stop = 0;
      unsigned int g = 0;
      const char data_ret[7][100];
      for (g = 0; g < 7; g++) {
        if (groups[g].rm_so == (size_t)-1) {
          abrupt_stop = g;
          //break;
        }
        char srcCpy[strlen(src) + 1];
        strcpy(srcCpy, src);
        srcCpy[groups[g].rm_eo] = 0;
        strcpy(data_ret[g], srcCpy+groups[g].rm_so);
      }


      char *name = data_ret[4];

      // theres a room id
      if (abrupt_stop == 0) {
        char *room = data_ret[6];
        char **rooms_arr = seperate_by_comma(rooms);
        int k = 0, found = 0;
        for (k = 0; rooms != NULL && rooms_arr[k] != NULL; k++) {
          if (strcmp(rooms_arr[k], room) == 0) {
            found = 1;
          }
        }

        if (found == 0) {
          if (rooms == NULL) {
            rooms = malloc(strlen(room) + 1);
            if (rooms == NULL) {
              printf("error malloc\n");
              exit(0);
            }
            rooms[0] = '\0';
            strcpy(rooms, room);
          } else {
            char *new_str = malloc(strlen(rooms) + strlen(room) + 2);
            if (rooms == NULL) {
              printf("error malloc\n");
              exit(0);
            }
            new_str[0] = '\0';// apparently makes sure memory is empty
            strcpy(new_str, rooms);
            strcat(new_str, ",");
            strcat(new_str, room);

            free(rooms);
            rooms = new_str;
          }
        }
      }

      //printf("%s\n", src);
      if (strcmp(data_ret[3], "G") == 0 && abrupt_stop != 0) {
        // person is arriving to gallary
        if (strcmp(data_ret[2], "A") == 0) {
          insert_person(guests_arr, data_ret[4], 1000);
          //printf("inserting guest %s\n", data_ret[4]);
          // person leaving gallary
        } else if (strcmp(data_ret[2], "L") == 0) {
          remove_person(guests_arr, data_ret[4], 1000);
          //printf("removing guest %s\n", data_ret[4]);
        }
      } else if (strcmp(data_ret[3], "E") == 0 && abrupt_stop != 0) {
        if (strcmp(data_ret[2], "A") == 0) {
          insert_person(emps_arr, data_ret[4], 1000);
          //printf("inserting emp %s\n", data_ret[4]);
          // person leaving gallary
        } else if (strcmp(data_ret[2], "L") == 0) {
          remove_person(emps_arr, data_ret[4], 1000);
          //printf("removing emp %s\n", data_ret[4]);
        }
      }
    }
    src = strtok_r(NULL, "\n", &end_str);
  }

  sort_lexicographically(guests_arr, 1000);
  sort_lexicographically(emps_arr, 1000);

  int i;
  for (i = 0; i < 1000; i++) {
    if (guests_arr[i] != NULL) {
      if (guests == NULL) {
          guests = malloc(strlen(guests_arr[i]) + 1);
          if (guests == NULL) {
              printf("error malloc\n");
              exit(0);
            }
          guests[0] = '\0';
          strcpy(guests, guests_arr[i]);
        } else {
          char *new_str = malloc(strlen(guests) + strlen(guests_arr[i]) + 2);
          if (new_str == NULL) {
              printf("error malloc\n");
              exit(0);
            }
          new_str[0] = '\0';// apparently makes sure memory is empty
          strcpy(new_str, guests);
          strcat(new_str, ",");
          strcat(new_str, guests_arr[i]);

          free(guests);
          guests = new_str;
        }
    }

    if (emps_arr[i] != NULL) {
      if (emps == NULL) {
          emps = malloc(strlen(emps_arr[i]) + 1);
          if (emps == NULL) {
              printf("error malloc\n");
              exit(0);
            }
          emps[0] = '\0';
          strcat(emps, emps_arr[i]);
        } else {
          char *new_str = malloc(strlen(emps) + strlen(emps_arr[i]) + 2);
          if (new_str == NULL) {
              printf("error malloc\n");
              exit(0);
            }
          new_str[0] = '\0';// apparently makes sure memory is empty
          strcpy(new_str, emps);
          strcat(new_str, ",");
          strcat(new_str, emps_arr[i]);

          free(emps);
          emps = new_str;
        }
    }
  }

  static char *result[3];
  
  if (guests != NULL) {
    result[0] = malloc(strlen(guests) + 1);
    if (result[0] == NULL) {
      printf("error malloc\n");
      exit(0);
    }
    result[0][0] = '\0';
    strcpy(result[0], guests);
  }
    
  if (emps != NULL) {
    result[1] = malloc(strlen(emps) + 1);
    if (result[1] == NULL) {
      printf("error malloc\n");
      exit(0);
    }
    result[1][0] = '\0';
    strcpy(result[1], emps);
  }
  if (rooms != NULL) {
    result[2] = malloc(strlen(rooms) + 1);
    if (result[2] == NULL) {
      printf("error malloc\n");
      exit(0);
    }
    result[2][0] = '\0';
    strcpy(result[2], rooms);
  }

  free(msg_str2);
  free(guests_arr);
  free(emps_arr);
  return result;
}

char** seperate_by_comma(char* data) {
  if (data == NULL) return NULL;

  char *data_copy = malloc(strlen(data)+1);
  if (data_copy == NULL) {
      printf("error malloc\n");
      exit(0);
  }
  data_copy[0] = '\0';
  strcpy(data_copy, data);
  const char sep[2] = ",";
  char *token, *token_end;
  int count = 0, i = 0;
  token = strtok_r(data_copy, sep, &token_end);
  // get count
  while (token != NULL) {
    count++;
    token = strtok_r(NULL, sep, &token_end);
  }

  // allocate memory and store
  char **array = malloc(sizeof(char*)*(count+1));
  if (array == NULL) {
    printf("Error allocating memory.");
    return NULL;
  }

  token = data_copy;

  for (i = 0; i < count; i++) {
    array[i] = (char *) malloc(strlen(token) + 1);
    if (array[i] == NULL) {
      printf("error malloc\n");
      exit(0);
    }
    array[i][0] = '\0';
    strcpy(array[i], token);
    token += strlen(token) + 1;
    token += strspn(token, sep);
  }
  // end of array
  array[count] = NULL;

  return array;
}


void remove_person(char **persons, char* person, int total_persons) {
  int i = 0;
  for (i = 0; i < total_persons; i++){
    if (persons[i] == NULL)
      continue;
    if (strcmp(persons[i], person) == 0) {
      free(persons[i]);
      persons[i] = NULL;
      break;
    }
  }
}

void insert_person(char** persons, char* person, int total_persons) {
  int i = 0;
  for (i = 0; i < total_persons; i++) {
    if (persons[i] == NULL) {
      persons[i] = malloc(strlen(person) + 1);
      if (persons[i] == NULL) {
      printf("error malloc\n");
      exit(0);
    }
      persons[i][0] = '\0';
      strcpy(persons[i], person);
      break;
    }
  }
}

void sort_lexicographically(char** persons, int total_persons) {
  int i, j;
    // sort lexicographically
  for (i = 0; i < total_persons-1; i++) {
    for (j = i +1; j < total_persons; j++) {
      if (persons[i] == NULL || persons[j] == NULL)
        continue;
      if (strcmp(persons[i], persons[j]) > 0) {
        char *temp = malloc(strlen(persons[i]) + 1);
        if (temp == NULL) {
          printf("erro maloc");
          exit(0);
        }
        temp[0] = '\0';
        strcpy(temp, persons[i]);
        //free(persons[i]);
        persons[i] = malloc(strlen(persons[j]) + 1);
        if (persons[i] == NULL) {
          printf("erro maloc");
          exit(0);
        }
        persons[i][0] = '\0';
        strcpy(persons[i], persons[j]);
        //free(persons[j]);
        persons[j] = malloc(strlen(temp) + 1);
        if (persons[j] == NULL) {
          printf("erro maloc");
          exit(0);
        }
        persons[j][0] = '\0';
        strcpy(persons[j], temp);
        //free(temp);
      }
    }
  }
}

char* get_room_persons(char** persons, int total_persons, char* roomid) {
  int i, j;
  char *result = NULL;

  // sort lexicographically
  for (i = 0; i < total_persons-1; i++) {
    for (j = i +1; j < total_persons; j++) {
      if (persons[i] == NULL || persons[j] == NULL)
        continue;
      if (strcmp(persons[i], persons[j]) > 0) {
        char *temp = malloc(strlen(persons[i]) + 1);
        if (temp == NULL) {
          printf("erro maloc");
          exit(0);
        }
        temp[0] = '\0';
        strcpy(temp, persons[i]);
        free(persons[i]);
        persons[i] = malloc(strlen(persons[j]) + 1);
        if (persons[i] == NULL) {
          printf("erro maloc");
          exit(0);
        }
        persons[i][0] = '\0';
        strcpy(persons[i], persons[j]);
        free(persons[j]);
        persons[j] = malloc(strlen(temp) + 1);
        if (persons[j] == NULL) {
          printf("erro maloc");
          exit(0);
        }
        persons[j][0] = '\0';
        strcpy(persons[j], temp);
        free(temp);
      }
    }
  }


  for (i = 0; i < total_persons; i++) {
    if (persons[i] != NULL) {
      if (result == NULL) {
        result = malloc(strlen(persons[i]) + 1);
        if (result == NULL) {
          printf("erro maloc");
          exit(0);
        }
        result[0] = '\0';
        strcpy(result, persons[i]);
      } else {
        char *new_str = malloc(strlen(persons[i]) + strlen(result) + 2);
        if (new_str == NULL) {
          printf("erro maloc");
          exit(0);
        }
        new_str[0] = '\0';
        strcpy(new_str, result);
        strcat(new_str, ",");
        strcat(new_str, persons[i]);
        free(result);
        result = new_str;
      }
    }
  }

  if (result == NULL) {
    return NULL;
  } else {
    char *new_str = malloc(strlen(roomid) + strlen(": ") + strlen(result) + 1);
    if (new_str == NULL) {
          printf("erro maloc");
          exit(0);
        }
    new_str[0] = '\0';
    strcpy(new_str, roomid);
    strcat(new_str, ": ");
    strcat(new_str, result);
    free(result);
    return new_str;
  }
}

void sort_rooms(char** rooms) {
  int total_rooms = 0, i, j;
  for (i = 0; rooms[i] != NULL; i++) {
    total_rooms++;
  }

  for (i = 0; i < total_rooms-1; i++) {
    for (j = i+1; j < total_rooms; j++) {
      if (atoi(rooms[i]) - atoi(rooms[j]) > 0) {
        char *temp = malloc(strlen(rooms[i]) + 1);
        if (temp == NULL) {
          printf("erro maloc");
          exit(0);
        }
        temp[0] = '\0';
        strcpy(temp, rooms[i]);
        free(rooms[i]);
        rooms[i] = malloc(strlen(rooms[j]) + 1);
        if (rooms[i] == NULL) {
          printf("erro maloc");
          exit(0);
        }
        rooms[i][0] = '\0';
        strcpy(rooms[i], rooms[j]);
        free(rooms[j]);
        rooms[j] = malloc(strlen(temp) + 1);
        if (rooms[j] == NULL) {
          printf("erro maloc");
          exit(0);
        }
        rooms[j][0] = '\0';
        strcpy(rooms[j], temp);
        free(temp);
      }
    }
  }
}


void apply_time(char* name, char* person_type, char* msg_str) {
  int total_time = 0;
  int curr_start = -1, last_time_seen = -1;
  char *msg_str2 = calloc(MSG_LEN, 1);
   if (msg_str2 == NULL) {
    printf("Error mallocing100.\n");
    exit(0);
  }
  strcpy(msg_str2, msg_str);
  char *src_end;
  char *src = strtok_r(msg_str2, "\n", &src_end);
  while (src != NULL) {
    regex_t re;
    regmatch_t groups[7];

    if (regcomp(&re, "^T: ([0-9]+) (A|L): (E|G) \"([A-Za-z]+)\"( R: ([0-9]+))?$", REG_EXTENDED)) {
      printf("Error compiling regex.");
      return;
    }
    if (regexec(&re, src, 7, groups, 0) == 0) {
      int abrupt_stop = 0;
      unsigned int g = 0;
      const char data_ret[7][100];
      for (g = 0; g < 7; g++) {
        if (groups[g].rm_so == (size_t)-1) {
          abrupt_stop = g;
          break;
        }
        char srcCpy[strlen(src) + 1];
        strcpy(srcCpy, src);
        srcCpy[groups[g].rm_eo] = 0;
        strcpy(data_ret[g], srcCpy+groups[g].rm_so);
      }

      // doesnt have room id and 
    if (abrupt_stop != 0 && strcmp(data_ret[4], name) == 0 && strcmp(data_ret[3], person_type) == 0) {
        if (strcmp(data_ret[2], "A") == 0) {
          curr_start = atoi(data_ret[1]);
        } else if (strcmp(data_ret[2], "L") == 0) {
          int end_time = atoi(data_ret[1]);
          total_time += (end_time - curr_start);
          curr_start = -1; // reset start
        }
      }

      last_time_seen = atoi(data_ret[1]);
    }
    src = strtok_r(NULL, "\n", &src_end);
  }


  if (curr_start != -1) {
    total_time += (last_time_seen - curr_start);
  }
  //printf("%s\n", msg_str);
  printf("%d", total_time);
}

