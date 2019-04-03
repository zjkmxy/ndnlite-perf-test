#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <ndn-lite.h>

#define DATA_BLOCK_SIZE 1024
#define DATA_CHUNK_SIZE (DATA_BLOCK_SIZE + 256)
#define MAX_CHUNKS_NUM 25610

ndn_name_t name_prefix;
uint8_t buf[4096];
uint8_t chunks[MAX_CHUNKS_NUM][DATA_CHUNK_SIZE];
size_t chunk_sizes[MAX_CHUNKS_NUM];
uint32_t chunks_num = 0;
ndn_unix_face_t *face;
bool running;

int parse_args(int argc, char *argv[]){
  if(argc < 3){
    fprintf(stderr, "ERROR: wrong arguments.\n");
    printf("Usage: <name-prefix> <file>\n");
    return 1;
  }
  if(ndn_name_from_string(&name_prefix, argv[1], strlen(argv[1])) != NDN_SUCCESS){
    fprintf(stderr, "ERROR: wrong name.\n");
    return 2;
  }

  return 0;
}

int prepare_data(const char* filename){
  long filesize, i, cursz;
  int ret;

  FILE* file = fopen(filename, "rb");
  if(file == NULL){
    fprintf(stderr, "ERROR: file doesn't exist.\n");
    return 3;
  }

  fseek(file, 0 , SEEK_END);
  filesize = ftell(file);
  fseek(file, 0 , SEEK_SET);
  chunks_num = (filesize + DATA_BLOCK_SIZE - 1) / DATA_BLOCK_SIZE;

  for(i = 0; !feof(file); i ++){
    cursz = fread(buf, 1, DATA_BLOCK_SIZE, file);
    ret = tlv_make_data(chunks[i], DATA_CHUNK_SIZE, &chunk_sizes[i], 6,
                        TLV_DATAARG_NAME_PTR, &name_prefix,
                        TLV_DATAARG_NAME_SEGNO_U64, (uint64_t)i,
                        TLV_DATAARG_FRESHNESSPERIOD_U64, (uint64_t)10000,
                        TLV_DATAARG_FINALBLOCKID_U64, (uint64_t)(chunks_num - 1),
                        TLV_DATAARG_CONTENT_BUF, buf,
                        TLV_DATAARG_CONTENT_SIZE, (size_t)cursz);
  }

  fclose(file);
  return 0;
}

int on_interest(const uint8_t* raw_interest, uint32_t interest_size, void* userdata){
  int ret;
  uint64_t segno = 0;

  ret = tlv_parse_interest((uint8_t*)raw_interest, interest_size, 1,
                           TLV_INTARG_NAME_SEGNO_U64, &segno);
  if(ret == NDN_SUCCESS && segno < chunks_num){
    ndn_forwarder_put_data(chunks[segno], chunk_sizes[segno]);
  }

  return NDN_FWD_STRATEGY_SUPPRESS;
}

int main(int argc, char *argv[]){
  int ret;
  ndn_encoder_t encoder;

  ndn_lite_startup();

  if((ret = parse_args(argc, argv)) != 0){
    return ret;
  }
  if((ret = prepare_data(argv[2])) != 0){
    return ret;
  }

  printf("Data ready: %u chunks\n", chunks_num);

  face = ndn_unix_face_construct(NDN_NFD_DEFAULT_ADDR, false);
  if(face->intf.state != NDN_FACE_STATE_UP){
    printf("ERROR: Unable to establish unix socket: %d\n", errno);
    printf("If NDN_NFD_DEFAULT_ADDR is used, sudo is needed.\n");
    ndn_face_destroy(&face->intf);
    return -1;
  }

  running = true;
  encoder_init(&encoder, buf, sizeof(buf));
  ndn_name_tlv_encode(&encoder, &name_prefix);
  ndn_forwarder_register_prefix(encoder.output_value, encoder.offset, on_interest, NULL);
  while(running){
    ndn_forwarder_process();
    usleep(10);
  }

  ndn_face_destroy(&face->intf);

  return 0;
}
