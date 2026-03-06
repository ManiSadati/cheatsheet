
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
    int FIindex = -1;
    FaultType FItype;

    // Pick one target instruction index for this run.
    void setRandomInjection(size_t nallocs, size_t nsubs, size_t ft_){
        FaultType ft = (FaultType)ft_;
        FIindex = 0;
        FItype = ft;
        srand(time(NULL));
        if(FItype == LOGIC_FAULT){
            FIindex = rand() % nsubs;    
        }
        else{
            FIindex = rand() % nallocs;
        }
        printf("FIindex is %d\n",FIindex);
    }

    // malloc wrapper with optional fault injection.
    void* mallocFI(size_t size, size_t FIcounter){
        if(FIcounter == FIindex){
            if(FItype == BUF_OVERFLOW){
                // Undersize allocation.
                size = size * 0.7;
                printf("size is %zu\n",size);
                void* p = malloc(size);
                return p;
            }
            if(FItype == MEM_EXHAUST){

                printf("here lol\n");
                // Simulate allocation failure.
                return nullptr;
            }   
        }
        void* p = malloc(size);
        return p;
    }

    // calloc wrapper with optional fault injection.
    void *callocFI(size_t nmemb, size_t size, size_t FIcounter){
        if(FIcounter == FIindex){
            if(FItype == BUF_OVERFLOW){
                // Undersize allocation.
                nmemb = nmemb * 0.7;
                void* p = calloc(nmemb, size);
                return p;
            }
            if(FItype == MEM_EXHAUST){
                // Simulate allocation failure.
                printf("here clol\n");
                return nullptr;
            }   
        }
        void* p = calloc(nmemb, size);
        return p;
    }

    // Pre-check function for sub to see whethere it should be injected or not,
    // True only at the selected sub instruction for FI.
    bool shouldInjectSub(int FIcounter){
        if(FItype != LOGIC_FAULT)
            return false;
        return (FIcounter -1) == FIindex;
    }


} // extern "C"
