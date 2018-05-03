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

int parse_cmdline(int argc, char *argv[]) {

  int opt = -1;
  int is_good = -1;
  int timestamp;

  char* logpath;


  //pick up the switches
  while ((opt = getopt(argc, argv, "T:K:E:G:ALR:B:")) != -1) {
    switch(opt) {
      case 'B':
        //batch file
        break;

      case 'T':
        printf("%s\n",argv[optind-1]);
        printf("%d\n",strcheck(argv[optind-1],1,1,1));
        break;

      case 'K':
        //secret token
        break;

      case 'A':
        //arrival
        break;

      case 'L':
        //departure
        break;

      case 'E':
        //employee name
        break;

      case 'G':
        //guest name
        break;

      case 'R':
        //room ID
        break;

      default:
        //unknown option, leave
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
  result = parse_cmdline(argc, argv);




  return 0;
}
