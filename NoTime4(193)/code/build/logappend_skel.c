#define MSG_LEN 50000
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <regex.h>
#include <stdbool.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

int parse_cmdline(int argc, char *argv[]) {
  int opt = -1;
  int is_good = 0;
  int time = -1;
  int room = -1;
  int total_len;
  int olen = 0;
  int original;
  int ret;
  bool batch_provided = false;
  size_t l = 0;
  ssize_t read;
  size_t file_read;
  char arrival = 0;
  char employee;
  FILE *log;
  FILE *batch;
  FILE *cipher;
  char *logpath;
  char name[100] = {'\0'};
  char to_be_printed[200];
  char *relevant_line = NULL;
  char *batch_line;
  unsigned char token[100] = {'\0'};
  unsigned char tag[17] = {'\0'};
  unsigned char iv[12] = {'\0'};
  unsigned char encrypted[MSG_LEN] = {'\0'};
  unsigned char msg_str[MSG_LEN] = {'\0'};
  unsigned char msg_str2[MSG_LEN] = {'\0'};
  EVP_CIPHER_CTX *ctx;
  
  if(strcmp(argv[0], "batch") == 0){
    optind = 0;
  }
  //pick up the switches
  while ((opt = getopt(argc, argv, "T:K:E:G:ALR:B:")) != -1) {
    
    switch(opt) {
      case 'B':
        batch_provided = true;
        batch = fopen(optarg, "r");
        // FILE *saved_batch = fopen("saved_batch", "w");
        // char buf[2000] = {'\0'};

        // fread(buf, 1, sizeof(buf) - 1, batch);
        // fputs(buf, saved_batch);
        // fclose(saved_batch);
        
        // if(batch == NULL) {
        //   printf("invalid");
        //   return 255;
        // }
        // rewind(batch);
        
        while ((read = getline(&batch_line, &l, batch)) != -1) {
          int alt_argc = 1;
          char *alt_argv[20];
          char *pch;
          
          
          alt_argv[0] = "batch";
          
          pch = strtok (batch_line," \n");
          while (pch != NULL){
            //printf("%s\n", pch);
            alt_argv[alt_argc++] = pch;
            pch = strtok (NULL, " \n");
          }
          original = optind;
          parse_cmdline(alt_argc, alt_argv);
          optind = original;
        }
        
        fclose(batch);
        
        break;
      
      case 'T':
        //timestamp
        if(sscanf(optarg, "%d", &time) != 1){
          printf("invalid");
          return 255;
        }
        
        if(time < 1 || time > 1073741823){
          printf("invalid");
          return 255;
        }
        
        break;
      
      case 'K':
        //secret token
        strcpy(token, optarg);
        break;
      
      case 'A':
        //arrival
        arrival = 'A';
        break;
      
      case 'L':
        //departure
        arrival = 'L';
        break;
      
      case 'E':
        //employee name
        employee = 'E';
        strcpy(name, optarg);
        break;
      
      case 'G':
        //guest name
        employee = 'G';
        strcpy(name, optarg);
        break;
      
      case 'R':
        //room ID
        if(sscanf(optarg, "%d", &room) != 1){
          printf("invalid");
          return 255;
        }
        
        if(room < 0 || room > 1073741823){
          printf("invalid");
          return 255;
        }
        
        break;
      
      default:
        //unknown option, leave
        
        printf("invalid");
        return 255;
        break;
    }
    
  }
  
  if(!batch_provided){
    if(time == -1 || token == NULL || name == NULL || arrival == 0){
      printf("invalid");
      return 255;
    }
    
    //pick up the positional argument for log path
    if(optind == argc - 1) { 
      logpath = argv[optind];
    }else{
      printf("invalid");
      return 255;
    }
    
    if(access(logpath, F_OK) != -1){
      int i;
      log = fopen(logpath, "r");
      file_read = fread(encrypted, 1, sizeof(encrypted) - 1, log);
      
      //strcpy(tag, encrypted + file_read - 16);
      for(i = 0; i < 16; i++){
        tag[i] = *(encrypted + file_read - 16 + i);
        //printf("%c", tag[i]);
      }
      encrypted[file_read - 16] = '\0';
      
      //printf("\nEncrypt tag: %s\n", tag);
      //printf("Encrypted: %s\n", encrypted);
      ctx = EVP_CIPHER_CTX_new();
      EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
      
      EVP_DecryptInit_ex(ctx, NULL, NULL, token, iv);
      
      EVP_DecryptUpdate(ctx, msg_str, &olen, encrypted, file_read - 16);
      total_len = olen;
      
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
      
      ret = EVP_DecryptFinal_ex(ctx, msg_str + olen, &olen);
      
      msg_str[total_len] = '\0';
      
      EVP_CIPHER_CTX_free(ctx);
      
      //printf("%s\n", msg_str);
      //printf("%s\n", tag);
      if(ret > 0){
        total_len += olen;
      }else{
        printf("invalid");
        return 255;
      }



      /*ctx = EVP_CIPHER_CTX_new();
      EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, NULL, NULL);

      EVP_DecryptInit_ex(ctx, NULL, NULL, token, iv);

      EVP_DecryptUpdate(ctx, msg_str, &olen, encrypted, file_read);
      total_len = olen;
      ret = EVP_DecryptFinal_ex(ctx, msg_str + olen, &olen);
      total_len += olen;
      msg_str[total_len] = '\0';

      printf("%s", msg_str);
      EVP_CIPHER_CTX_free(ctx);

      if(ret > 0){
        total_len += olen;
      }else{
        printf("invalid");
        return 255;
      }*/
      
      fclose(log);
    }else{
      if(arrival == 'L' || room >= 0){
        printf("invalid");
        return 255;
      }
    }
    
    strcpy(msg_str2, msg_str);
    char *line = strtok(msg_str2, "\n");
    while(line){
      /*Group 0: useless
      Group 1: timestamp
      Group 2: A/L
      Group 3: E/G
      Group 4: name
      Group 5: useless
      Group 6: room*/
      
      regex_t re;
      size_t groups = 7;
      regmatch_t groupArray[groups];
      char group_chars[groups][60];
      int g;
      
      int i = regcomp(&re, "^T: ([0-9]+) (A|L): (E|G) \"([A-Za-z]+)\"( R: ([0-9]+))?$", REG_EXTENDED);
      
      i = regexec(&re, line, groups, groupArray, 0);
      
      if(i != 0){
        printf("invalid");
        return 255;
      }
      
      for(g = 0; g < groups; g++){
        if (groupArray[g].rm_so == (size_t)-1){
          break;
        }
        
        strncpy(group_chars[g], line + groupArray[g].rm_so, groupArray[g].rm_eo - groupArray[g].rm_so);
        group_chars[g][groupArray[g].rm_eo - groupArray[g].rm_so] = 0;
        
      }
      
      regfree(&re);
      
      if(strcmp(group_chars[4], name) == 0 && *group_chars[3] == employee){
        relevant_line = line;
      }
      
      if(time <= atoi(group_chars[1])){
        printf("invalid");
        return 255;
      }
      
      line  = strtok(NULL, "\n");
    }
    
    if(relevant_line == NULL){
      //name has not been seen before
      if(arrival == 'L' || room >= 0){
        printf("invalid");
        return 255;
      }
    }else{
      //name has been seen before
      regex_t re;
      size_t groups = 7;
      regmatch_t groupArray[groups];
      char group_chars[groups][60];
      bool room_line = true;
      int g;
      
      int i = regcomp(&re, "^T: ([0-9]+) (A|L): (E|G) \"([A-Za-z]+)\"( R: ([0-9]+))?$", REG_EXTENDED);
      
      i = regexec(&re, relevant_line, groups, groupArray, 0);
      
      for(g = 0; g < groups; g++){
        if (groupArray[g].rm_so == (size_t)-1){
          if(g == 5){
            room_line = false;
          }
          break;
        }
        
        strncpy(group_chars[g], relevant_line + groupArray[g].rm_so, groupArray[g].rm_eo - groupArray[g].rm_so);
        group_chars[g][groupArray[g].rm_eo - groupArray[g].rm_so] = 0;
        //printf("%s\n", group_chars[g]);
      }
      
      regfree(&re);
      
      if(arrival == 'A'){
        if(*group_chars[2] == 'A' && room_line){
          //arrived to room
          printf("invalid");
          return 255;
        }else if(*group_chars[2] == 'A' && !room_line){
          //arrived to gallery
          if(room == -1){
            printf("invalid");
            return 255;
          }
        }else if(*group_chars[2] == 'L' && room_line){
          //left a room
          if(room == -1){
            printf("invalid");
            return 255;
          }
        }else{
          //left gallery
          if(room >= 0){
            printf("invalid");
            return 255;
          }
        }
      }else{
        if(*group_chars[2] == 'A' && room_line){
          //arrived to room
          if(atoi(group_chars[6]) != room){
            printf("invalid");
            return 255;
          }
        }else if(*group_chars[2] == 'A' && !room_line){
          //arrived to gallery
          if(room >= 0){
            printf("invalid");
            return 255;
          }
        }else if(*group_chars[2] == 'L' && room_line){
          //left a room
          if(room >= 0){
            printf("invalid");
            return 255;
          }
        }else{
          printf("invalid");
          return 255;
        }
      }
    }
    
    
    if(room >= 0){
      sprintf(to_be_printed, "T: %d %c: %c \"%s\" R: %d\n", time, arrival, employee, name, room);
    }else{
      sprintf(to_be_printed, "T: %d %c: %c \"%s\"\n", time, arrival, employee, name);
    }
    
    
    /*printf("before:\n");
    printf("%s\n", msg_str);
    strcat(msg_str, to_be_printed);
    printf("after:\n");
    printf("%s\n", msg_str);*/

    strcat(msg_str, to_be_printed);
    
    int i;
    
    ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
    
    EVP_EncryptInit_ex(ctx, NULL, NULL, token, iv);
    
    EVP_EncryptUpdate(ctx, encrypted, &olen, msg_str, strlen(msg_str));
    total_len = olen;
    EVP_EncryptFinal_ex(ctx, encrypted + olen, &olen);
    total_len += olen;
    encrypted[total_len] = '\0';
    
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    
    for(i = total_len; i < total_len + 16; i++){
      encrypted[i] = tag[i - total_len];
      //printf("%c", encrypted[i]);
    }

    //printf(tag);

    encrypted[total_len + 16] = '\0';
   
    cipher = fopen(logpath, "w");
    fwrite(encrypted, 1, total_len + 16, cipher);
    
    EVP_CIPHER_CTX_free(ctx);

    /*ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, NULL, NULL);

    EVP_EncryptInit_ex(ctx, NULL, NULL, token, iv);

    EVP_EncryptUpdate(ctx, encrypted, &olen, msg_str, strlen(msg_str));
    total_len = olen;
    EVP_DecryptFinal_ex(ctx, encrypted + olen, &olen);
    total_len += olen;
    encrypted[total_len] = '\0';

    cipher = fopen(logpath, "w");
    fputs(encrypted, cipher);

    EVP_CIPHER_CTX_free(ctx);*/
    
    fclose(cipher);
  }
  
  return is_good;
}


int main(int argc, char *argv[]) {
  
  int result;
  result = parse_cmdline(argc, argv);
  
  return result;
}

