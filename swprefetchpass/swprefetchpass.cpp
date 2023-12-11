// original includes
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Transforms/Scalar.h"

// NOTE: legacy pass manager
// #include "llvm/Transforms/IPO/PassManagerBuilder.h"
// #include "llvm/IR/LegacyPassManager.h"

#include "llvm/Support/raw_ostream.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


// #include <iostream>  // prefer dbgs() over std::cout
#include <algorithm>
#include <map>
#include <set>
#include <string>

#include <llvm/Support/Debug.h>

// NOTE: includes from hw2
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/CFG.h"

#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

using namespace llvm;


namespace {

struct SwPrefetchPass : public PassInfoMixin<SwPrefetchPass> {

class PrefetchBuilder {
public:
    PrefetchBuilder(std::vector<Instruction*> insts_in, Function *F_in)
    : context(F_in->getContext()),
      builder(insts_in[1]),
      F(F_in),
      ind_var(insts_in[0]),
      base_offset(32) {
        for (Instruction* I : insts_in) {
            if(dyn_cast<GetElementPtrInst>(I)) {
                arrs.push_back(I);
            } // loop contains store
        }
    }

    void gen_insert_prefetch_code() {
        // Don't even attempt to prefetch more than 3 indirections
        if (arrs.size() > 3) return;

        Instruction* inner_arr = arrs[0];
        Instruction* outer_arr = arrs[1];
        int inner_offset = base_offset * 2;
        int outer_offset = base_offset;
        if (arrs.size() == 3) { // uncommon case, triple indirection
            inner_offset = base_offset * 3;
            outer_offset = base_offset * 2;
        }

        // inner prefetch
        Value* pref_index = get_arr_i_plus_offset(inner_arr, ind_var, inner_offset);
        add_prefetch(inner_arr, pref_index);

        // outer prefetch
        Value* inner_index = get_arr_i_plus_offset(inner_arr, ind_var, outer_offset);
        Instruction* inner_gep = get_gep_addr(inner_arr, inner_index);
        Instruction* inner_load = dyn_cast<Instruction>(builder.CreateLoad(get_arr_type(inner_arr)->getElementType(), inner_gep));
        add_prefetch(outer_arr, inner_load);

        // triple indirection
        // eg. C[B[A[i]]
        if (arrs.size() == 3) {
            int triple_offset = base_offset;
            // eg. A[i+offset]
            inner_index = get_arr_i_plus_offset(inner_arr, ind_var, triple_offset);
            inner_gep = get_gep_addr(inner_arr, inner_index);
            inner_load = dyn_cast<Instruction>(builder.CreateLoad(get_arr_type(inner_arr)->getElementType(), inner_gep));
            // eg. B[A[i+offset]]
            Instruction* outer_gep = get_gep_addr(inner_arr, inner_load);
            Instruction* outer_load = dyn_cast<Instruction>(builder.CreateLoad(get_arr_type(outer_arr)->getElementType(), outer_gep));
            // eg. prefetch C[B[A[i+offset]]]
            Instruction* triple_arr = arrs[2];
            add_prefetch(triple_arr, outer_load);
        }
    }

private:
    ConstantInt* makeInt32(int i) {
        return ConstantInt::get(Type::getInt32Ty(context), i);
    }

    ArrayType* get_arr_type(Instruction* arr_gep) {
        GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(arr_gep);
        PointerType* ptr = dyn_cast<PointerType>(gep->getPointerOperandType());
        return dyn_cast<ArrayType>(ptr->getElementType());
    }

    int get_array_size(Instruction* I) {
        return get_arr_type(I)->getNumElements();
    }

    Instruction* get_gep_addr(Instruction* arr_gep, Value* index) {
        Value* array_start = arr_gep->getOperand(0);
        return dyn_cast<Instruction>(builder.CreateGEP(get_arr_type(arr_gep), array_start, {makeInt32(0), index}));
    }

