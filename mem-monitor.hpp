#include <cstdio>
#include <chrono>
#include "mem-usage.hpp"
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;

class MemMonitor {
private:
  FILE* hfile;
  steady_clock::time_point tbegin;

public:
  MemMonitor(){
    hfile = fopen("./mem-data.txt", "w");
    tbegin = steady_clock::now();
    record();
  }
  ~MemMonitor(){
    record();
    fclose(hfile);
  }

  void record(){
    steady_clock::time_point tcur = steady_clock::now();
    fprintf(hfile, "%lf %llu\n", duration_cast<duration<double>>(tcur - tbegin).count(), MemUsage::Get());
  }
};
