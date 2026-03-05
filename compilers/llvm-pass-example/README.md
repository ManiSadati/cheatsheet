this is an example of a llvm pass 


clang++ --shared -fPIC HeapChecker.cpp -o HeapChecker.so
clang++ -fPIC -shared Assignment3.cpp -o TrackAnalysis.so   `llvm-config --cxxflags --ldflags --system-libs --libs core`

clang++ -O0 -g -S -emit-llvm -fno-discard-value-names -o Quicksort.ll -c Quicksort.cpp
opt -S -enable-new-pm=0 -load ../../TrackAnalysis.so -track_analysis < HelloWorld.ll > HelloWorld_Instr.ll
clang HelloWorld_Instr.ll ../../HeapChecker.so -o HelloWorld