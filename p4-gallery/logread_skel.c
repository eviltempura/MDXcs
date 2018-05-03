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




int main(int argc, char *argv[]) {
  int   opt,len;
  char  *logpath = NULL;


  while ((opt = getopt(argc, argv, "K:PSRE:G:VT")) != -1) {
    switch(opt) {
      case 'T':
        break;

      case 'V':
        break;

      case 'P':
        break;

      case 'K':
        break;

      case 'S':
        break;

      case 'R':
        break;

      case 'E':

      case 'G':
        break;
    }
  }

  if(optind < argc) {
    logpath = argv[optind];
  }

}
