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

typedef struct {
  char **names ;
  char *curr_num;
  int num_names;
  int act_num_names;
} room;

typedef struct {
  room **rooms;
  char **employees;
  char **guests;
  int *room_list;
  int num_rooms;
  int num_employees;
  int num_guests;
  int act_num_guests;
  int act_num_employees;
} gallery;

static int sort(const void *a, const void *b) {
  if(a == NULL || b == NULL) {
    if(a == NULL){
      return 1;
    } else {
      return -1;
    }
  }
  return strcmp(* (const char **) a, *(const char **) b);
}

static int sortNums(const void *a, const void *b){
  const int *ia = (const int *)a;
  const int *ib = (const int *)b;
  return *ia - *ib;
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
      handleErrors();
    }

    if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
      handleErrors(); 
    }
    pltxt_len = len;

    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    if (ret > 0) {
      pltxt_len += len;
      return pltxt_len;
    } else {
      integrityViolation();
			return -1; //Will never happen
    }
}

int main(int argc, char *argv[]) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  int opt;
  
  int i; //For any iterations
  size_t len;

  int S = 0, R = 0, E = 0, G = 0, K = 0, I = 0, T = 0;

  unsigned char iv[IV_LEN]; //can't be bigger
  unsigned char key[KEY_LEN];//store the value of the key they use to access the file
  memset(iv, 0, IV_LEN);
  memset(key, 0, KEY_LEN); 

  unsigned char *plaintext = NULL; //BUFFER FOR ANY PLAINTEXT GOTTEN FROM LOG
  unsigned int plaintext_len; //Populated when decrypting to know how much plaintext to read!

  unsigned char *ciphertext = NULL; //BUFFER FOR ANY CIPHERTEXT TO BE WRITTEN TO FILE
  unsigned int ciphertext_len; //Populated when encrypting to know how much ciphertext to read!

  char *name = NULL;
  unsigned char *real_key = NULL; //Not cut down to 32, still checks authenticity
  char *path = NULL;

  while ((opt = getopt(argc, argv, "K:SRE:G:TI")) != -1) {
    switch(opt) {
      case 'T':
        if (R || S || I) {
          handleErrors();
        }
        T = 1;
      break;

      case 'I':
        if (S || R || T) {
          handleErrors();
        }
        I = 1;
      break;

      case 'K':
        free(real_key);
        real_key = NULL;

        K = 1;
        
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
        if(R || E || G || I || T) {
          handleErrors();
        }
        S = 1;
        break;

      case 'R':
        //can only have S or R
        if(S || I || T) {
          handleErrors();
        }
        R = 1;
        break;

      case 'E':
        if (S) {
          handleErrors();
        }
        ++E; //maybe have multiple due to -I option
        
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
        if (S) {
          handleErrors();
        }
        ++G;
        
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
      if(!isdigit(argv[optind][i]) 
        && !isalpha(argv[optind][i]) && argv[optind][i] != '.' 
        && argv[optind][i] != '/' && argv[optind][i] != '_' && argv[optind][i] != '\0'){
        handleErrors();
      }
      path[i] = argv[optind][i];
    }
    path[i] = '\0';
  } else {
    handleErrors(); //incorrect arguments provided
  }

  if (!(S && K && !R && !E && !G && !I && !T) 
  	&& !(R && K && !S && !I && !T && ((E && !G) || (!E && G)))
  	&& !(I && K && !S && !R && !T && (E || G)) 
  	&& !(T && K && !S && !R && !I && ((E && !G) || (!E && G)))) {
    handleErrors(); //only correct invocations of program
  }

    //decrypt file to read
    FILE *fp;
    if ((fp = fopen(path, "r")) == NULL) {
      handleErrors();
    }

    char type[2];
    if (E) {
      type[0] = 'E';
    } else {
      type[0] = 'G';
    }
    type[1] = '\0';

    char *curr_time = NULL; //get the time of the current line
    char *curr_name = NULL;
    char arriveOrLeave[2]; // A or L
    char typeOfPerson[2];
    char *room_num = NULL; // store the value of the current line room num
    char *curr_key = NULL; //not really needed for this part 
    int roomNumList = 0; //keep track of the lenght of the list of room numbers (used for allocation)
    int read = 0;
	int timeSpent = 0, in_gallery = 0, prev_time_entered = 0, most_recent_time = 0;
    //First, we'll need to decrypt 
    //Read from fp into ciphertext buffer
    //Then populate plaintext buffer via
    //decrypt method

    fseek(fp, 0, SEEK_END);
    ciphertext_len = ftell(fp);
    rewind(fp);
    //file exists

    ciphertext = malloc(ciphertext_len);
    fread(ciphertext, 1, ciphertext_len, fp);
    fclose(fp);//done reading

    unsigned char *scapegoat = malloc(ciphertext_len);
    memset(scapegoat, 0, ciphertext_len);

    int dummy;
    if (-1 == 
        (dummy = decrypt(ctx, ciphertext,
          ciphertext_len, key, iv, scapegoat))) {
        handleErrors();
    }

    plaintext_len = (unsigned int) dummy;
    plaintext = malloc(plaintext_len);
    memcpy(plaintext, scapegoat, plaintext_len);

    free(scapegoat);
    scapegoat = NULL;
    //now we have plaintext and length populated
    
    int k = 0;
    char *roomsVisited = NULL;

    gallery g = {0};

    i = 0;
    while (i < plaintext_len) {
      arriveOrLeave[0] = plaintext[i++];
      arriveOrLeave[1] = '\0';
      i++;//Space
      read++;

      typeOfPerson[0] = plaintext[i++];
      typeOfPerson[1] = '\0';
      i++;//Space
      read++;

      len = 0;
      while (plaintext[i + len] != ' ') {
        len++;
      }
      curr_key = malloc(len + 1);
      memcpy(curr_key, plaintext + i, len);
      i += len + 1;
      curr_key[len] = '\0';
      read++;

      len = 0;
      while(plaintext[i + len] != ' ') {
        len++;
      }
      curr_time = malloc(len + 1);
      memcpy(curr_time, plaintext + i, len);
      i += len + 1;
      curr_time[len] = '\0';
      read++;

      len = 0;
      while(plaintext[i + len] != ' ' 
          && plaintext[i + len] != '\0') {
        len++;
      }
      curr_name = malloc(len + 1);
      read++;
        
      memcpy(curr_name, plaintext + i, len);

      if (plaintext[i + len] == ' ') {
        i += len + 1;
        curr_name[len] = '\0';
        len = 0;
        while(plaintext[i + len] != '\0') {
          len++;
        }
        room_num = malloc(len + 1);
        memcpy(room_num, plaintext + i, len);
        read++;
        i += len + 1;
        room_num[len] = '\0';

    } else {
        curr_name[len] ='\0';
        i += len + 1;
    }
    if (T) {
    	most_recent_time = (int) strtol(curr_time, NULL, 10);
		if (!strcmp(name, curr_name) &&
			type[0] == typeOfPerson[0]) {  //it's the guest, what is he doing
			//Only need to add up time when my dude leaves gallery
			//If he's still in gallery at end, add some more time to the guy
			if (room_num == NULL) { //only chance my guy may be leaving or arriving gallery
				
				if (arriveOrLeave[0] == 'A') {
					in_gallery = 1;
					prev_time_entered = most_recent_time;
				} else { //leaving gallery, clock your time
					in_gallery = 0;
					timeSpent += most_recent_time - prev_time_entered;
				}
			}
		}
	} else if(S){
      //print entire document
        if(read == 6){
          //read in a room num
          if(strcmp(arriveOrLeave, "A") == 0){
            if(g.rooms == NULL){
              g.rooms = malloc(sizeof (char *));
              g.rooms[0] = malloc(sizeof (room));
              g.rooms[0]->names = malloc(sizeof (char *));
              g.room_list = malloc(sizeof (int ));
              g.rooms[0]->curr_num = malloc(strlen(room_num) + 1);
              strcpy(g.rooms[0]->curr_num, room_num);
              g.room_list[0] = strtol(room_num, NULL, 10);
              g.rooms[0]->names[0] = malloc(strlen(curr_name) + 1);
              strcpy(g.rooms[0]->names[0], curr_name);
              g.num_rooms++;
              g.rooms[0]->num_names = 1;
              g.rooms[0]->act_num_names = 1;
            }else{
              int room_found = 0;
              for(k=0; k<g.num_rooms; k++){
                if(strcmp(g.rooms[k]->curr_num, room_num) == 0){
                  room_found = 1;
                  if(g.rooms[k]->names == NULL){
                    g.rooms[k]->names = malloc(sizeof(char *));
                    g.rooms[k]->names[0] = malloc(strlen(curr_name));
                    strcpy(g.rooms[k]->names[0], curr_name);
                    g.rooms[k]->num_names++;
                    g.rooms[k]->act_num_names++;
                    break;
                  }else{
                    g.rooms[k]->num_names++;
                    g.rooms[k]->act_num_names++;
                    g.rooms[k]->names = realloc(g.rooms[k]->names, sizeof(char *) * (g.rooms[k]->num_names));
                    g.rooms[k]->names[g.rooms[k]->num_names-1] = malloc(strlen(curr_name ) +1);
                    strcpy(g.rooms[k]->names[g.rooms[k]->num_names-1], curr_name);
                    break;
                  }
                }
              }
              if(!room_found){
                g.num_rooms++;
                g.rooms = realloc(g.rooms, (sizeof(room *)) * g.num_rooms);
                g.rooms[g.num_rooms-1] = malloc(sizeof (room));
                g.rooms[g.num_rooms-1]->names = malloc(sizeof (char *));
                g.rooms[g.num_rooms-1]->curr_num = malloc(strlen(room_num) + 1);
                strcpy(g.rooms[g.num_rooms-1]->curr_num, room_num);
                g.room_list = realloc(g.room_list, sizeof (int ) * g.num_rooms);
                g.room_list[g.num_rooms-1] = strtol(room_num, NULL, 10);
                g.rooms[g.num_rooms-1]->names[0] = malloc(strlen(curr_name) + 1);
                strcpy(g.rooms[g.num_rooms-1]->names[0], curr_name);
                g.rooms[g.num_rooms-1]->num_names = 1;
                g.rooms[g.num_rooms-1]->act_num_names = 1;
              }
            }
          }else{
            int l = 0;
            while(l < g.num_rooms){
              if(strcmp(g.rooms[l]->curr_num, room_num) == 0){
                for(k = 0; k < g.rooms[l]->num_names; k++){
                  if(strcmp(g.rooms[l]->names[k], curr_name) == 0){
                    strcpy(g.rooms[l]->names[k], "");
                    //free(g.rooms[l]->names[k]);
                    //g.rooms[l]->names[k] = NULL;
                    g.rooms[l]->act_num_names--;
                  }
                }
              }

              l++;
            }
          }
        }else{
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
                  //free(g.employees[k]);
                  //g.employees[k] = NULL;
                  g.act_num_employees--;
                }
              }
            }else{
              //guest
              for(k = 0; k < g.num_guests; k++){
                if(strcmp(g.guests[k], curr_name) == 0){
                  strcpy(g.guests[k], "");
                  //free(g.guests[k]);
                  //g.guests[k] = NULL;
                  g.act_num_guests--;
                }
              }
            }
          }
        }
      }else if (R) {
      // print chronological order of rooms visited for guest/employee
        if(read == 6){
          if((strcmp(curr_name, (char *)name) == 0) && strcmp(type, typeOfPerson) == 0 && strcmp(arriveOrLeave, "A") == 0){
              len = strlen(room_num);
              roomsVisited = realloc(roomsVisited, len + roomNumList);
              memcpy(roomsVisited+ roomNumList, room_num, len);
              roomNumList += len;
              roomsVisited[roomNumList++] = ',';  
          }
        }
      
  }
            //free stuff
        free(curr_name);
        curr_name = NULL;
        free(curr_time);
        curr_time = NULL;
        free(curr_key);
        curr_key = NULL;
        free(room_num);
        room_num = NULL;
        read = 0;

}

     if (T) {
     	//log the extra minutes if need be
     	if (in_gallery) {
     		timeSpent += most_recent_time;
     	}
     	if (timeSpent) {
     		printf("%d", timeSpent);
     	}

     	return 0; //this is probably all we need for time, I'll start working on I tomorrow morning

     } else if(R){
      if(roomsVisited == NULL){
        printf("\n");
        return 0;
       }
      roomsVisited[--roomNumList] = '\0';
      printf("%s\n", roomsVisited);
      return 0;
     }else if (S) {
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
        qsort(g.room_list, g.num_rooms, sizeof (int ), sortNums);

        int count = 0;
        char room_string[11];
        memset(room_string, 0x0, 11);
        int empty_rooms = 0;
          for(i = 0; i < g.num_rooms; i++){
            if(g.rooms[i]->act_num_names == 0){
              empty_rooms++;
            }
          }
        while (count < g.num_rooms){
          sprintf(room_string, "%d", g.room_list[count]);
          for(i = 0; i < g.num_rooms; i++){
            if(strcmp(room_string, g.rooms[i]->curr_num) == 0){
              if(g.rooms[i]->act_num_names != 0){
                printf("%s: ", g.rooms[i]->curr_num);
                qsort(g.rooms[i]->names, g.rooms[i]->num_names, sizeof(char *), sort);
                int seen_names = 0;
                for(j = 0; j < g.rooms[i]->num_names; j++){
                  if(strcmp(g.rooms[i]->names[j], "") != 0){

                    if(seen_names + 1 == g.rooms[i]->act_num_names){
                      if(count + 1 + empty_rooms == g.num_rooms){
                        printf("%s", g.rooms[i]->names[j]);
                      }else{
                        printf("%s\n", g.rooms[i]->names[j]);
                      }
                    }else{
                      printf("%s,", g.rooms[i]->names[j]);
                    }
                    seen_names++;
                  }
                }
              }
              break;
            }  
          }
          memset(room_string, 0x0, 11);
          count++;
        }
      }

      return 0;
  }

}
