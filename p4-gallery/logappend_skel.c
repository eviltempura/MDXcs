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

int strcheck(char *str, int lc, int uc, int num) {
  int lc_ans = lc, uc_ans = uc, num_ans = num;

  for(int i = 0; i < strlen(str); i++) {
    if(str[i] >= 97 && str[i] <= 122) {
      lc_ans = lc_ans | 1;
    } else if(str[i] >= 65 && str[i] <= 90) {
      uc_ans = uc_ans | 1;
    } else if(str[i] >= 48 && str[i] <= 57) {
      num_ans = num_ans | 1;
    } else {
      return 0;
    }
  }

  return (lc == lc_ans) && (uc == uc_ans) && (num == num_ans);
}

int parse_cmdline(int argc, char *argv[], struct Log *log) {

  int opt = -1;
  int is_good = 1;
  int is_emp = -1;
  int is_arr = -1;
  char* logpath;

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
        if(strcheck(argv[optind-1],0,0,1)) {
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
        if(strcheck(argv[optind-1],1,1,1)) {
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
        if(strcheck(argv[optind-1],1,1,0)) {
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
        if(strcheck(argv[optind-1],1,1,0)) {
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
        if(strcheck(argv[optind-1],0,0,1)) {
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
    logpath = argv[optind];
  }

  return is_good;
}


int main(int argc, char *argv[]) {

  int result;
  struct Log log;
  result = parse_cmdline(argc, argv, &log);

  if(result == 0) {
    printf("invalid\n");
    exit(255);
  }

  printf("Timestamp: %d, Token: %s\n", log.timestamp, log.token);
  printf("This person is a(n) %s ", log.is_emp==1?"employee":"guest");
  printf("who just %s room no.%d\n", log.is_arr==1?"arrived":"left",log.int_room);
  return 0;
}
