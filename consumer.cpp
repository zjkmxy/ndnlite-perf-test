#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <thread>
#include <ndn-lite.h>

const int maxCount = 25603;
const char* namePrefix = "/example/testApp/randomData";
int sock;
uint8_t buf[4096];
uint8_t interest[maxCount][70];
size_t interest_size[maxCount];

void receive(){
  int i = 0;
  ssize_t ret = 0;
  while(i < maxCount){
    ret = recv(sock, buf, i < 0x100 ? 1120 : 1121, 0);
    i ++;
  }
}

inline double timediff(timespec t1, timespec t2){
  return (t1.tv_sec - t2.tv_sec) * 1.0 + (t1.tv_nsec - t2.tv_nsec) / 1000000000.0;
}

int main(){
  sockaddr_un nfdaddr;
  
  // Connect
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  nfdaddr.sun_family = AF_UNIX;
  strcpy(nfdaddr.sun_path, NDN_NFD_DEFAULT_ADDR);
  
  if(connect(sock, (sockaddr*)&nfdaddr, sizeof(nfdaddr)) != 0){
    printf("ERROR: connect() failed %d\n", errno);
    return -1;
  }
  
  // Prepare
  srand(time(0));
  ndn_name_t name;
  ndn_name_from_string(&name, namePrefix, strlen(namePrefix));
  for(int i = 0; i < maxCount; i ++){
    int ret = tlv_make_interest(interest[i], sizeof(interest[i]), &interest_size[i], 5,
                                TLV_INTARG_NAME_PTR, &name,
                                TLV_INTARG_NAME_SEGNO_U64, (uint64_t)i,
                                TLV_INTARG_CANBEPREFIX_BOOL, true,
                                TLV_INTARG_MUSTBEFRESH_BOOL, true,
                                TLV_INTARG_LIFETIME_U64, 60000);

    if(ret != NDN_SUCCESS){
      printf("Error: %d\n", ret);
    }
  }
  
  timespec tmStart, tmEnd;
  clock_gettime(CLOCK_REALTIME, &tmStart);

  std::thread recver(receive);
  for(int i = 0; i < maxCount; i ++){
    ssize_t ret = send(sock, &interest[i][0], interest_size[i], 0);
    if(ret != interest_size[i]){
      printf("ERROR: sendto failed.\n");
    }
  }
  recver.join();

  clock_gettime(CLOCK_REALTIME, &tmEnd);
  printf("Time past: %lf\n", timediff(tmEnd, tmStart));
  
  return 0;
}
