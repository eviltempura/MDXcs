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
#include <time.h>

int main(int argc, char *argv[]) {
  int i;
  int opt = -1;
   // TIMESTAMP
 
  char *newlogentry;
  FILE *inputlogfile; FILE *batchfile;
  int inputlogsize;
  char *inputlog;
  char tag[17]= {0};
  char *decryptedlog;
  char *decryptedlogdup;
  EVP_CIPHER_CTX *ctx;
  char iv[16] = {0};
  int len;
  int decryptedlen;
  int last_timestamp;
  char *outputlog_plaintext;

  FILE *outputfile;
  char *outputlog_encrypted;
  int encryptedlen;

  // variables read in
  int timestamp = -1;
  char *token = NULL; char *batchfilepath;
  int arrival_departure = -1;  // ARRIVAL OR DEPARTURE
  int employee_guest = -1; // EMPLOYEE OR GUEST
  char *name = NULL;
  char *logpath = NULL;
  int roomid = -1;

  while ((opt = getopt(argc, argv, "T:K:E:G:ALR:B:")) != -1) {
    switch(opt) {
      case 'B':
         batchfilepath =calloc(strlen(optarg)+1,sizeof(char)); strcpy(batchfilepath, optarg);
        goto batch;
        exit(0);
        break;

      case 'T': // timestamp case
        for (i = 0; i < strlen(optarg); i++){
          if (!isdigit(optarg[i])){
            printf("invalid\n");
            exit(255);
          }

        }
        if (sscanf(optarg, "%d", &timestamp) == 0){
          printf("invalid\n");
          exit(255);
        }

        if ( (timestamp <1) || (timestamp>1073741823) ){
          printf("invalid\n");
          exit(255);
        }
        break;

      case 'K':
        // check that token is only alphanumeric
        token = calloc(strlen(optarg) + 1, sizeof(char));
        strcpy(token,optarg);
        for (i = 0; i < strlen(token); i++){
          if (isalnum(token[i])==0){
            printf("invalid\n");
            exit(255);
          }
        }

        //secret token
        break;

      case 'A':
        // if arrival_departure is set to departure, error out
        if (arrival_departure == 1){
          printf("invalid\n");
          exit(255);
        }
        // otherwise set arrival_departure to 0 
        arrival_departure = 0;
        break;

      case 'L':
        // if arrival_departure is set to 0 for arrival then error out
        if (arrival_departure == 0){
          printf("invalid\n");
          exit(255);
        }
        arrival_departure = 1;
        // if arrival_departure is 0 error out
        // othwersie set arrival_Departure to 1

        //departure
        break;

      case 'E':
        // if employee_guest is 1 error out
        // otherwise set employee_guest to 0
        // read in name
        if (employee_guest == 1){ // guest has been specified
          printf("invalid\n");
          exit(255);
        }
        employee_guest = 0;
        name = calloc(strlen(optarg) + 1, sizeof(char));
        strcpy(name, optarg);
        for (i = 0; i < strlen(name); i++){
          if (isalpha(name[i])==0){
            printf("invalid\n");
            exit(255);
          }
        }
        //employee name
        break;

      case 'G':
        // if employee_guest is 0 error out
        // otherwise set employee_guest to 1
        // read in name
        if (employee_guest == 0){ // employee has been specified
          printf("invalid\n");
          exit(255);
        }
        employee_guest = 1;
        name = calloc(strlen(optarg) + 1, sizeof(char));
        strcpy(name, optarg);
        for ( i = 0; i < strlen(name); i++){
          if (isalpha(name[i])==0){
            printf("invalid\n");
            exit(255);
          }
        }
        //guest name
        break;

      case 'R':
        // read in integer with sscanf, check it returns 1 otherwise error out
        // check that the room number is >= 0 and <= 1,073,741,823
        for ( i = 0; i < strlen(optarg); i++){
          if (!isdigit(optarg[i])){
            printf("invalid\n");
            exit(255);
          }
        }
        if (sscanf(optarg, "%d", &roomid) == 0){
          printf("invalid\n");
          exit(255);
        }
        if ( (roomid < 0) || (roomid > 1073741823) ){
          printf("invalid\n");
          exit(255);
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
  //pick up the positional argument for log path
  if(optind < argc) {
    logpath = calloc(strlen(argv[optind]) + 1, sizeof(char));
    strcpy(logpath, argv[optind]);
  }
  if (optind != argc - 1){
    printf("invalid\n");
    exit(255);
  }
  if ( (timestamp == -1) || (arrival_departure == -1) || (employee_guest == -1) ){ // if we didnt read in a timestmap, or arrival/departure, or employee/guest error out
    printf("invalid\n");
    exit(255);
  }
  if ( (token==NULL) || (name == NULL) || (logpath == NULL) ){ // if token or name or logpath weren't read in error out
    printf("invalid\n");
    exit(255);
  }

  // printf("timestamp: %d\ntoken: %s\narrival_departure: %d\nemployee_guest: %d\nname: %s\nlogpath:%s\nroomid:%d\n",timestamp,token,arrival_departure,employee_guest,name,logpath,roomid);

  newlogentry = calloc(4096+strlen(name), sizeof(char));
  sprintf(newlogentry, "%d,%d,%s,%d,%d\n",timestamp,employee_guest,name,arrival_departure,roomid); // CONSTRUCT NEW LOG ENTRY

  // printf("%s\n",newlogentry );

  // CREATE THE MAP
  map_str_t m;
  map_init(&m);

  // END MAP CREATION

  if ( ( inputlogfile = fopen(logpath, "r") ) ) {              //log already exists
    char *log_line;

    // printf("LOG EXISTS\n");

    fseek(inputlogfile, 0L, SEEK_END);
    inputlogsize = ftell(inputlogfile);
    rewind(inputlogfile);


    if (inputlogsize < 32){
      printf("invalid\n");
      exit(255);
    }

    inputlog = calloc(inputlogsize, sizeof(char));
    fread(inputlog, sizeof(char), inputlogsize, inputlogfile);
    fclose(inputlogfile);                                                 // inputlog contains the encrypted input log

    memcpy(iv, inputlog+(inputlogsize-16),16);                             // grab IV
    memcpy(tag, inputlog+(inputlogsize-32), 16);                          // grab tag
    memset(inputlog + (inputlogsize-32), '\0', sizeof(char));             // remove tag from inputlog 
    // ---------------------------------- VERIFY TAG AND DECRYPT LOG ------------------------------
    ctx = EVP_CIPHER_CTX_new();
    decryptedlog = calloc(inputlogsize * 2, sizeof(char));
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, token, iv);
    EVP_DecryptUpdate(ctx, decryptedlog, &len, inputlog, inputlogsize-32);
    decryptedlen = len;

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)) {
      printf("invalid\n");
      exit(255);
    }
    if (EVP_DecryptFinal_ex(ctx, decryptedlog+decryptedlen, &len) <= 0){
      printf("invalid\n");
      exit(255);
    }
    decryptedlen += len;

    decryptedlogdup = calloc(strlen(decryptedlog)+1, sizeof(char));
    strcpy(decryptedlogdup, decryptedlog);
        // printf("%s\n", decryptedlogdup);
    // ------------------- LOG HAS BEEN VERIFIED AND DECRYPTED INTO decryptedlog -----------------------




    // Process the log
    char *end_str;
    log_line = strtok_r(decryptedlog, "\n", &end_str);
    while (log_line != NULL){
      char *end_token;
      // printf("%s+\n", log_line);

      char *line_parser = strtok_r(log_line, ",", &end_token);
      last_timestamp = atoi(line_parser);
      line_parser = strtok_r(NULL, ",", &end_token);
      int line_employee_guest = atoi(line_parser);
      line_parser = strtok_r(NULL, ",", &end_token);
      char *line_name = calloc(strlen(line_parser) + 1, sizeof(char));
      strcpy(line_name, line_parser);
      line_parser = strtok_r(NULL, ",", &end_token);
      int line_arrival_departure = atoi(line_parser);
      line_parser = strtok_r(NULL, ",", &end_token);
      int line_roomid = atoi(line_parser);


      // store in hashmap and check against it
      char *hashkey_string = calloc(strlen(line_name)*2,sizeof(char));
      sprintf(hashkey_string, "%d%s", line_employee_guest, line_name); // Generate hashkey by combining 0 or 1 for employee / guest
      
      // printf("hashkey_string:%s\n",hashkey_string);
      map_remove(&m, hashkey_string); // remove current position

      if (line_arrival_departure == 0){
        // process the arrival
        // printf("PROCESSING ARRIVAL: ");
        // printf("%d,%d,%s,%d,%d\n",last_timestamp, line_employee_guest, line_name, line_arrival_departure, line_roomid);
        // printf("%s\n", hashkey_string);
        map_set(&m, hashkey_string, line_roomid);
        // printf("%d\n", map_get(&m, hashkey_string));

      } else{
        // process the departure
        // printf("PROCESSING DEPARTURE: ");
        // printf("%d,%d,%s,%d,%d\n",last_timestamp, line_employee_guest, line_name, line_arrival_departure, line_roomid);
        // printf("%s\n", hashkey_string);
        if (line_roomid != -1){
            map_set(&m, hashkey_string, -1);
        }
        // printf("%d\n", map_get(&m, hashkey_string));
      }
      log_line = strtok_r(NULL, "\n", &end_str);
    } 

    if (timestamp <= last_timestamp){
      printf("invalid\n");
      exit(255);
    }

    outputlog_plaintext = calloc(4096+strlen(decryptedlogdup) + strlen(newlogentry), sizeof(char));
    sprintf(outputlog_plaintext, "%s%s",decryptedlogdup,newlogentry);
    // printf("%s\n", outputlog_plaintext);
  } else {

    // LOG DOESN'T EXIST, GENERATE THE OUTPUT LOG AND IV
    for ( i = 0; i < 16; i++){
      int num = (rand() % 9);
      iv[i] = num;
    }
    outputlog_plaintext = calloc(4096+strlen(newlogentry), sizeof(char));
    sprintf(outputlog_plaintext, "%s", newlogentry);
    // printf("+%s\n", outputlog_plaintext);
  }


  // verify that the new person won't break the log requirements.
  
  // newlog_hashkey = calloc(strlen(name) * 2, sizeof(char));
  // sprintf(newlog_hashkey, "%d%s", employee_guest, name);
  // printf("%s\n",newlog_hashkey );

  // GENERATE HASHKEY FOR THE NEW LOG
  char *newlog_hashkey_string = calloc(strlen(name)*2,sizeof(char));
  sprintf(newlog_hashkey_string, "%d%s", employee_guest, name); // Generate hashkey by combining 0 or 1 for employee / guest

  // const char *key;
  // map_iter_t iter = map_iter(&m);

  // while ((key = map_next(&m, &iter))) {
  //   printf("%s -> %d\n", key, *map_get(&m, key));
  // }

  // printf("WE'VE CREATED THE HASH\n");
  // printf("%s\n", newlog_hashkey_string);
  if (map_get(&m,newlog_hashkey_string)){
    // printf("PERSON ALREDAY EXISTS\n");
    if (arrival_departure == 0){
      // printf("PERSON IS ARRIVING \n");
      // arriving, make sure the current roomid = -1 since thats the only place they can arrive from
      // printf("%s\n",newlog_hashkey_string );
      // printf("%d\n",*map_get(&m,newlog_hashkey_string) );
      if (roomid == -1){ // PERSON CANT ARRIVE IN GALLERY FROM GALLERY
        printf("invalid\n");
        exit(255);
      }
      if (*map_get(&m,newlog_hashkey_string) != -1){
        printf("invalid\n");
        exit(255);
      }
    }
    else {
      // printf("PERSON IS LEAVING \n");
      // they are departing, so make sure they are departing from the room they are in
      if (*map_get(&m,newlog_hashkey_string) != roomid){
        // printf("Actual roomid for %s is %d\n",newlog_hashkey_string,*map_get(&m,newlog_hashkey_string) );
        printf("invalid\n");
        exit(255);
      }

    }


  } else {
    // printf("PERSON DOESNT EXISTS\n");
    // we have a new person
    if (arrival_departure != 0 || roomid != -1){
      printf("invalid\n");
      exit(255);
    }
  }

  // ---------- VERIFICATION PASSED, WRITE ENCRYPTED LOG AND TAG TO FILE ---------------

  outputlog_encrypted = calloc(strlen(outputlog_plaintext) * 2, sizeof(char));

  ctx = EVP_CIPHER_CTX_new();
  EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, token, iv);
  EVP_EncryptUpdate(ctx, outputlog_encrypted, &len, outputlog_plaintext, strlen(outputlog_plaintext));
  encryptedlen = len;
  EVP_EncryptFinal_ex(ctx, outputlog_encrypted+len, &len);
  encryptedlen += len;
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
  tag[16] = 0;
  memcpy(outputlog_encrypted+encryptedlen, tag, 16*sizeof(char));
  memcpy(outputlog_encrypted+encryptedlen+16, iv, 16*sizeof(char));

  if ( (outputfile = fopen(logpath, "w")) == NULL){
    printf("invalid\n");
    exit(255);
  }

  fwrite(outputlog_encrypted, sizeof(char), encryptedlen + 32, outputfile);
  fclose(outputfile);

  // printf("%s\n", outputlog_plaintext);
  // printf("PASSED\n");
  exit(0);
  return(0);
  batch:
    batchfile = NULL;
    char *batchline;
    char *execline;
    batchline = calloc(4096, sizeof(char));
    if ( (batchfile  = fopen(batchfilepath, "r") ) == NULL ){  // batchfile doesn't exist
      printf("invalid\n");
      exit(255);
    } 

    while ( fgets(batchline,4096,batchfile) ){
      execline = calloc(32+strlen(batchline),sizeof(char));
      sprintf(execline, "./logappend %s",batchline); // CONSTRUCT NEW LOG ENTRY
      system(execline);
      free(execline);
    }

    exit(0);

}