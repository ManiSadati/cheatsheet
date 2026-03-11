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
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/CommandLine.h"

// Global pass option
static llvm::cl::opt<int> FItype(
    "fi-type",
    llvm::cl::desc("type/type for fault injection"),
    llvm::cl::init(0));

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

struct Assignment : public ModulePass {

    static char ID;
    Assignment() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {

        LLVMContext &C = M.getContext();

        // void setRandomInjection(size_t nallocs, size_t nsubs, FaultType ft)
        FunctionCallee setRandomInjectionFn = M.getOrInsertFunction("setRandomInjection", Type::getVoidTy(C), Type::getInt64Ty(C), Type::getInt64Ty(C), Type::getInt64Ty(C));
        FunctionCallee mallocFIFn = M.getOrInsertFunction("mallocFI", Type::getInt8PtrTy(C), Type::getInt64Ty(C), Type::getInt64Ty(C));
        FunctionCallee callocFIFn = M.getOrInsertFunction("callocFI", Type::getInt8PtrTy(C), Type::getInt64Ty(C), Type::getInt64Ty(C), Type::getInt64Ty(C));
        FunctionCallee shouldInjectSubFn = M.getOrInsertFunction("shouldInjectSub", Type::getInt32Ty(C), Type::getInt64Ty(C));

        
        int alloc_counter = 0;
        int sub_counter = 0;
        for (Function &F: M){
            for (BasicBlock &BB: F){
                for (BasicBlock::const_iterator It = BB.begin(); It != BB.end();) {
                    Instruction *I = const_cast<llvm::Instruction *>(&*It);
                    It++;
                    if (CallInst *CI = dyn_cast<CallInst>(I)){
                        if(CI->getCalledFunction()){
                            // change malloc and calloc with their FI version.
                            // this will add a new arguemtn to the last 
                            if(CI->getCalledFunction()->getName() == "malloc"){
                                IRBuilder<> IRB(CI);
                                Value *FIcounter =  ConstantInt::get(IRB.getInt64Ty(), alloc_counter++);

                                CallInst *mallocFICallI = IRB.CreateCall(mallocFIFn, {CI->getArgOperand(0), FIcounter});
                                
                                CI->replaceAllUsesWith(mallocFICallI);
                                CI->eraseFromParent();
                            }
                            if(CI->getCalledFunction()->getName() == "calloc"){
                                IRBuilder<> IRB(CI);
                                Value *FIcounter =  ConstantInt::get(IRB.getInt64Ty(), alloc_counter++);

                                CallInst *callocFICallI = IRB.CreateCall(callocFIFn, {CI->getArgOperand(0), CI->getArgOperand(1), FIcounter});
                                
                                CI->replaceAllUsesWith(callocFICallI);
                                CI->eraseFromParent();
                            }
                        }
                    }
                    if(BinaryOperator *Sub = dyn_cast<BinaryOperator>(I)){
                        if(Sub->getOpcode() == Instruction::Sub){
                            IRBuilder<> IRB(Sub);
                            Value *FIcounter =  ConstantInt::get(IRB.getInt64Ty(), sub_counter++);
                            CallInst *shouldInjectSubCallI = IRB.CreateCall(shouldInjectSubFn, {FIcounter});
                            
                            Instruction *afterSub = getNextInstruction(Sub);
                            if (!afterSub)
                                continue;

                            Value *ShouldInject =
                                IRB.CreateICmpNE(shouldInjectSubCallI,
                                                 ConstantInt::get(shouldInjectSubCallI->getType(), 0));

                            Instruction *ThenTerm = nullptr;
                            Instruction *ElseTerm = nullptr;
                            SplitBlockAndInsertIfThenElse(ShouldInject, afterSub, &ThenTerm, &ElseTerm);

                            IRBuilder<> ThenBuilder(ThenTerm);
                            BinaryOperator *Add = cast<BinaryOperator>(
                                ThenBuilder.CreateBinOp(Instruction::Add, Sub->getOperand(0), Sub->getOperand(1)));

                            BasicBlock *MergeBB = afterSub->getParent();
                            IRBuilder<> MergeBuilder(&*MergeBB->getFirstInsertionPt());
                            PHINode *MergedResult = MergeBuilder.CreatePHI(Sub->getType(), 2);
                            MergedResult->addIncoming(Add, ThenTerm->getParent());
                            MergedResult->addIncoming(Sub, ElseTerm->getParent());

                            Sub->replaceUsesWithIf(MergedResult, [&](Use &U) {
                                return U.getUser() != MergedResult;
                            });
                        }
                    }
                }
            }
        }



        Function *Main = M.getFunction("main");
        if (!Main || Main->isDeclaration())
            return true;

        for (BasicBlock &BB : *Main) {
            for (BasicBlock::const_iterator It = BB.begin(); It != BB.end();) {
                Instruction *I = const_cast<llvm::Instruction *>(&*It);
                It++;
                if (I->isTerminator() || isa<PHINode>(I))
                    continue;


                Instruction *InsertBefore = I->getNextNode();
                if (!InsertBefore)
                    continue;

                IRBuilder<> IRB(InsertBefore);
                // void setRandomInjection(size_t nallocs, size_t nsubs, FaultType ft){

                Value *nallocs =  ConstantInt::get(IRB.getInt64Ty(), alloc_counter);
                Value *nsubs =  ConstantInt::get(IRB.getInt64Ty(), sub_counter);
                Value *ft =  ConstantInt::get(IRB.getInt64Ty(), FItype);
                IRB.CreateCall(setRandomInjectionFn,{nallocs,nsubs,ft});
                return true;
            }
        }
        
        return true;
    }
};

}


char Assignment::ID = 0;
static RegisterPass<Assignment> X("faultinjection",
                                   "Pass to inject faults");
