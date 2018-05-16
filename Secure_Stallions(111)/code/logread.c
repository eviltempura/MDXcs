#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <stdint.h>
#include <ctype.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#define KEY_LEN 32
#define IV_LEN 16

typedef struct{
  char **names ;
  char *curr_num;
  int num_names;
  int act_num_names;
} room;

typedef struct{
  room *rooms;
  char **employees;
  char **guests;
  int **room_list;
  int num_rooms;
  int num_employees;
  int num_guests;
  int act_num_guests;
  int act_num_employees;
} gallery;

static int sort(const void *a, const void *b){
  return strcmp(* (const char **) a, *(const char **) b);
}

static int sortNums(const void *a, const void *b){
  if (&a > &b){
    return -1;
  }else if(&a < &b){
    return 1;
  }else {
    return 0;
  }
}

void handleErrors() {
  printf("invalid\n");
  exit(255);
}

void integrityViolation() {
  printf("integrity violation\n");
  exit(255);
}

int decrypt(EVP_CIPHER_CTX *ctx, unsigned char *ciphertext,
                      int ciphertext_len, unsigned char *key,
                      unsigned char *iv, unsigned char *plaintext) {
    int len;
    int pltxt_len;
    int ret;

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
      integrityViolation();
    }

    if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
      integrityViolation();  
    }
    pltxt_len = len;

    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    if (ret > 0) {
      pltxt_len += len;
      return pltxt_len;
    } else {
      return -1;
    }
}

