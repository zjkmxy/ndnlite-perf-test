TARGETS = consumer nfd-producer lite-producer

all: $(TARGETS)

consumer: consumer.cpp
	g++ -std=c++14 -lndn-lite $< -o $@ -O3

nfd-producer: nfd-producer.cpp
	g++ -std=c++14 -lndn-cxx -lboost_system $< -o $@ -O3

lite-producer: lite-producer.cpp
	g++ -std=c++14 -lndn-lite $< -o $@ -O3

clean:
	rm -rf *.dSYM
	rm $(TARGETS)