    Value* get_arr_i_plus_offset(Instruction* arr_gep, Value* i, int offset) {
        Instruction* i_plus_offset = dyn_cast<Instruction>(builder.CreateAdd(i, makeInt32(offset)));
        int arr_size = get_array_size(arr_gep);
        Value* size_minus_one = builder.CreateSub(makeInt32(arr_size), makeInt32(1));
        Value* cmp = builder.CreateICmpSLT(size_minus_one, i_plus_offset); // SLT = signed less than
        return builder.CreateSelect(cmp, size_minus_one, i_plus_offset);
    }

    // prefetch ref: https://llvm.org/docs/LangRef.html#llvm-prefetch-intrinsic
    void add_intrinsic_prefetch(Instruction* pref_addr) {
        Instruction* cast = dyn_cast<Instruction>(builder.CreateBitCast(pref_addr, Type::getInt8PtrTy(context)));
        std::vector<Type *> arg_type = {
            Type::getInt8PtrTy(context)
        };
        std::vector<Value *> args = {
            cast, // address to prefetch from
            makeInt32(0), // rw
            makeInt32(3), // locality
            makeInt32(1)  // cache
        };
        Function *func = Intrinsic::getDeclaration(F->getParent(), Intrinsic::prefetch, arg_type);
        builder.CreateCall(func, args);
    }

    void add_prefetch(Instruction* arr, Value* index) {
        Instruction* pref_addr = get_gep_addr(arr, index);
        add_intrinsic_prefetch(pref_addr);
    }


    std::vector<Instruction*> loads;
    std::vector<Instruction*> arrs;
    LLVMContext &context;
    IRBuilder<> builder;
    Function* F;
    Value* ind_var;
    const int base_offset;
};


    int get_slice_if_prefetchable(Instruction* I, Loop* L, LoopInfo &LI, std::vector<Instruction*> &slice, int num_loads, std::string debug_str, bool debug) {
        if (debug) dbgs() << debug_str << "dfs, I:" << *I << '\n';
        Loop* I_loop = LI.getLoopFor(I->getParent());
        // base case: instruction outside of loop
        if (L != I_loop) {
            if (debug) dbgs() << debug_str << "base case: outside of loop\n";
            return num_loads;
        }
        // base case: induction var
        if (I == I->getParent()->getFirstNonPHI()) {
            if (debug) dbgs() << debug_str << "base case: induction var\n";
            slice.push_back(I);
            return num_loads - 1; // -1 to not count the induction load
        }

        slice.push_back(I);
        Use* u = I->getOperandList();
        Use* end = u + I->getNumOperands();
        if (debug) dbgs() << debug_str << "uses:\n";
        for (Use* v = u; v<end; v++) {
            if (debug) dbgs() << debug_str << "->" << *(v->get()) << '\n';
        }

        int max_loads = 0;
        for (Use* v = u; v<end; v++) {
            if(LoadInst * linst = dyn_cast<LoadInst>(v->get())) {
                // dbgs() << debug_str << "setting prefetchable at: " << ;
                max_loads = std::max(max_loads, get_slice_if_prefetchable(linst, L, LI, slice, num_loads+1, debug_str+"  ", debug));
            }
            else if(Instruction* k = dyn_cast<Instruction>(v->get())) {
                max_loads = std::max(max_loads, get_slice_if_prefetchable(k, L, LI, slice, num_loads, debug_str+"  ", debug));
            }
            else {
                if (debug) dbgs() << debug_str << "use not Inst?" << *(v->get()) << '\n';
            }
        } // for (Use* v = u; v<end; v++)

        return max_loads;
    }

    bool fault_avoidance_success(std::vector<Instruction*> &load_slice, BasicBlock &BB) {
        if (contains_func_calls(BB)) return false;
        // if (contains_arr_stores(load_slice, BB)) return false; // TODO
        return true;
    }

    bool contains_func_calls(BasicBlock &BB) {
        for (Instruction &I : BB) {
            if (dyn_cast<CallInst>(&I)) {
                return true;
            }
        }
        return false;
    }

