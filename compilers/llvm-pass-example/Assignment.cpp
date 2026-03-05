#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <cxxabi.h>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicInst.h"

using namespace llvm;
using namespace std;

// Demangle the function name
string demangle(const char *name){
    int status = -1;

    std::unique_ptr<char, void (*)(void *)> res{
        abi::__cxa_demangle(name, NULL, NULL, &status), std::free};
    return (status == 0) ? res.get() : std::string(name);
}

// Returns the source code line number corresponding to the LLVM instruction.
int getSourceCodeLine(Instruction *I) {
    llvm::DebugLoc debugInfo = I->getDebugLoc();
    
    int line = -1;
    if (debugInfo)
        line = debugInfo.getLine();
    return line;
}


// Topologically sort all the basic blocks in a function.
// Handle cycles in the directed graph using Tarjan's algorithm
// of Strongly Connected Components (SCCs).
vector<BasicBlock *> topoSortBBs(Function &F) {
  vector<BasicBlock *> tempBB;
  for (scc_iterator<Function *> I = scc_begin(&F), IE = scc_end(&F); I != IE;
       ++I) {

    // Obtain the vector of BBs in this SCC and print it out.
    const std::vector<BasicBlock *> &SCCBBs = *I;

    for (std::vector<BasicBlock *>::const_iterator BBI = SCCBBs.begin(),
                                                   BBIE = SCCBBs.end();
         BBI != BBIE; ++BBI) {

      BasicBlock *b = const_cast<llvm::BasicBlock *>(*BBI);
      tempBB.push_back(b);
    }
  }

  reverse(tempBB.begin(), tempBB.end());
  return tempBB;
}

Instruction* getNextInstruction(Instruction *I){
    if(I->isTerminator())
        return nullptr;
    BasicBlock::iterator bb_it(I);
    while(isa<PHINode>(++bb_it));
    return &*bb_it;
}


namespace {

struct Assignment5 : public ModulePass {

    static char ID;
    Assignment5() : ModulePass(ID) {}
    
};

}


static RegisterPass<Assignment5> X("faultinjection",
                                   "Pass to inject faults");