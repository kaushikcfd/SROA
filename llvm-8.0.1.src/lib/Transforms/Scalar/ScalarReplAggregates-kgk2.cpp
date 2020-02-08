//===- ScalarReplAggregates.cpp - Scalar Replacement of Aggregates --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This transformation implements the well known scalar replacement of
// aggregates transformation.  This xform breaks up alloca instructions of
// structure type into individual alloca instructions for
// each member (if possible).  Then, if possible, it transforms the individual
// alloca instructions into nice clean scalar SSA form.
//
// This combines an SRoA algorithm with Mem2Reg because they
// often interact, especially for C++ programs.  As such, this code
// iterates between SRoA and Mem2Reg until we run out of things to promote.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "scalarrepl"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

// FIXME: Enable them later?
// STATISTIC(NumReplaced,  "Number of aggregate allocas broken up");
// STATISTIC(NumPromoted,  "Number of scalar allocas promoted to register");

namespace {
  struct SROA : public FunctionPass {
    static char ID; // Pass identification
    SROA() : FunctionPass(ID) { }

    // Entry point for the overall scalar-replacement pass
    bool runOnFunction(Function &F);

    // getAnalysisUsage - List passes required by this pass.  We also know it
    // will not alter the CFG, so say so.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesCFG();
    }

  private:
    // Add fields and helper functions for this pass here.
  };
}

char SROA::ID = 0;
static RegisterPass<SROA> X("scalarrepl-kgk2",
			    "Scalar Replacement of Aggregates (by <kgk2>)",
			    false /* does not modify the CFG */,
			    false /* transformation, not just analysis */);


// Public interface to create the ScalarReplAggregates pass.
// This function is provided to you.
FunctionPass *createMyScalarReplAggregatesPass() { return new SROA(); }


/*
 * Collects all possible struct objects that can be legally expanded.
 */
struct ExpandableAllocaCollector: public InstVisitor<ExpandableAllocaCollector>
{
  int i_allocaInst = 0;
  std::vector<AllocaInst*> collectedAllocas;

  void visitAllocaInst(AllocaInst &allocaInst)
  {
    LLVM_DEBUG(dbgs() << "AllocaInst " << i_allocaInst << ": " << allocaInst << "\n");
    if (allocaInst.getAllocatedType()->isStructTy())
    {
      // FIXME: In the final version: there should be some check as in which
      // alloca instructions are chosen for expansion.
      collectedAllocas.push_back(&allocaInst);

      LLVM_DEBUG(dbgs() << "Its a pointer types and...\n");
      LLVM_DEBUG(dbgs() << "The type is --" << *allocaInst.getAllocatedType() << "\n");
      LLVM_DEBUG(dbgs() << "And it is writing to..." << allocaInst.getName()+"_danda\n");
    }
    i_allocaInst += 1;
  }
};


// {{{ Replace an alloca with all its members

/*
 * Replaces all instances of the GEP of the struct to be expanded with the
 * "new" variables.
 */
struct GetElemPtrReplacer: public InstVisitor<GetElemPtrReplacer>
{
  Value* structToExpand;
  std::vector<Value*> scalarsToBeReplacedWith;

  void visitGetElementPtrInst (GetElementPtrInst &gepInsn)
  {
    if(gepInsn.getPointerOperand() == structToExpand)
    {
      LLVM_DEBUG(dbgs() << "We should replace: " << gepInsn << "\n");
      assert(gepInsn.getNumIndices() == 2);
      assert(gepInsn.hasAllConstantIndices());
      ConstantInt *pos = (ConstantInt*)((gepInsn.idx_end()-1)->get());
      uint64_t intPos = pos->getZExtValue();
      LLVM_DEBUG(dbgs() << "This  insn should be replaced with '" << *(scalarsToBeReplacedWith[intPos]) << "'\n");
      BasicBlock::iterator ii(gepInsn);
      ReplaceInstWithValue(gepInsn.getParent()->getInstList(), ii,
          scalarsToBeReplacedWith[intPos]);
    }
  }
};

/*
 * Removes an *allocaInst* to a struct object and expands all its attributes
 */
void expandStructAlloca(Function &F, AllocaInst* allocaInst)
{
  assert(allocaInst->getAllocatedType()->isStructTy());
  std::string valueName = allocaInst->getName();
  StructType* structTypeToBeExpanded = dyn_cast<StructType>(allocaInst->getAllocatedType());
  std::vector<Value*> insertedScalars;
 
  int i = 0;
  // init-ing new instructions
  for(Type* const type: structTypeToBeExpanded->elements())
  {
    AllocaInst* newInsn = new AllocaInst(type, 0, valueName+"_"+std::to_string(i++), allocaInst);
    LLVM_DEBUG(dbgs() << "Inserted a new Insn " << i << ": " << *newInsn << "\n");
    insertedScalars.push_back(newInsn);
  }

  // replacing the uses of fields of structs with those of the newly created
  // scalars
  GetElemPtrReplacer gepReplacer;
  gepReplacer.structToExpand = allocaInst;
  gepReplacer.scalarsToBeReplacedWith = insertedScalars;
  gepReplacer.visit(F);

  // remove the allocaInst
  allocaInst->eraseFromParent();
}

// }}}


//===----------------------------------------------------------------------===//
//                      SKELETON FUNCTION TO BE IMPLEMENTED
//===----------------------------------------------------------------------===//
//
// Function runOnFunction:
// Entry point for the overall ScalarReplAggregates function pass.
// This function is provided to you.
bool SROA::runOnFunction(Function &F) {
  dbgs() << "===========================================================================\n";
  dbgs() << "Started with:\n";
  F.print(dbgs());
  ExpandableAllocaCollector allocaCollector;
  allocaCollector.visit(F);
  assert(allocaCollector.collectedAllocas.size() == 1);
  expandStructAlloca(F, allocaCollector.collectedAllocas[0]);
  dbgs() << "===========================================================================\n";
  dbgs() << "Final Function:\n";
  F.print(dbgs());

  // since we changed the function, we *must* return True.
  return true;
}

// vim:fdm=marker