    bool contains_arr_stores(std::vector<Instruction*> &load_slice, BasicBlock &BB) {
        std::set<Value*> arrs = get_arrs(load_slice);
        for (Instruction &I_val : BB) {
            Instruction* I = &I_val;
            if(dyn_cast<StoreInst>(I)) {
                dbgs() << "on store: " << *I << '\n';
                Use* u = I->getOperandList();
                Use* end = u + I->getNumOperands();
                // for (Use* v = u; v<end; v++) {
                Use* v = ++u;
                if(Value* use_I = dyn_cast<Value>(v->get())) {
                    dbgs() << "comparing use  : " << *use_I << '\n';
                    dbgs() << "comparing array: " << *arrs.find(use_I) << '\n';
                    if (arrs.find(use_I) != arrs.end()) {
                        dbgs() << "faulty use  : " << *use_I << '\n';
                        dbgs() << "faulty array: " << *arrs.find(use_I) << '\n';
                        return true;
                    }
                }
                // }
            }
        }
        return false;
    }

    std::set<Value*> get_arrs(std::vector<Instruction*> &load_slice) {
        std::set<Value*> arrs;
        for (Instruction* I : load_slice) {
            if(GetElementPtrInst* gep_I = dyn_cast<GetElementPtrInst>(I)) {
                dbgs() << "found arr: " << *gep_I->getPointerOperand() << '\n';
                dbgs() << "       in: " << *I << '\n';
                arrs.insert(gep_I->getPointerOperand());
            }
        }
        return arrs;
    }

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        dbgs() << "-------------------- START SwPrefetchPass --------------------\n";
        LoopAnalysis::Result &LI = FAM.getResult<LoopAnalysis>(F);

        bool modified = false;

        std::vector<std::vector<Instruction*>> prefetchable_slices;

        // 1. Identify prefetchable indirect loads
        for(auto &BB : F) {
            // for (auto &I : BB) { // iterate forwards
            for (BasicBlock::reverse_iterator it = BB.rbegin(); it != BB.rend(); ++it) { // iterate backwards
                Instruction* I = &*it;
                if (LoadInst *i = dyn_cast<LoadInst>(I)) {
                    if(Loop* L = LI.getLoopFor(&BB)) {
                        dbgs() << "\n---Load in loop: " << *i << '\n';

                        std::vector<Instruction*> slice;
                        // Instruction* I, Loop* L, LoopInfo &LI, std::vector<Instruction*> &slice, int num_loads, std::string debug_str, bool debug
                        int num_loads = get_slice_if_prefetchable(i, L, LI, slice, 0, "", false);
                        bool prefetchable = num_loads > 0;
                        if (prefetchable) {
                            // fault avoidance checks
                            if (!fault_avoidance_success(slice, BB)) {
                                dbgs() << "Fault avoidance failed:" << *i << "\n";
                                continue;
                            }

                            // dbgs() << "Can prefetch:" << *i << "\n";
                            for (Instruction* slice_I : slice) {
                                dbgs() << "    " << *slice_I << '\n';
                            }

                            // reverse slice
                            std::reverse(slice.begin(), slice.end());

                            // add to all prefetchable slides
                            prefetchable_slices.push_back(slice);

                            break; // note: instead of unioning all, we just take the largest one from the start
                        }
                        else {
                            dbgs() << "Can NOT prefetch:" << *i << "\n";
                        }
                    }
                }
            }
        }

        dbgs() << "\n-------- Finished identfication phase --------\n";
        dbgs() << "Prefetchable loads:\n";
        for (auto slice : prefetchable_slices) {
            dbgs() << "    " << *slice.back() << '\n';
        }

        // 2. Generate and Insert prefetch code
        dbgs() << "\n-------- Starting prefetch gen/ins phase --------\n";
        for (std::vector<Instruction*> &slice : prefetchable_slices) {
            dbgs() << "Prefetching load: " << *slice.back() << '\n';

            // Instruction* insert_point = slice.front()->getParent()->getTerminator();
            // PrefetchBuilder pb(slice, &F, insert_point);
            PrefetchBuilder pb(slice, &F);
            pb.gen_insert_prefetch_code();

            modified = true;
        }

        (modified ? dbgs() << "modified\n" : dbgs() << "not modified\n");

        dbgs() << "-------------------- END   SwPrefetchPass --------------------\n";
        return PreservedAnalyses::all();
    }

}; // struct SwPrefetchPass

} // namespace


extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "SwPrefetchPass", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "swprefetchpass"){
            FPM.addPass(SwPrefetchPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}