int main(int argc, char *argv[]) {
  EVP_CIPHER_CTX *ctx;
  ctx = EVP_CIPHER_CTX_new();
  int   opt;
  char  *path = NULL;
  int i; //For any iterations
  size_t len;
  int S = 0, R = 0, E = 0, G = 0;
  unsigned char iv[IV_LEN]; //can't be bigger
  unsigned char key[KEY_LEN];//store the value of the key they use to access the file
 
  memset(iv, 0, IV_LEN);
  memset(key, 0, KEY_LEN); 

  unsigned char plaintext[EVP_MAX_MD_SIZE]; //BUFFER FOR ANY PLAINTEXT GOTTEN FROM LOG
  memset(plaintext, 0, EVP_MAX_MD_SIZE);
  unsigned int plaintext_len; //Populated when decrypting to know how much plaintext to read!

  unsigned char ciphertext[EVP_MAX_MD_SIZE]; //BUFFER FOR ANY CIPHERTEXT TO BE WRITTEN TO FILE
  memset(ciphertext, 0, EVP_MAX_MD_SIZE);
  unsigned int ciphertext_len; //Populated when encrypting to know how much ciphertext to read!

  unsigned char *name = NULL;
  unsigned char *real_key = NULL; //Not cut down to 32, still checks authenticity


  while ((opt = getopt(argc, argv, "K:SRE:G:T")) != -1) {
    switch(opt) {
      case 'T':
        printf("unimplemented\n" );
        break;

      case 'K':
        free(real_key);
        real_key = NULL;
        
        //go through each character of the key being passed in, if its not a alphabetic char or its not a number then the key is invalid.
        //if it is valid then store that value in the key
        
        len = strlen(optarg);
        real_key = malloc(len +1);

       for(i = 0; i < KEY_LEN; i++){
        if (i < len) {
          if(!isdigit(optarg[i]) && !isalpha(optarg[i])) { //if non-numeric and non-alphabetic then fail can be one or the other
            handleErrors();
          }
          if (i < IV_LEN) {
            iv[i] = optarg[i];
          }
          key[i] = optarg[i];
          real_key[i] = optarg[i];
        } else { //Pad spaces up to appropriate lengths
          if (i < IV_LEN) {
            iv[i] = ' ';
          }
          key[i] = ' ';
        }


      } 
      for (; i < len; i++) {
        if (!isdigit(optarg[i]) && !isalpha(optarg[i])) {
          handleErrors();
        }
        real_key[i] = optarg[i];
      }
      real_key[i] = '\0';

      break;

      case 'S':
        //can only have S or R
        if(R){
          handleErrors();
        }
        S = 1;
        break;

      case 'R':
        //can only have S or R
        if(S){
          handleErrors();
        }
        R = 1;
        break;

      case 'E':
        if (G) {
          handleErrors();
        }
        E = 1;
        
        free(name);
        name = NULL;

        len = strlen(optarg);
        name = malloc(len + 1);
        for(i = 0; i < len; i++){
          if(!isalpha(optarg[i])){
            handleErrors();
          }
          name[i] = optarg[i];
        }
        name[i] = '\0';
      break;

      case 'G':
        if (E) {
          handleErrors();
        }
        G = 1;
        
        free(name);
        name = NULL;

        len = strlen(optarg);
        name = malloc(len + 1);
        for(i = 0; i < len; i++){
          if(!isalpha(optarg[i])){
            handleErrors();
          }
          name[i] = optarg[i];
        }
        name[i] = '\0';
        break;
    }
  }

  //pick up the positional argument for log path
  if(optind == argc - 1) { //Good case, log file provided
    len = strlen(argv[optind]) + 1;
    path = malloc(len);
    for(i = 0; i < len; i++){
      //checks to make sure each char of the log filename is valid
      if(!isdigit(argv[optind][i]) && !isalpha(argv[optind][i]) && argv[optind][i] != '.' && argv[optind][i] != '/' && argv[optind][i] != '_' && argv[optind][i] != '\0'){
        handleErrors();
      }
      path[i] = argv[optind][i];
    }
    path[i] = '\0';
  }

    //decrypt file to read
    FILE *fp;
    if ((fp = fopen((char *) path, "r")) == NULL) {
      handleErrors();
    }
    //First, we'll need to decrypt 
    //Read from fp into ciphertext buffer
    //Then populate plaintext buffer via
    //decrypt method
    i = 0;
    while (fread(ciphertext + i, 1, 1, fp)) {
      i++;
    }
    ciphertext_len = i;
    //Ciphertext now populated

    int dummy;
    if ((dummy = decrypt(ctx, ciphertext, 
        ciphertext_len, key, iv, plaintext))
            == -1) {
      integrityViolation(); //INVALID KEY OR IV
    }
    plaintext_len = (unsigned int) dummy;

    //now we have plaintext and length populated
    int read; //keeps track of how much many params were read from sscanf
    int start = 0; //keeps track of where we are in the plaintext
    char *buff = NULL; //used to copy each line of plaintext into so it can be read
    char curr_time[11]; //get the time of the current line
    char curr_name[512]; //not sure how big name should be
    char typeOfPerson[2]; //type of person either E or G
    char arriveOrLeave[2]; // A or L
    char room_num[11]; // store the vaule of the current line room num
    char curr_key[EVP_MAX_MD_SIZE]; //not really needed for this part
    int roomNumList = 0; //keep track of the lenght of the list of room numbers (used for allocation)
    i=0;
    int k = 0;

    char *new_text = NULL;
    new_text = malloc(plaintext_len);
    memcpy(new_text, plaintext, plaintext_len);

    char type[2];
      if(E){
        strcpy(type, "E");
      }else{
        strcpy(type, "G");
      }

    if(S){
      //print entire document
      gallery g = {0};


      while(i < plaintext_len){
        int j = 0;
        free(buff);
        while(new_text[i] != '\0'){
          i++;
          j++;
        }
        i++;
        j++;
        buff = malloc(j);
        
        memcpy(buff, new_text +start , j);
        read = sscanf(buff, "%s %s %s %s %s %s\n", arriveOrLeave, typeOfPerson, curr_key, curr_time, curr_name, room_num);
        if(read == 6){
          //read in a room num
          if(strcmp(arriveOrLeave, "A") == 0){
            if(g.rooms == NULL){
              g.rooms = malloc(sizeof (room ));
              g.rooms[0].names = malloc(sizeof (char *));
              g.room_list = malloc(sizeof (int *));
              g.room_list[0] = malloc(sizeof (int));
              g.rooms[0].curr_num = malloc(strlen(room_num) + 1);
              strcpy(g.rooms[0].curr_num, room_num);
              *g.room_list[0] = strtol(room_num, NULL, 10);
              g.rooms[0].names[0] = malloc(strlen(curr_name) + 1);
              strcpy(g.rooms[0].names[0], curr_name);
              g.num_rooms++;
              g.rooms[0].num_names++;
              g.rooms[0].act_num_names++;
            }else{
              int room_found = 0;
              for(k=0; k<g.num_rooms; k++){
                if(strcmp(g.rooms[k].curr_num, room_num) == 0){
                  room_found = 1;
                  if(g.rooms[k].names == NULL){
                    g.rooms[k].names = malloc(sizeof(char *));
                    g.rooms[k].names[0] = malloc(strlen(curr_name));
                    strcpy(g.rooms[k].names[0], curr_name);
                    g.rooms[k].num_names++;
                    g.rooms[k].act_num_names++;
                    break;
                  }else{
                    g.rooms[k].num_names++;
                    g.rooms[k].act_num_names++;
                    g.rooms[k].names = realloc(g.rooms[k].names, sizeof(char *) * (g.rooms[k].num_names));
                    g.rooms[k].names[g.rooms[k].num_names-1] = malloc(strlen(curr_name ) +1);
                    strcpy(g.rooms[k].names[g.rooms[k].num_names-1], curr_name);
                    break;
                  }
                }
              }
              if(!room_found){
                g.num_rooms++;
                g.rooms = realloc(g.rooms, (sizeof(room)) * g.num_rooms);
                g.rooms[g.num_rooms-1].names = malloc(sizeof (char *));
                g.rooms[g.num_rooms-1].curr_num = malloc(strlen(room_num) + 1);
                strcpy(g.rooms[g.num_rooms-1].curr_num, room_num);
                g.room_list = realloc(g.room_list, sizeof (int *) * g.num_rooms);
                g.room_list[g.num_rooms-1] = malloc(sizeof (int));
                *g.room_list[g.num_rooms-1]= strtol(room_num, NULL, 10);
                g.rooms[g.num_rooms-1].names[0] = malloc(strlen(curr_name) + 1);
                strcpy(g.rooms[g.num_rooms-1].names[0], curr_name);
                g.rooms[g.num_rooms-1].num_names++;
                g.rooms[g.num_rooms-1].act_num_names++;
              }
            }
          }else{
            int l = 0;
            while(l < g.num_rooms){
              if(strcmp(g.rooms[l].curr_num, room_num) == 0){
                for(k = 0; k < g.rooms[l].num_names; k++){
                  if(strcmp(g.rooms[l].names[k], curr_name) == 0){
                    strcpy(g.rooms[l].names[k], "");
                    g.rooms[l].act_num_names--;
                  }
                }
              }

              l++;
            }
          }
        }else if(read == 5){
          if(strcmp(arriveOrLeave, "A") == 0){
            //arrive at gallery
            if(strcmp(typeOfPerson, "E") == 0){
              //empolyye arrived at the gallery
              if(g.employees == NULL){
                g.employees = malloc(sizeof(char *));
                g.employees[0] = malloc(strlen(curr_name) + 1);
                strcpy(g.employees[0], curr_name);
                g.num_employees++;
                g.act_num_employees++;
              }else{
                int name_found = 0;
                for(k = 0; k < g.num_employees; k++){
                  if(strcmp(curr_name, g.employees[k]) == 0){
                    name_found = 1;
                  }
                }
                if(!name_found){
                  g.num_employees++;
                  g.act_num_employees++;
                  g.employees = realloc(g.employees, sizeof(char *) * (g.num_employees));
                  g.employees[g.num_employees-1] = malloc(strlen(curr_name) + 1);
                  strcpy(g.employees[g.num_employees-1], curr_name);
                }
              }
            }else{
              //guest
              if(g.guests == NULL){
                g.guests = malloc(sizeof(char *));
                g.guests[0] = malloc(strlen(curr_name) + 1);
                strcpy(g.guests[0], curr_name);
                g.num_guests++;
                g.act_num_guests++;
              }else{
                int name_found = 0;
                for(k = 0; k < g.num_guests; k++){
                  if(strcmp(curr_name, g.guests[k]) == 0){
                    name_found = 1;
                  }
                }
                if(!name_found){
                  g.num_guests++;
                  g.act_num_guests++;
                  g.guests = realloc(g.guests, sizeof(char *) * (g.num_guests));
                  g.guests[g.num_guests-1] = malloc(strlen(curr_name) + 1);
                  strcpy(g.guests[g.num_guests-1], curr_name);
                }
              }
            }
          }else{
            //leave gallery
            if(strcmp(typeOfPerson, "E") == 0){
              //employee
              for(k = 0; k < g.num_employees; k++){
                if(strcmp(g.employees[k], curr_name) == 0){
                  strcpy(g.employees[k], "");
                  g.act_num_employees--;
                }
              }
            }else{
              //guest
              for(k = 0; k < g.num_guests; k++){
                if(strcmp(g.guests[k], curr_name) == 0){
                  strcpy(g.guests[k], "");
                  g.act_num_guests--;
                }
              }
            }
          }
        }
        memset(curr_time, 0x0, 11);
        memset(curr_name, 0x0, 512);
        memset(typeOfPerson, 0x0, 2);
        memset(room_num, 0x0, 11);
        memset(arriveOrLeave, 0x0, 2);
        memset(buff, 0x0, j);
        
        start += j;
      }

      //first print employees
      int j=0;
      if(g.act_num_employees == 0){
        printf("\n");
      }else{
        
        qsort(g.employees, g.num_employees, sizeof (const char *), sort);
        for(i = 0; i < g.num_employees; i++){
          if(strcmp("", g.employees[i]) != 0){
            j++;
            if(j == g.act_num_employees){
              printf("%s\n", g.employees[i]);
            }else{
              printf("%s,", g.employees[i]);
            }
          
          }
        }
      }

      //then print guests
      j = 0;
      if(g.act_num_guests == 0){
        printf("\n");
      }else{
        qsort(g.guests, g.num_guests, sizeof (const char *), sort);
        for(i = 0; i < g.num_guests; i++){
          if(strcmp("", g.guests[i]) != 0){
            j++;
            //if its the last employee
          if(j == g.act_num_guests){
            printf("%s\n", g.guests[i]);
          }else{
            printf("%s,", g.guests[i]);
          }
        }
        }
      }

      //print rooms
      if(g.num_rooms != 0){
        qsort(g.room_list, g.num_rooms, sizeof (int *), sortNums);

        int count = 0;
        char room_string[11];
        memset(room_string, 0x0, 11);
        while (count < g.num_rooms){
          for(i = 0; i < g.num_rooms; i++){
            sprintf(room_string, "%d", *g.room_list[count]);
            if(strcmp(room_string, g.rooms[i].curr_num) == 0){
              if(g.rooms[i].act_num_names != 0){
                printf("%s:", g.rooms[i].curr_num);
                qsort(g.rooms[i].names, g.rooms[i].act_num_names, sizeof(char *), sort);
                int seen_names = 0;
                for(j = 0; j < g.rooms[i].num_names; j++){
                  if(strcmp(g.rooms[i].names[j], "") != 0){
                    if(seen_names + 1 == g.rooms[i].act_num_names){
                      printf("%s\n", g.rooms[i].names[j]);
                    }else{
                      printf("%s,", g.rooms[i].names[j]);
                    }
                    seen_names++;
                  }
                }
              }
            }  
          }
          memset(room_string, 0x0, 11);
          count++;
        }
      }

      return 1;
    }else{
      // print chronological order of rooms visited for guest/employee
      char *roomsVisited = NULL;

      while(i < plaintext_len){
        int j = 0;
        free(buff);
        while(new_text[i] != '\0'){
          i++;
          j++;
        }
        i++;
        j++;
        buff = malloc(j);
        
        memcpy(buff, new_text +start , j);
        read = sscanf(buff, "%s %s %s %s %s %s\n", arriveOrLeave, typeOfPerson, curr_key, curr_time, curr_name, room_num);
        if(read == 6){
          if((strcmp(curr_name, (char *)name) == 0) && strcmp(type, typeOfPerson) == 0 && strcmp(arriveOrLeave, "A") == 0){
              len = strlen(room_num);
              roomsVisited = realloc(roomsVisited, len + roomNumList);
              memcpy(roomsVisited+ roomNumList, room_num, len);
              roomNumList += len;
              roomsVisited[roomNumList++] = ',';  
          }
          memset(curr_time, 0x0, 11);
          memset(curr_name, 0x0, 512);
          memset(typeOfPerson, 0x0, 2);
          memset(room_num, 0x0, 11);
          memset(arriveOrLeave, 0x0, 2);
          memset(buff, 0x0, j);
        }
        
        start += j;
      }
      if(roomsVisited == NULL){
        printf("\n");
        return 1;
      }
      roomsVisited[--roomNumList] = '\0';
      printf("%s\n", roomsVisited);
      return 1;
    }

}
