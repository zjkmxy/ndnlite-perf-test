#define _GNU_SOURCE
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/transport/unix-transport.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <iostream>
#include <functional>
#include <unistd.h>
#include <stdio.h>
#include "common.h"
#include "mem-monitor.hpp"
using std::bind;
using std::shared_ptr;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using namespace ndn;

//#define MONITOR_MEMORY

#ifdef MONITOR_MEMORY
static MemMonitor mem_monitor;
#endif

uint8_t buf[DATA_CHUNK_SIZE];

class Producer
{
public:
    void
    run(char *nameStr, char *fileStr)
    {
        prefix = nameStr;
        preparePieces(fileStr);

        m_face.setInterestFilter(prefix, std::bind(&Producer::onInterest, this, _2), nullptr);
#ifdef MONITOR_MEMORY
        m_scheduler.scheduleEvent(10_ms, [this]{ recordMem(); });
#endif
        //m_face.processEvents();
        m_ioService.run();
    }

    Producer():
    m_face(m_ioService),
    m_scheduler(m_ioService)
    {
        lastseq = 0;
    }

private:
    void
    recordMem(){
#ifdef MONITOR_MEMORY
        mem_monitor.record();
#endif
        m_scheduler.scheduleEvent(10_ms, [this]{ recordMem(); });
    }

    void
    preparePieces(char *filename){
        ssize_t filesize;
        size_t cursz;
        Name::Component finalBlockId;

        FILE* file = fopen(filename, "rb");
        if(file == NULL){
            fprintf(stderr, "ERROR: file doesn't exist.\n");
            throw std::exception();
        }

        fseek(file, 0 , SEEK_END);
        filesize = ftell(file);
        fseek(file, 0 , SEEK_SET);
        lastseq = (filesize + DATA_BLOCK_SIZE - 1) / DATA_BLOCK_SIZE - 1;

#ifdef MONITOR_MEMORY
        mem_monitor.record();
#endif

        finalBlockId = Name::Component::fromSegment(lastseq);
        for(int i = 0; !feof(file); i ++){
            cursz = fread(buf, 1, DATA_BLOCK_SIZE, file);

            auto data = make_shared<Data>(Name(prefix).appendSegment(i));
            data->setFreshnessPeriod(10_ms);
            data->setFinalBlock(finalBlockId);
            data->setContent(buf, cursz);
            m_keyChain.sign(*data, security::SigningInfo(security::SigningInfo::SignerType::SIGNER_TYPE_SHA256));

            m_store.push_back(data);
        }

#ifdef MONITOR_MEMORY
        mem_monitor.record();
#endif

        fclose(file);

        printf("Data ready: %u chunks\n", lastseq + 1);
    }

    void
    onInterest(const Interest& interest){
        uint32_t segno = interest.getName()[-1].toSegment();
        if(segno > lastseq){
            return;
        }
        m_face.put(*m_store[segno]);
    }
    
private:
    Name prefix;
    uint32_t lastseq;
    std::vector<shared_ptr<Data>> m_store;
    boost::asio::io_service m_ioService;
    Face m_face;
    KeyChain m_keyChain;
    Scheduler m_scheduler;
};

int
main(int argc, char** argv)
{
    Producer producer;
    try {
        if(argc > 2){
            producer.run(argv[1], argv[2]);
        }else{
            std::cout << "Usage: " << argv[0] << " <name-prefix> <file>" << std::endl;
          
            return 0;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    std::cout << "Ended" << std::endl;
    return 0;
}
