#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <ctype.h>
#include <getopt.h>

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "map.h"
#include "map.c"


int main(int argc, char * argv[]) {
  int i = 0;
  int j = 0;
  int opt = -1;
  int len;
  char * token = NULL;
  char * logpath = NULL;
  FILE * inputlogfile;
  char tag[17] = {
    0
  };
  int inputlogsize;
  char * inputlog;
  char * decryptedlog;
  char * decryptedlogdup;
  EVP_CIPHER_CTX * ctx;
  char iv[16] = {
    0
  };
  int decryptedlen;
  int employee_guest = -1; // EMPLOYEE OR GUEST
  char * name = NULL;
  int last_timestamp;
  int s_or_r = -1;

  while ((opt = getopt(argc, argv, "STIRK:G:E:")) != -1) {
    switch (opt) {
      case 'K':
        token = calloc(strlen(optarg) + 1, sizeof(char));
        strcpy(token, optarg);
        for ( i = 0; i < strlen(token); i++) {
          if (isalnum(token[i]) == 0) {
            printf("integrity violation");
            exit(255);
          }
        }
        break;

      case 'S':
        if (s_or_r == 1) {
          printf("invalid");
          exit(255);
        }
        s_or_r = 0;
        break;

      case 'R':
        if (s_or_r == 0) {
          printf("invalid");
          exit(255);
        }
        s_or_r = 1;
        break;

      case 'T':
        printf("unimplemented");
        exit(0);
        break;

      case 'I':
        printf("unimplemented");
        exit(0);
        break;

      case 'G':
        // if employee_guest is 0 error out
        // otherwise set employee_guest to 1
        // read in name
        if (employee_guest == 0) { // employee has been specified
          printf("invalid\n");
          exit(255);
        }
        employee_guest = 1;
        name = calloc(strlen(optarg) + 1, sizeof(char));
        strcpy(name, optarg);
        for ( i = 0; i < strlen(name); i++) {
          if (isalpha(name[i]) == 0) {
            printf("invalid\n");
            exit(255);
          }
        }
        //guest name
        break;

      case 'E':
        // if employee_guest is 1 error out
        // otherwise set employee_guest to 0
        // read in name
        if (employee_guest == 1) { // guest has been specified
          printf("invalid\n");
          exit(255);
        }
        employee_guest = 0;
        name = calloc(strlen(optarg) + 1, sizeof(char));
        strcpy(name, optarg);
        for ( i = 0; i < strlen(name); i++) {
          if (isalpha(name[i]) == 0) {
            printf("invalid\n");
            exit(255);
          }
        }
        break;

      case ':':
        printf("invalid\n");
        exit(255);
        break;

      case '?':
        printf("invalid\n");
        exit(255);
        break;

      default:
        //unknown option, leave
        break;
      }
    }
  if (optind < argc) {
    logpath = calloc(strlen(argv[optind]) + 1, sizeof(char));
    strcpy(logpath, argv[optind]);
  }
  if (optind != argc - 1){
    // extra arguments
    printf("invalid\n");
    exit(255);
  }
  if ((token == NULL) || (logpath == NULL)) { // if token or name or logpath weren't read in error out
    printf("invalid\n");
    exit(255);
  }

  if (s_or_r == -1){
    // WE DIDNT SPECIFY S OR R, ERROR OUT
    printf("invalid\n");
    exit(255);
  }
  if (s_or_r == 0){
    // s case, make sure we don't have unnecessary variables
    if (employee_guest != -1){
      printf("invalid\n");
      exit(255);
    }
    if (name != NULL){
      printf("invalid\n");
      exit(255);
    }
  }

  if (s_or_r == 1){
    if (employee_guest == -1){
      printf("invalid\n");
      exit(255);
    }
    if (name == NULL){
      printf("invalid\n");
      exit(255);
    }
  }

map_str_t m;
map_init( & m);

  // END MAP CREATION
if ((inputlogfile = fopen(logpath, "r'")) == NULL) {
    // log does not exist, fail out
  printf("invalid\n");
  exit(255);

} else {
    // we are able to open the log
  char * log_line;

  fseek(inputlogfile, 0L, SEEK_END);
  inputlogsize = ftell(inputlogfile);
  rewind(inputlogfile);
  if (inputlogsize < 32) {
    printf("invalid\n");
    exit(255);
  }
  inputlog = calloc(inputlogsize, sizeof(char));
  fread(inputlog, sizeof(char), inputlogsize, inputlogfile);
    fclose(inputlogfile); // inputlog contains the encrypted input log

    memcpy(iv, inputlog+(inputlogsize-16),16);
    memcpy(tag, inputlog + (inputlogsize - 32), 16); // grab tag
    memset(inputlog + (inputlogsize - 32), '\0', sizeof(char)); // remove tag from inputlog 
    // ---------------------------------- VERIFY TAG AND DECRYPT LOG ------------------------------
    ctx = EVP_CIPHER_CTX_new();
    decryptedlog = calloc(inputlogsize * 2, sizeof(char));
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, token, iv);
    EVP_DecryptUpdate(ctx, decryptedlog, &len, inputlog, inputlogsize - 32);
    decryptedlen = len;

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)) {
      printf("integrity violation");
      exit(255);
    }
    if (EVP_DecryptFinal_ex(ctx, decryptedlog + decryptedlen, &len) <= 0) {
      printf("invalid\n");
      exit(255);
    }
    decryptedlen += len;
    decryptedlogdup = calloc(strlen(decryptedlog) + 1, sizeof(char));
    strcpy(decryptedlogdup, decryptedlog);
    // ------------------- LOG HAS BEEN VERIFIED AND DECRYPTED INTO decryptedlog -----------------------

    if (s_or_r == 0) {
      const char * key;
      char * temp;
      int first = 1;
      int counter = 0;
      int mapsize = 0;
      int holder;
      map_iter_t iter = map_iter( & m);
      // Process the log
      char * end_str;
      log_line = strtok_r(decryptedlog, "\n", &end_str);
      while (log_line != NULL) {
        char * end_token;
        //printf("%s+\n", log_line);
        char * line_parser = strtok_r(log_line, ",", & end_token);
        last_timestamp = atoi(line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        int line_employee_guest = atoi(line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        char * line_name = calloc(strlen(line_parser) + 1, sizeof(char));
        strcpy(line_name, line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        int line_arrival_departure = atoi(line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        int line_roomid = atoi(line_parser);

        // store in hashmap and check against it
        char * hashkey_string = calloc(strlen(line_name) * 2, sizeof(char));
        sprintf(hashkey_string, "%d%s", line_employee_guest, line_name); // Generate hashkey by combining 0 or 1 for employee / guest
        // printf("hashkey_string:%s\n",hashkey_string);
        map_remove( & m, hashkey_string); // remove current position

        if (line_arrival_departure == 0) {
          // process the arrival
          map_set( & m, hashkey_string, line_roomid);
          // printf("%d\n", map_get(&m, hashkey_string));

        } else {
          // process the departure
          if (line_roomid != -1) {
            map_set( & m, hashkey_string, -1);
          }
          // printf("%d\n", map_get(&m, hashkey_string));
        }
        log_line = strtok_r(NULL, "\n", & end_str);
      }
      //print employee list
      while ((key = map_next( & m, & iter))) {
        mapsize++;
      }
      map_iter_t iter5 = map_iter( & m);
      char *employee_names_tosort[mapsize];
      char *employee_holder;
      while ((key = map_next(&m, &iter5))){
        if ((  map_get( & m, key) != NULL) && (key[0] == '0')) { //need to check if the person has left the gallery entirely
          employee_names_tosort[counter] = calloc(strlen(key), sizeof(char));
          memmove(employee_names_tosort[counter], key + 1, strlen(key));
          counter++;  
        }
      }
      for ( i = 0; i < counter; i++) {
        for ( j = 0; j < counter; j++) {
          if (strcmp(employee_names_tosort[j], employee_names_tosort[i]) > 0) {
            employee_holder = employee_names_tosort[i];
            employee_names_tosort[i] = employee_names_tosort[j];
            employee_names_tosort[j] = employee_holder;
          }
        }
      }
      first = 1;
      //print names for given room
      for ( i = 0; i < counter; i++) {
        if (first) {
          printf("%s", employee_names_tosort[i]);
          first--;
        } else {
          printf(",");
          printf("%s", employee_names_tosort[i]);
        }
      }
      for ( i = 0; i < counter; i++) {
        free(employee_names_tosort[i]);
      }
      printf("\n");
      first = 1;
      counter = 0;
      map_iter_t iter2 = map_iter( & m);
      char *guest_names_tosort[mapsize];
      char *guest_holder;
      //print guest list
      while ((key = map_next( & m, & iter2))) {
        if (( map_get( &m, key) != NULL) && (key[0] == '1')) { //need to check if the person has left the gallery entirely
          guest_names_tosort[counter] = calloc(strlen(key), sizeof(char));
          memmove(guest_names_tosort[counter], key + 1, strlen(key));
          counter++;  
        }
      }
      for ( i = 0; i < counter; i++) {
        for ( j = 0; j < counter; j++) {
          if (strcmp(guest_names_tosort[j], guest_names_tosort[i]) > 0) {
            guest_holder = guest_names_tosort[i];
            guest_names_tosort[i] = guest_names_tosort[j];
            guest_names_tosort[j] = guest_holder;
          }
        }
      }
      first = 1;
      //print names for given room
      for ( i = 0; i < counter; i++) {
        if (first) {
          printf("%s", guest_names_tosort[i]);
          first--;
        } else {
          printf(",");
          printf("%s", guest_names_tosort[i]);
        }
      }
      for ( i = 0; i < counter; i++) {
        free(guest_names_tosort[i]);
      }
      first = 1;
      //print people in each room
      //get an array of room ids, sort it, iterate through the sorted room IDs, print the room ID, then
      //iterate over the hashmap, if value == roomID, print it
      //printf("%d\n", mapsize);
      int rooms[mapsize];
      counter = 0;
      map_iter_t iter3 = map_iter( & m);
      //populate array with all roomIDs
      while ((key = map_next( & m, & iter3))) {
        rooms[counter] = * map_get( & m, key);
        counter++;
      }
      //sort the roomIDs
      for ( i = 0; i < mapsize; i++) {
        for ( j = 0; j < mapsize; j++) {
          if (rooms[j] > rooms[i]) {
            holder = rooms[i];
            rooms[i] = rooms[j];
            rooms[j] = holder;
          }
        }
      }
      //remove duplicates
      int nodups[mapsize];
      counter = 0;
      for ( i = 0; i < mapsize - 1; i++) {
        if (rooms[i] != rooms[i + 1]) {
          nodups[counter++] = rooms[i];
        }
      }
      nodups[counter++] = rooms[mapsize - 1];
      for ( i = 0; i < counter; i++) {
        rooms[i] = nodups[i];
      }
      //iterate through sorted roomIDs, print the room ID, then iterate over the hashmap. If value == roomID, print key
      char * holder2;
      for ( i = 0; i < counter; i++) {
        if (rooms[i] == -1) {
          continue;
        }
        holder = 0;
        char * tosort[counter];
        map_iter_t iter4 = map_iter( & m);
        printf("\n");
        printf("%d: ", rooms[i]);
        //printf("%s\n", map_next(&m, &iter4));
        //make a new array with the names for given room
        while ((key = map_next( & m, & iter4))) {
          if (rooms[i] == * map_get( & m, key)) {
            tosort[holder] = calloc(strlen(key), sizeof(char));
            memmove(tosort[holder], key + 1, strlen(key));
            holder++;
          }
        }
        //sort the names for given room
        int k;
        for ( k = 0; k < holder; k++) {
          for ( j = 0; j < holder; j++) {
            if (strcmp(tosort[j], tosort[k]) > 0) {
              holder2 = tosort[k];
              tosort[k] = tosort[j];
              tosort[j] = holder2;
            }
          }
        }
        first = 1;
        //print names for given room
        int l;
        for (l = 0; l < holder; l++) {
          if (first) {
            printf("%s", tosort[l]);
            first--;
          } else {
            printf(",");
            printf("%s", tosort[l]);
          }
        }
        int m;
        for ( m = 0; m < holder; m++) {
          free(tosort[m]);
        }
      }
    }
    if (s_or_r ==1 ) { //E or G must be specified
      int first = 1;
      if (name == NULL) {
        printf("invalid");
        exit(255);
      }
      // Process the log
      char * end_str;
      strcpy(decryptedlog, decryptedlogdup);
      log_line = strtok_r(decryptedlog, "\n", & end_str);
      while (log_line != NULL) {
        char * end_token;
        // printf("%s+\n", log_line);
        char * line_parser = strtok_r(log_line, ",", & end_token);
        last_timestamp = atoi(line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        int line_employee_guest = atoi(line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        char * line_name = calloc(strlen(line_parser) + 1, sizeof(char));
        strcpy(line_name, line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        int line_arrival_departure = atoi(line_parser);
        line_parser = strtok_r(NULL, ",", & end_token);
        int line_roomid = atoi(line_parser);
        if ((strcmp(name, line_name) == 0) && (line_arrival_departure == 0) && (line_roomid != -1)) {
          if (first) {
            printf("%d", line_roomid);
            first--;
          } else {
            printf(",%d", line_roomid);
          }
        }
        free(line_name);
        log_line = strtok_r(NULL, "\n", & end_str);
      }
    }
  }
  exit(0);
}