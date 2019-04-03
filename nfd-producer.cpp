#define _GNU_SOURCE
#include <ndn-cxx/face.hpp>
#include <iostream>
#include <functional>
#include <unistd.h>
#include <stdio.h>
using std::bind;
using std::shared_ptr;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using namespace ndn;

const int DATA_BLOCK_SIZE = 1024;
uint8_t buf[4096];

class Producer
{
public:
    void
    run(char *nameStr, char *fileStr)
    {
        prefix = nameStr;
        preparePieces(fileStr);

        m_face.setInterestFilter(prefix, std::bind(&Producer::onInterest, this, _2), nullptr);

        m_face.processEvents();
    }

    Producer():m_face(){
        lastseq = 0;
    }

private:
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
    Face m_face;
    Name prefix;
    uint32_t lastseq;
    std::vector<shared_ptr<Data>> m_store;
    KeyChain m_keyChain;
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
