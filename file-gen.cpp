#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstdint>
#include "common.h"

int maxCount = 0;
uint8_t buf[DATA_BLOCK_SIZE];

int main(int argc, char *argv[]){
  if(argc < 3){
    printf("Usage: %s <filename> <segment count>\n", argv[0]);
    return 1;
  }

  if(sscanf(argv[2], "%d", &maxCount) != 1){
    fprintf(stderr, "ERROR: wrong segment count.\n");
    return 2;
  }

  FILE* fp = fopen(argv[1], "wb");
  if(fp == NULL){
    printf("Unable to open file %d\n", errno);
    return 3;
  }

  for(int i = 0; i < DATA_BLOCK_SIZE; i ++){
    buf[i] = (i & 0xFF);
  }

  for(int i = 0; i < maxCount; i ++){
    fwrite(buf, 1, DATA_BLOCK_SIZE, fp);
  }

  fclose(fp);

  return 0;
}