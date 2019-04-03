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
#include <pthread.h>
#include <ndn-lite.h>

const int INTEREST_BUFSIZE = 100;
ndn_name_t namePrefix;
int maxCount;
int sock;
uint8_t buf[4096];
uint8_t (*interest)[INTEREST_BUFSIZE];
size_t (*interest_size);

class thread : public std::thread{
public:
  template <class _Fp>
  thread(_Fp f):std::thread(f){}
  void setScheduling(int policy, int priority) {
      int ret = 0;
      sch_params.sched_priority = priority;
      if((ret = pthread_setschedparam(native_handle(), policy, &sch_params)) != 0) {
        printf("Failed to set scheduling: %d\n", ret);
      }
  }
private:
  sched_param sch_params;
};

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

int parse_args(int argc, char *argv[]){
  if(argc < 3){
    fprintf(stderr, "ERROR: wrong arguments.\n");
    printf("Usage: %s <name-prefix> <segment-count>\n", argv[0]);
    return 1;
  }
  if(ndn_name_from_string(&namePrefix, argv[1], strlen(argv[1])) != NDN_SUCCESS){
    fprintf(stderr, "ERROR: wrong name.\n");
    return 2;
  }
  if(sscanf(argv[2], "%d", &maxCount) != 1){
    fprintf(stderr, "ERROR: wrong segment count.\n");
    return 3;
  }

  interest = new uint8_t[maxCount][INTEREST_BUFSIZE];
  interest_size = new size_t[maxCount];
  if(!interest || !interest_size){
    fprintf(stderr, "ERROR: insufficient memory.\n");
    return 4;
  }

  return 0;
}

int main(int argc, char *argv[]){
  sockaddr_un nfdaddr;
  int ret;

  // Parse args
  if((ret = parse_args(argc, argv)) != 0){
    return ret;
  }

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
  for(int i = 0; i < maxCount; i ++){
    int ret = tlv_make_interest(interest[i], sizeof(interest[i]), &interest_size[i], 5,
                                TLV_INTARG_NAME_PTR, &namePrefix,
                                TLV_INTARG_NAME_SEGNO_U64, (uint64_t)i,
                                TLV_INTARG_CANBEPREFIX_BOOL, true,
                                TLV_INTARG_MUSTBEFRESH_BOOL, true,
                                TLV_INTARG_LIFETIME_U64, 60000);

    if(ret != NDN_SUCCESS){
      printf("Error: tlv_make_interest failed %d\n", ret);
    }
  }
  
  timespec tmStart, tmEnd;
  clock_gettime(CLOCK_REALTIME, &tmStart);

  thread recver(receive);
  recver.setScheduling(SCHED_FIFO, 50);
  for(int i = 0; i < maxCount; i ++){
    ssize_t ret = send(sock, &interest[i][0], interest_size[i], 0);
    if(ret != interest_size[i]){
      printf("ERROR: sendto failed.\n");
    }
  }
  recver.join();

  clock_gettime(CLOCK_REALTIME, &tmEnd);
  printf("Time past: %lf\n", timediff(tmEnd, tmStart));
  
  delete[] interest;
  delete[] interest_size;

  return 0;
}
