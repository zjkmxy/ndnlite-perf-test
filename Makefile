TARGETS = consumer nfd-producer lite-producer file-gen

all: $(TARGETS)

consumer: consumer.cpp common.h
	g++ -std=c++14 $< -o $@ -O3 -lndn-lite -lpthread

nfd-producer: nfd-producer.cpp common.h
	g++ -std=c++14 $< -o $@ -O3 -lndn-cxx -lboost_system

lite-producer: lite-producer.cpp common.h mem-monitor.hpp mem-usage.hpp
	g++ -std=c++14 $< -o $@ -O3 -lndn-lite

file-gen: file-gen.cpp common.h
	g++ -std=c++14 $< -o $@ -O3

25600.testfile: file-gen
	./file-gen 25600.testfile 25600

run-lite: lite-producer 25600.testfile
	./lite-producer /example/testApp/randomData ./25600.testfile

run-nfd: nfd-producer 25600.testfile
	./nfd-producer /example/testApp/randomData ./25600.testfile

run-consumer: consumer
	./consumer /example/testApp/randomData 25600

clean:
	rm -rf *.dSYM
	rm -rf $(TARGETS)
	rm -rf *.testfile
