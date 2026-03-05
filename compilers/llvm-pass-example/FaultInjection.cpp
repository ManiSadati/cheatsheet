
#include <vector>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <random>

extern "C"{
    
    // Supported fault modes.
    enum FaultType {
        BUF_OVERFLOW,
        MEM_EXHAUST,
        LOGIC_FAULT
    };
    
    // Global FI state.
    int FIcounter = 0;
    int FIindex = -1;
    FaultType FItype;

    // Pick one target instruction index for this run.
    void setRandomInjection(int nallocs, int nsubs, FaultType ft){
        FIindex = 0;
        FIcounter = 0;
        FItype = ft;
        if(FItype == LOGIC_FAULT){
            FIindex = rand() % nsubs;    
        }
        else{
            FIindex = rand() % nallocs;
        }
    }

    // malloc wrapper with optional fault injection.
    void* mallocFI(size_t size, int FnIndex){
        if(FIcounter == FnIndex){
            if(FItype == BUF_OVERFLOW){
                // Undersize allocation.
                size = size * 0.7;
                void* p = malloc(size);
                FIcounter++;
                return p;
            }
            if(FItype == MEM_EXHAUST){
                // Simulate allocation failure.
                FIcounter ++;
                return nullptr;
            }   
        }
        // only increaes the counter whenever the fault type matches this function
        if(FItype == BUF_OVERFLOW || FItype == MEM_EXHAUST)
            FIcounter ++;
        void* p = malloc(size);
        return p;
    }

    // calloc wrapper with optional fault injection.
    void *calloc(size_t nmemb, size_t size, int FnIndex){
        if(FIcounter == FnIndex){
            if(FItype == BUF_OVERFLOW){
                // Undersize allocation.
                nmemb = nmemb * 0.7;
                void* p = calloc(nmemb, size);
                FIcounter++;
                return p;
            }
            if(FItype == MEM_EXHAUST){
                // Simulate allocation failure.
                FIcounter ++;
                return nullptr;
            }   
        }
        // only increaes the counter whenever the fault type matches this function
        if(FItype == BUF_OVERFLOW || FItype == MEM_EXHAUST)
            FIcounter ++;
        void* p = calloc(nmemb, size);
        return p;
    }

    // Pre-check function for sub to see whethere it should be injected or not,
    // True only at the selected sub instruction for FI.
    bool shouldInjectSub(int FnIndex){
        if(FItype != LOGIC_FAULT)
            return false;
        FIcounter ++;
        return (FIcounter -1) == FnIndex;
    }


} // extern "C"
