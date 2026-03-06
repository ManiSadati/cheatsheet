this is an example of a llvm pass 

```
clang++ --shared -fPIC FaultInjection.cpp -o FaultInjection.so

clang++ -fPIC -shared Assignment.cpp -o FaultInjection.so   `llvm-config --cxxflags --ldflags --system-libs --libs core`


clang++ -O0 -g -S -emit-llvm -fno-discard-value-names -o Quicksort.ll -c Quicksort.cpp
opt -S -enable-new-pm=0 -load ../../Assignment.so -faultinjection < Quicksort.ll > Quicksort_Instr.ll
clang Quicksort_Instr.ll ../../FaultInjection.so -o Quicksort

```
for the first two, change the malloc and calloc functions however, the sub function cannot be changed, so what should  i do?  I think the best way is to set a function called presub where it calculates whether this sub should be injected/turned into add , and just return true/false, and based on that I either do add or sub.