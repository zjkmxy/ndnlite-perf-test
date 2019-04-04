TARGETS = consumer nfd-producer lite-producer file-gen

all: $(TARGETS)

consumer: consumer.cpp common.h
	g++ -std=c++14 -lndn-lite $< -o $@ -O3

nfd-producer: nfd-producer.cpp common.h
	g++ -std=c++14 -lndn-cxx -lboost_system $< -o $@ -O3

lite-producer: lite-producer.cpp common.h
	g++ -std=c++14 -lndn-lite $< -o $@ -O3

file-gen: file-gen.cpp common.h
	g++ -std=c++14 $< -o $@ -O3

testfile: file-gen
	./file-gen 25600.testfile 25600

run-lite: lite-producer testfile
	./lite-producer /example/testApp/randomData ./25600.testfile

run-nfd: nfd-producer testfile
	./nfd-producer /example/testApp/randomData ./25600.testfile

run-consumer: consumer
	./consumer /example/testApp/randomData 25600

clean:
	rm -rf *.dSYM
	rm -rf $(TARGETS)
	rm -rf *.testfile
