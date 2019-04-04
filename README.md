Compare the performance between NDN-Lite and NFD
================================================

Here are some programs which are used to compare the performace with NDN-Lite's forwarder and NFD.

Install NFD
===========
Both ndn-cxx and NFD are compiled with `-O0` by default, which may lead to an unfair comparision.
So I install the latest release version 0.6.5 with optimized compiler flag.

The following commands are what I used exactly. 
I linked OpenSSL manually because it cannot be found automatically on my Mac,
and I applied `nfdpatch.diff` to make NFD-0.6.5 work with Boost-1.69.0.
The `master` branch of ndn-cxx and NFD have fixed this bug.
```bash
git clone https://github.com/named-data/ndn-cxx.git --tags
cd ndn-cxx
git checkout ndn-cxx-0.6.5
CXXFLAGS="-O2" ./waf configure --with-openssl=/usr/local/Cellar/openssl/1.0.2r
./waf
sudo ./waf install
cd ..

git clone https://github.com/named-data/NFD.git --recursive --tags
cd NFD
git checkout NFD-0.6.5
CXXFLAGS="-O2" ./waf configure
patch -p1 < ../nfdpatch.diff
./waf
sudo ./waf install
cd ..
```

Install NDN-Lite
================
```bash
git clone https://github.com/named-data-iot/ndn-iot-package-over-posix.git --recursive
cd ndn-iot-package-over-posix
vim ndn-lite/ndn-constants.h
  :%s/NDN_CONTENT_BUFFER_SIZE 256/NDN_CONTENT_BUFFER_SIZE 2048/g
  :wq
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DDYNAMIC_LIB=YES ..
make -j4
sudo make install
```

Run the test
============
To run the NFD producer:
```bash
nfd-start
make run-nfd
```

To run the NDN-Lite producer:
```bash
nfd-stop
sudo make run-lite
```

The NDN-Lite producer and NFD use the same Unix socket file `/var/run/nfd.sock`,
so don't run both at the same time.

To run the consumer:
```bash
make run-consumer
```
