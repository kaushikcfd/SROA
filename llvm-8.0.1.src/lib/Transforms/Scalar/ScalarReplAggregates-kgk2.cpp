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

#define DEBUG_TYPE "scalarrepl-kgk2"
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

STATISTIC(NumReplaced,  "Number of aggregate allocas broken up");
STATISTIC(NumPromoted,  "Number of scalar allocas promoted to register");

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


bool isAllocaFirstClassType(AllocaInst& allocaInst)
{
    return (
        allocaInst.getAllocatedType()->isFPOrFPVectorTy()
        || allocaInst.getAllocatedType()->isIntOrIntVectorTy()
        || allocaInst.getAllocatedType()->isPtrOrPtrVectorTy());
}



/*
 * Collects all possible struct objects that can be legally expanded.
 */
struct PromotableAllocaCollector: public InstVisitor<PromotableAllocaCollector>
{
  std::vector<AllocaInst*> collectedAllocas;

  void visitAllocaInst(AllocaInst &allocaInst)
  {
    LLVM_DEBUG(dbgs() << "=============================================================\n");
    LLVM_DEBUG(dbgs() << "Came for the allocaInst: " << allocaInst << "\n");
    
    // check (P1)
    if (isAllocaFirstClassType(allocaInst))
    {
      Value::use_iterator use_iter=allocaInst.use_begin(), use_end=allocaInst.use_end();
      // check (P2)
      for (; use_iter != use_end; ++use_iter)
      {
        if (isa<AllocaInst>(*use_iter))
          continue;

        if (LoadInst *loadInst = dyn_cast<LoadInst>(*use_iter))
        {
          if (!loadInst->isVolatile())
            continue;
          else
            // use is a load, but volatile => alloca cannot be promoted
            break;
        }

        if (StoreInst *storeInst = dyn_cast<StoreInst>(*use_iter))
        {
          if (!storeInst->isVolatile())
            continue;
          else
            // use is a store, but volatile => alloca cannot be promoted
            break;
        }

        // Use is neither 'load' nor a 'store' => alloca cannot be promoted
        LLVM_DEBUG(dbgs() << "Failed because '"<< *(*use_iter) << "' is neither a load nor store.\n");
        break;
      }
      if (use_iter == use_end)
        collectedAllocas.push_back(&allocaInst);
    }
    else
      LLVM_DEBUG(dbgs() << "(P1) has royally embarassed itself.\n");
  }
};


bool satisfiesU1OrU2(User* user)
{
  if (GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(user))
  {
    if (gepInst->hasAllConstantIndices())
    {
      //FIXME: Not sure if this assert is needed. This might be dangerous.
      assert (gepInst->getNumIndices() == 2);
      ConstantInt *pos = (ConstantInt*)((gepInst->idx_begin())->get());
      uint64_t firstIdx = pos->getZExtValue();
      if (firstIdx == 0)
      {
        Value::use_iterator use_iter=gepInst->use_begin(), use_end=gepInst->use_end();
        for (; use_iter != use_end; ++use_iter)
        {
          if (LoadInst *loadInst = dyn_cast<LoadInst>(use_iter->getUser()))
          {
            if (loadInst->getPointerOperand() == gepInst)
              continue;
            else
              return false;
          }

          if (StoreInst *storeInst = dyn_cast<StoreInst>(use_iter->getUser()))
          {
            if (storeInst->getPointerOperand() == gepInst)
              continue;
            else
              return false;
          }

          if (!satisfiesU1OrU2(use_iter->getUser()))
            return false;
        }
        assert (use_iter == use_end);
        return true;
      }
    }
  }

  if (ICmpInst *icmpInst = dyn_cast<ICmpInst>(user))
  {
    if (icmpInst->isEquality())
    {
      if (isa<ConstantPointerNull>(icmpInst->getOperand(0))
          || isa<ConstantPointerNull>(icmpInst->getOperand(1)))
      {
        if (icmpInst->getPredicate() == CmpInst::Predicate::ICMP_EQ)
        {
          BasicBlock::iterator ii(icmpInst);
          ReplaceInstWithValue(icmpInst->getParent()->getInstList(), ii,
              ConstantInt::getFalse(icmpInst->getType()));
        }
        else
        {
          assert (icmpInst->getPredicate() == CmpInst::Predicate::ICMP_NE);

          BasicBlock::iterator ii(icmpInst);
          ReplaceInstWithValue(icmpInst->getParent()->getInstList(), ii,
              ConstantInt::getTrue(icmpInst->getType()));
        }

        return true;
      }
    }
  }

  return false;
}


/*
 * Collects all possible struct objects that can be legally expanded.
 */
struct ExpandableAllocaCollector: public InstVisitor<ExpandableAllocaCollector>
{
  std::vector<AllocaInst*> collectedAllocas;

  void visitAllocaInst(AllocaInst &allocaInst)
  {
    if (allocaInst.getAllocatedType()->isStructTy())
    {
      LLVM_DEBUG(dbgs() << "Inspecting '" << allocaInst << "'\n");
      Value::use_iterator use_iter=allocaInst.use_begin(), use_end=allocaInst.use_end();
      for (; use_iter != use_end; ++use_iter)
      {
        LLVM_DEBUG(dbgs() << "User: " << *(use_iter->getUser()) << "\n");

        if (!satisfiesU1OrU2(use_iter->getUser()))
        {
          LLVM_DEBUG(dbgs() << "Did not satisfy either (U1) or (U2).\n");
          break;
        }
        else
          LLVM_DEBUG(dbgs() << "Satisfied both U1 & U2.\n");

      }
      if (use_iter == use_end)
        collectedAllocas.push_back(&allocaInst);
    }
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
//                      SROA: Entry Point                                     //
//===----------------------------------------------------------------------===//
bool SROA::runOnFunction(Function &F) {
  dbgs() << "===-------------------------------------------------------------------------===\n";
  dbgs() << "Input Function:\n";
  dbgs() << "===-------------------------------------------------------------------------===\n";
  F.print(dbgs());
  dbgs() << "===-------------------------------------------------------------------------===\n";

  bool allocasWerePromoted = true, allocasWereExpanded = true;
  int iter_count = 0;

  while (allocasWerePromoted || allocasWereExpanded)
  {
    dbgs() << "DEBUG INFO: Running iteration #" << ++iter_count << ".\n";

    // Step1: Collect all allocas to be handed to PromoteMemToReg.
    PromotableAllocaCollector promotableAllocaCollector;
    promotableAllocaCollector.visit(F);
    NumPromoted += promotableAllocaCollector.collectedAllocas.size();
    allocasWerePromoted = !(promotableAllocaCollector.collectedAllocas.empty());

    DominatorTree domTree(F);
    PromoteMemToReg(promotableAllocaCollector.collectedAllocas, domTree);

    // Step2: Collect all *struct-type allocas* which could be expanded.
    ExpandableAllocaCollector expandableAllocaCollector;
    expandableAllocaCollector.visit(F);
    NumReplaced += expandableAllocaCollector.collectedAllocas.size();
    allocasWereExpanded = !(expandableAllocaCollector.collectedAllocas.empty());

    // Step3: Expand all allocas collected in Step2.
    for (AllocaInst* allocaToBeExpanded: expandableAllocaCollector.collectedAllocas)
      expandStructAlloca(F, allocaToBeExpanded);
  }


  dbgs() << "===-------------------------------------------------------------------------===\n";
  dbgs() << "SROA-ed Function:\n";
  dbgs() << "===-------------------------------------------------------------------------===\n";
  F.print(dbgs());

  // since we changed the function, we *must* return True.
  return true;
}

// vim:fdm=marker
