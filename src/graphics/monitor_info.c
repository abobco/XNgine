#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctype.h"


Vector2 get_display_dimensions() {

  FILE *fp;
  char path[1035];
  int w =0, h =0;

  // run a shell command to get display info, read into a string
  fp = popen("/bin/fbset -s", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  // get the output 1 line at a time
  while (fgets(path, sizeof(path), fp) != NULL) {
    // discard output until the 1st digit of resolution is read
    int index = 0;
    for ( int i = 0; i < strlen(path); i ++) {
        if ( isdigit(path[i])) {
            index = i-1;
            break;
        }   
    }

    // convert resolution string to ints
    if ( index ) {
        char w_str[16], h_str[16];
        int w_index=0, h_index=0;
        while ( path[++index] != 'x') {
            w_str[w_index++] = path[index];
        }
        while ( path[++index] != '\"') {
            h_str[h_index++] = path[index];
        }

        w = atoi(w_str);
        h = atoi(h_str);
        break;
    }
  }

  /* close */
  pclose(fp);

  return (Vector2) {(float)w, (float)h};
}