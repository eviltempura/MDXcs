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


int main(int argc, char *argv[]) {

  int result;
  struct Log log = {.timestamp = -999,
                    .token     = "@",
                    .is_emp    = -999,
                    .name      = "@",
                    .is_arr    = -999,
                    .int_room  = -999,
                    .char_room = "@",
                    .logpath   = "@"};
  result = parse_cmdline(argc, argv, &log);

  if(result == 0) {
    printf("invalid\n");
    exit(255);
  }

  /*TODO: should print invalid and exit with 255
  if some necessary arguments are missing
  optional argument: -B,-R*/
  if(log.timestamp == -999 || strcmp(log.token,"@") == 0
  || log.is_emp == -999    || strcmp(log.name,"@") == 0
  || log.is_arr == -999    || strcmp(log.logpath,"@") == 0) {
    printf("invalid\n");
    exit(255);
  }

  printf("Timestamp: %d, Token: %s\n", log.timestamp, log.token);
  printf("%s is a(n) %s ", log.name, log.is_emp==1?"employee":"guest");
  printf("who just %s room no.%d\n", log.is_arr==1?"arrived":"left",log.int_room);
  printf("Log has been appended to %s\n", log.logpath);
  return 0;
}
