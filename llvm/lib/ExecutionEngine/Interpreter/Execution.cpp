//===-- Execution.cpp - Implement code to simulate the program ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains the actual instruction interpreter.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "interpreter"
#include "Interpreter.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include <cmath>
using namespace llvm;

STATISTIC(NumDynamicInsts, "Number of dynamic instructions executed");
static Interpreter *TheEE = 0;


//===----------------------------------------------------------------------===//
//                     Value Manipulation code
//===----------------------------------------------------------------------===//

static GenericValue executeAddInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty);
static GenericValue executeSubInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty);
static GenericValue executeMulInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty);
static GenericValue executeUDivInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeSDivInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeFDivInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeURemInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeSRemInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeFRemInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeAndInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty);
static GenericValue executeOrInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty);
static GenericValue executeXorInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty);
static GenericValue executeCmpInst(unsigned predicate, GenericValue Src1, 
                                   GenericValue Src2, const Type *Ty);
static GenericValue executeShlInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty);
static GenericValue executeLShrInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeAShrInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty);
static GenericValue executeSelectInst(GenericValue Src1, GenericValue Src2,
                                      GenericValue Src3);

inline uint64_t doSignExtension(uint64_t Val, const IntegerType* ITy) {
  // Determine if the value is signed or not
  bool isSigned = (Val & (1 << (ITy->getBitWidth()-1))) != 0;
  // If its signed, extend the sign bits
  if (isSigned)
    Val |= ~ITy->getBitMask();
  return Val;
}

GenericValue Interpreter::getConstantExprValue (ConstantExpr *CE,
                                                ExecutionContext &SF) {
  switch (CE->getOpcode()) {
  case Instruction::Trunc:   
      return executeTruncInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::ZExt:
      return executeZExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::SExt:
      return executeSExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPTrunc:
      return executeFPTruncInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPExt:
      return executeFPExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::UIToFP:
      return executeUIToFPInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::SIToFP:
      return executeSIToFPInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPToUI:
      return executeFPToUIInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPToSI:
      return executeFPToSIInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::PtrToInt:
      return executePtrToIntInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::IntToPtr:
      return executeIntToPtrInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::BitCast:
      return executeBitCastInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::GetElementPtr:
    return executeGEPOperation(CE->getOperand(0), gep_type_begin(CE),
                               gep_type_end(CE), SF);
  case Instruction::Add:
    return executeAddInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::Sub:
    return executeSubInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::Mul:
    return executeMulInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::SDiv:
    return executeSDivInst(getOperandValue(CE->getOperand(0), SF),
                           getOperandValue(CE->getOperand(1), SF),
                           CE->getOperand(0)->getType());
  case Instruction::UDiv:
    return executeUDivInst(getOperandValue(CE->getOperand(0), SF),
                           getOperandValue(CE->getOperand(1), SF),
                           CE->getOperand(0)->getType());
  case Instruction::FDiv:
    return executeFDivInst(getOperandValue(CE->getOperand(0), SF),
                           getOperandValue(CE->getOperand(1), SF),
                           CE->getOperand(0)->getType());
  case Instruction::URem:
    return executeURemInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::SRem:
    return executeSRemInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::FRem:
    return executeFRemInst(getOperandValue(CE->getOperand(0), SF),
                           getOperandValue(CE->getOperand(1), SF),
                           CE->getOperand(0)->getType());
  case Instruction::And:
    return executeAndInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::Or:
    return executeOrInst(getOperandValue(CE->getOperand(0), SF),
                         getOperandValue(CE->getOperand(1), SF),
                         CE->getOperand(0)->getType());
  case Instruction::Xor:
    return executeXorInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::FCmp:
  case Instruction::ICmp:
    return executeCmpInst(CE->getPredicate(),
                          getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::Shl:
    return executeShlInst(getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::LShr:
    return executeLShrInst(getOperandValue(CE->getOperand(0), SF),
                           getOperandValue(CE->getOperand(1), SF),
                           CE->getOperand(0)->getType());
  case Instruction::AShr:
    return executeAShrInst(getOperandValue(CE->getOperand(0), SF),
                           getOperandValue(CE->getOperand(1), SF),
                           CE->getOperand(0)->getType());
  case Instruction::Select:
    return executeSelectInst(getOperandValue(CE->getOperand(0), SF),
                             getOperandValue(CE->getOperand(1), SF),
                             getOperandValue(CE->getOperand(2), SF));
  default:
    cerr << "Unhandled ConstantExpr: " << *CE << "\n";
    abort();
    return GenericValue();
  }
}

GenericValue Interpreter::getOperandValue(Value *V, ExecutionContext &SF) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
    return getConstantExprValue(CE, SF);
  } else if (Constant *CPV = dyn_cast<Constant>(V)) {
    return getConstantValue(CPV);
  } else if (GlobalValue *GV = dyn_cast<GlobalValue>(V)) {
    return PTOGV(getPointerToGlobal(GV));
  } else {
    return SF.Values[V];
  }
}

static void SetValue(Value *V, GenericValue Val, ExecutionContext &SF) {
  SF.Values[V] = Val;
}

void Interpreter::initializeExecutionEngine() {
  TheEE = this;
}

//===----------------------------------------------------------------------===//
//                    Binary Instruction Implementations
//===----------------------------------------------------------------------===//

#define IMPLEMENT_BINARY_OPERATOR(OP, TY) \
   case Type::TY##TyID: Dest.TY##Val = Src1.TY##Val OP Src2.TY##Val; break

#define IMPLEMENT_INTEGER_BINOP(OP, TY) \
   case Type::IntegerTyID: { \
     unsigned BitWidth = cast<IntegerType>(TY)->getBitWidth(); \
     if (BitWidth == 1) \
       Dest.Int1Val = Src1.Int1Val OP Src2.Int1Val; \
     else if (BitWidth <= 8) \
       Dest.Int8Val = Src1.Int8Val OP Src2.Int8Val; \
     else if (BitWidth <= 16) \
       Dest.Int16Val = Src1.Int16Val OP Src2.Int16Val; \
     else if (BitWidth <= 32) \
       Dest.Int32Val = Src1.Int32Val OP Src2.Int32Val; \
     else if (BitWidth <= 64) \
       Dest.Int64Val = Src1.Int64Val OP Src2.Int64Val; \
     else \
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n"; \
     maskToBitWidth(Dest, BitWidth); \
     break; \
   }

#define IMPLEMENT_SIGNED_BINOP(OP, TY) \
   if (const IntegerType *ITy = dyn_cast<IntegerType>(TY)) { \
     unsigned BitWidth = ITy->getBitWidth(); \
     if (BitWidth <= 8) \
       Dest.Int8Val  = ((int8_t)Src1.Int8Val)   OP ((int8_t)Src2.Int8Val); \
     else if (BitWidth <= 16) \
       Dest.Int16Val = ((int16_t)Src1.Int16Val) OP ((int16_t)Src2.Int16Val); \
     else if (BitWidth <= 32) \
       Dest.Int32Val = ((int32_t)Src1.Int32Val) OP ((int32_t)Src2.Int32Val); \
     else if (BitWidth <= 64) \
       Dest.Int64Val = ((int64_t)Src1.Int64Val) OP ((int64_t)Src2.Int64Val); \
     else { \
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n"; \
       abort(); \
     } \
     maskToBitWidth(Dest, BitWidth); \
   } else { \
    cerr << "Unhandled type for " #OP " operator: " << *Ty << "\n"; \
    abort(); \
   }

#define IMPLEMENT_UNSIGNED_BINOP(OP, TY) \
   if (const IntegerType *ITy = dyn_cast<IntegerType>(TY)) { \
     unsigned BitWidth = ITy->getBitWidth(); \
     if (BitWidth <= 8) \
       Dest.Int8Val  = ((uint8_t)Src1.Int8Val)   OP ((uint8_t)Src2.Int8Val); \
     else if (BitWidth <= 16) \
       Dest.Int16Val = ((uint16_t)Src1.Int16Val) OP ((uint16_t)Src2.Int16Val); \
     else if (BitWidth <= 32) \
       Dest.Int32Val = ((uint32_t)Src1.Int32Val) OP ((uint32_t)Src2.Int32Val); \
     else if (BitWidth <= 64) \
       Dest.Int64Val = ((uint64_t)Src1.Int64Val) OP ((uint64_t)Src2.Int64Val); \
     else { \
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n"; \
       abort(); \
     } \
     maskToBitWidth(Dest, BitWidth); \
   } else { \
    cerr << "Unhandled type for " #OP " operator: " << *Ty << "\n"; \
    abort(); \
  }

static GenericValue executeAddInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_BINOP(+, Ty);
    IMPLEMENT_BINARY_OPERATOR(+, Float);
    IMPLEMENT_BINARY_OPERATOR(+, Double);
  default:
    cerr << "Unhandled type for Add instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeSubInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_BINOP(-, Ty);
    IMPLEMENT_BINARY_OPERATOR(-, Float);
    IMPLEMENT_BINARY_OPERATOR(-, Double);
  default:
    cerr << "Unhandled type for Sub instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeMulInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_BINOP(*, Ty);
    IMPLEMENT_BINARY_OPERATOR(*, Float);
    IMPLEMENT_BINARY_OPERATOR(*, Double);
  default:
    cerr << "Unhandled type for Mul instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeUDivInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNSIGNED_BINOP(/,Ty)
  return Dest;
}

static GenericValue executeSDivInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_SIGNED_BINOP(/,Ty)
  return Dest;
}

static GenericValue executeFDivInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(/, Float);
    IMPLEMENT_BINARY_OPERATOR(/, Double);
  default:
    cerr << "Unhandled type for FDiv instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeURemInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNSIGNED_BINOP(%, Ty)
  return Dest;
}

static GenericValue executeSRemInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_SIGNED_BINOP(%, Ty)
  return Dest;
}

static GenericValue executeFRemInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
  case Type::FloatTyID:
    Dest.FloatVal = fmod(Src1.FloatVal, Src2.FloatVal);
    break;
  case Type::DoubleTyID:
    Dest.DoubleVal = fmod(Src1.DoubleVal, Src2.DoubleVal);
    break;
  default:
    cerr << "Unhandled type for Rem instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeAndInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNSIGNED_BINOP(&, Ty)
  return Dest;
}

static GenericValue executeOrInst(GenericValue Src1, GenericValue Src2,
                                  const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNSIGNED_BINOP(|, Ty)
  return Dest;
}

static GenericValue executeXorInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNSIGNED_BINOP(^, Ty)
  return Dest;
}

#define IMPLEMENT_SIGNED_ICMP(OP, TY) \
   case Type::IntegerTyID: {  \
     const IntegerType* ITy = cast<IntegerType>(TY); \
     unsigned BitWidth = ITy->getBitWidth(); \
     int64_t LHS = 0, RHS = 0; \
     if (BitWidth <= 8) { \
       LHS = int64_t(doSignExtension(uint64_t(Src1.Int8Val), ITy)); \
       RHS = int64_t(doSignExtension(uint64_t(Src2.Int8Val), ITy)); \
     } else if (BitWidth <= 16) { \
       LHS = int64_t(doSignExtension(uint64_t(Src1.Int16Val), ITy)); \
       RHS = int64_t(doSignExtension(uint64_t(Src2.Int16Val), ITy)); \
    } else if (BitWidth <= 32) { \
       LHS = int64_t(doSignExtension(uint64_t(Src1.Int32Val), ITy)); \
       RHS = int64_t(doSignExtension(uint64_t(Src2.Int32Val), ITy)); \
    } else if (BitWidth <= 64) { \
       LHS = int64_t(doSignExtension(uint64_t(Src1.Int64Val), ITy)); \
       RHS = int64_t(doSignExtension(uint64_t(Src2.Int64Val), ITy)); \
    } else { \
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n"; \
       abort(); \
     } \
     Dest.Int1Val = LHS OP RHS; \
     break; \
   }

#define IMPLEMENT_UNSIGNED_ICMP(OP, TY) \
   case Type::IntegerTyID: { \
     unsigned BitWidth = cast<IntegerType>(TY)->getBitWidth(); \
     if (BitWidth == 1) \
       Dest.Int1Val = ((uint8_t)Src1.Int1Val)   OP ((uint8_t)Src2.Int1Val); \
     else if (BitWidth <= 8) \
       Dest.Int1Val = ((uint8_t)Src1.Int8Val)   OP ((uint8_t)Src2.Int8Val); \
     else if (BitWidth <= 16) \
       Dest.Int1Val = ((uint16_t)Src1.Int16Val) OP ((uint16_t)Src2.Int16Val); \
     else if (BitWidth <= 32) \
       Dest.Int1Val = ((uint32_t)Src1.Int32Val) OP ((uint32_t)Src2.Int32Val); \
     else if (BitWidth <= 64) \
       Dest.Int1Val = ((uint64_t)Src1.Int64Val) OP ((uint64_t)Src2.Int64Val); \
     else { \
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n"; \
       abort(); \
     } \
     maskToBitWidth(Dest, BitWidth); \
     break; \
   }

// Handle pointers specially because they must be compared with only as much
// width as the host has.  We _do not_ want to be comparing 64 bit values when
// running on a 32-bit target, otherwise the upper 32 bits might mess up
// comparisons if they contain garbage.
#define IMPLEMENT_POINTER_ICMP(OP) \
   case Type::PointerTyID: \
        Dest.Int1Val = (void*)(intptr_t)Src1.PointerVal OP \
                       (void*)(intptr_t)Src2.PointerVal; break

static GenericValue executeICMP_EQ(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_UNSIGNED_ICMP(==, Ty);
    IMPLEMENT_POINTER_ICMP(==);
  default:
    cerr << "Unhandled type for ICMP_EQ predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_NE(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_UNSIGNED_ICMP(!=, Ty);
    IMPLEMENT_POINTER_ICMP(!=);
  default:
    cerr << "Unhandled type for ICMP_NE predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_ULT(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_UNSIGNED_ICMP(<, Ty);
    IMPLEMENT_POINTER_ICMP(<);
  default:
    cerr << "Unhandled type for ICMP_ULT predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_SLT(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_SIGNED_ICMP(<, Ty);
    IMPLEMENT_POINTER_ICMP(<);
  default:
    cerr << "Unhandled type for ICMP_SLT predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_UGT(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_UNSIGNED_ICMP(>, Ty);
    IMPLEMENT_POINTER_ICMP(>);
  default:
    cerr << "Unhandled type for ICMP_UGT predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_SGT(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_SIGNED_ICMP(>, Ty);
    IMPLEMENT_POINTER_ICMP(>);
  default:
    cerr << "Unhandled type for ICMP_SGT predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_ULE(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_UNSIGNED_ICMP(<=, Ty);
    IMPLEMENT_POINTER_ICMP(<=);
  default:
    cerr << "Unhandled type for ICMP_ULE predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_SLE(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_SIGNED_ICMP(<=, Ty);
    IMPLEMENT_POINTER_ICMP(<=);
  default:
    cerr << "Unhandled type for ICMP_SLE predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_UGE(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_UNSIGNED_ICMP(>=,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    cerr << "Unhandled type for ICMP_UGE predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeICMP_SGE(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_SIGNED_ICMP(>=, Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    cerr << "Unhandled type for ICMP_SGE predicate: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

void Interpreter::visitICmpInst(ICmpInst &I) {
  ExecutionContext &SF = ECStack.back();
  const Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result
  
  switch (I.getPredicate()) {
  case ICmpInst::ICMP_EQ:  R = executeICMP_EQ(Src1,  Src2, Ty); break;
  case ICmpInst::ICMP_NE:  R = executeICMP_NE(Src1,  Src2, Ty); break;
  case ICmpInst::ICMP_ULT: R = executeICMP_ULT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SLT: R = executeICMP_SLT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_UGT: R = executeICMP_UGT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SGT: R = executeICMP_SGT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_ULE: R = executeICMP_ULE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SLE: R = executeICMP_SLE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_UGE: R = executeICMP_UGE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SGE: R = executeICMP_SGE(Src1, Src2, Ty); break;
  default:
    cerr << "Don't know how to handle this ICmp predicate!\n-->" << I;
    abort();
  }
 
  SetValue(&I, R, SF);
}

#define IMPLEMENT_FCMP(OP, TY) \
   case Type::TY##TyID: Dest.Int1Val = Src1.TY##Val OP Src2.TY##Val; break

static GenericValue executeFCMP_OEQ(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(==, Float);
    IMPLEMENT_FCMP(==, Double);
  default:
    cerr << "Unhandled type for FCmp EQ instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeFCMP_ONE(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(!=, Float);
    IMPLEMENT_FCMP(!=, Double);

  default:
    cerr << "Unhandled type for FCmp NE instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeFCMP_OLE(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<=, Float);
    IMPLEMENT_FCMP(<=, Double);
  default:
    cerr << "Unhandled type for FCmp LE instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeFCMP_OGE(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>=, Float);
    IMPLEMENT_FCMP(>=, Double);
  default:
    cerr << "Unhandled type for FCmp GE instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeFCMP_OLT(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<, Float);
    IMPLEMENT_FCMP(<, Double);
  default:
    cerr << "Unhandled type for FCmp LT instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeFCMP_OGT(GenericValue Src1, GenericValue Src2,
                                     const Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>, Float);
    IMPLEMENT_FCMP(>, Double);
  default:
    cerr << "Unhandled type for FCmp GT instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

#define IMPLEMENT_UNORDERED(TY, X,Y) \
   if (TY == Type::FloatTy) \
     if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) { \
       Dest.Int1Val = true; \
       return Dest; \
     } \
   else if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) { \
     Dest.Int1Val = true; \
     return Dest; \
   }


static GenericValue executeFCMP_UEQ(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  return executeFCMP_OEQ(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UNE(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  return executeFCMP_ONE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ULE(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  return executeFCMP_OLE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UGE(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  return executeFCMP_OGE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ULT(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  return executeFCMP_OLT(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UGT(GenericValue Src1, GenericValue Src2,
                                     const Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  return executeFCMP_OGT(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ORD(GenericValue Src1, GenericValue Src2,
                                     const Type *Ty) {
  GenericValue Dest;
  if (Ty == Type::FloatTy)
    Dest.Int1Val = (Src1.FloatVal == Src1.FloatVal && 
                    Src2.FloatVal == Src2.FloatVal);
  else
    Dest.Int1Val = (Src1.DoubleVal == Src1.DoubleVal && 
                    Src2.DoubleVal == Src2.DoubleVal);
  return Dest;
}

static GenericValue executeFCMP_UNO(GenericValue Src1, GenericValue Src2,
                                     const Type *Ty) {
  GenericValue Dest;
  if (Ty == Type::FloatTy)
    Dest.Int1Val = (Src1.FloatVal != Src1.FloatVal || 
                    Src2.FloatVal != Src2.FloatVal);
  else
    Dest.Int1Val = (Src1.DoubleVal != Src1.DoubleVal || 
                    Src2.DoubleVal != Src2.DoubleVal);
  return Dest;
}

void Interpreter::visitFCmpInst(FCmpInst &I) {
  ExecutionContext &SF = ECStack.back();
  const Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result
  
  switch (I.getPredicate()) {
  case FCmpInst::FCMP_FALSE: R.Int1Val = false; break;
  case FCmpInst::FCMP_TRUE:  R.Int1Val = true; break;
  case FCmpInst::FCMP_ORD:   R = executeFCMP_ORD(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UNO:   R = executeFCMP_UNO(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UEQ:   R = executeFCMP_UEQ(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OEQ:   R = executeFCMP_OEQ(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UNE:   R = executeFCMP_UNE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ONE:   R = executeFCMP_ONE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ULT:   R = executeFCMP_ULT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OLT:   R = executeFCMP_OLT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UGT:   R = executeFCMP_UGT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OGT:   R = executeFCMP_OGT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ULE:   R = executeFCMP_ULE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OLE:   R = executeFCMP_OLE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UGE:   R = executeFCMP_UGE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OGE:   R = executeFCMP_OGE(Src1, Src2, Ty); break;
  default:
    cerr << "Don't know how to handle this FCmp predicate!\n-->" << I;
    abort();
  }
 
  SetValue(&I, R, SF);
}

static GenericValue executeCmpInst(unsigned predicate, GenericValue Src1, 
                                   GenericValue Src2, const Type *Ty) {
  GenericValue Result;
  switch (predicate) {
  case ICmpInst::ICMP_EQ:    return executeICMP_EQ(Src1, Src2, Ty);
  case ICmpInst::ICMP_NE:    return executeICMP_NE(Src1, Src2, Ty);
  case ICmpInst::ICMP_UGT:   return executeICMP_UGT(Src1, Src2, Ty);
  case ICmpInst::ICMP_SGT:   return executeICMP_SGT(Src1, Src2, Ty);
  case ICmpInst::ICMP_ULT:   return executeICMP_ULT(Src1, Src2, Ty);
  case ICmpInst::ICMP_SLT:   return executeICMP_SLT(Src1, Src2, Ty);
  case ICmpInst::ICMP_UGE:   return executeICMP_UGE(Src1, Src2, Ty);
  case ICmpInst::ICMP_SGE:   return executeICMP_SGE(Src1, Src2, Ty);
  case ICmpInst::ICMP_ULE:   return executeICMP_ULE(Src1, Src2, Ty);
  case ICmpInst::ICMP_SLE:   return executeICMP_SLE(Src1, Src2, Ty);
  case FCmpInst::FCMP_ORD:   return executeFCMP_ORD(Src1, Src2, Ty);
  case FCmpInst::FCMP_UNO:   return executeFCMP_UNO(Src1, Src2, Ty);
  case FCmpInst::FCMP_OEQ:   return executeFCMP_OEQ(Src1, Src2, Ty);
  case FCmpInst::FCMP_UEQ:   return executeFCMP_UEQ(Src1, Src2, Ty);
  case FCmpInst::FCMP_ONE:   return executeFCMP_ONE(Src1, Src2, Ty);
  case FCmpInst::FCMP_UNE:   return executeFCMP_UNE(Src1, Src2, Ty);
  case FCmpInst::FCMP_OLT:   return executeFCMP_OLT(Src1, Src2, Ty);
  case FCmpInst::FCMP_ULT:   return executeFCMP_ULT(Src1, Src2, Ty);
  case FCmpInst::FCMP_OGT:   return executeFCMP_OGT(Src1, Src2, Ty);
  case FCmpInst::FCMP_UGT:   return executeFCMP_UGT(Src1, Src2, Ty);
  case FCmpInst::FCMP_OLE:   return executeFCMP_OLE(Src1, Src2, Ty);
  case FCmpInst::FCMP_ULE:   return executeFCMP_ULE(Src1, Src2, Ty);
  case FCmpInst::FCMP_OGE:   return executeFCMP_OGE(Src1, Src2, Ty);
  case FCmpInst::FCMP_UGE:   return executeFCMP_UGE(Src1, Src2, Ty);
  case FCmpInst::FCMP_FALSE: { 
    GenericValue Result;
    Result.Int1Val = false; 
    return Result;
  }
  case FCmpInst::FCMP_TRUE: {
    GenericValue Result;
    Result.Int1Val = true;
    return Result;
  }
  default:
    cerr << "Unhandled Cmp predicate\n";
    abort();
  }
}

void Interpreter::visitBinaryOperator(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  const Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result

  switch (I.getOpcode()) {
  case Instruction::Add:   R = executeAddInst  (Src1, Src2, Ty); break;
  case Instruction::Sub:   R = executeSubInst  (Src1, Src2, Ty); break;
  case Instruction::Mul:   R = executeMulInst  (Src1, Src2, Ty); break;
  case Instruction::UDiv:  R = executeUDivInst (Src1, Src2, Ty); break;
  case Instruction::SDiv:  R = executeSDivInst (Src1, Src2, Ty); break;
  case Instruction::FDiv:  R = executeFDivInst (Src1, Src2, Ty); break;
  case Instruction::URem:  R = executeURemInst (Src1, Src2, Ty); break;
  case Instruction::SRem:  R = executeSRemInst (Src1, Src2, Ty); break;
  case Instruction::FRem:  R = executeFRemInst (Src1, Src2, Ty); break;
  case Instruction::And:   R = executeAndInst  (Src1, Src2, Ty); break;
  case Instruction::Or:    R = executeOrInst   (Src1, Src2, Ty); break;
  case Instruction::Xor:   R = executeXorInst  (Src1, Src2, Ty); break;
  default:
    cerr << "Don't know how to handle this binary operator!\n-->" << I;
    abort();
  }

  SetValue(&I, R, SF);
}

static GenericValue executeSelectInst(GenericValue Src1, GenericValue Src2,
                                      GenericValue Src3) {
  return Src1.Int1Val ? Src2 : Src3;
}

void Interpreter::visitSelectInst(SelectInst &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Src3 = getOperandValue(I.getOperand(2), SF);
  GenericValue R = executeSelectInst(Src1, Src2, Src3);
  SetValue(&I, R, SF);
}


//===----------------------------------------------------------------------===//
//                     Terminator Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::exitCalled(GenericValue GV) {
  // runAtExitHandlers() assumes there are no stack frames, but
  // if exit() was called, then it had a stack frame. Blow away
  // the stack before interpreting atexit handlers.
  ECStack.clear ();
  runAtExitHandlers ();
  exit (GV.Int32Val);
}

/// Pop the last stack frame off of ECStack and then copy the result
/// back into the result variable if we are not returning void. The
/// result variable may be the ExitValue, or the Value of the calling
/// CallInst if there was a previous stack frame. This method may
/// invalidate any ECStack iterators you have. This method also takes
/// care of switching to the normal destination BB, if we are returning
/// from an invoke.
///
void Interpreter::popStackAndReturnValueToCaller (const Type *RetTy,
                                                  GenericValue Result) {
  // Pop the current stack frame.
  ECStack.pop_back();

  if (ECStack.empty()) {  // Finished main.  Put result into exit code...
    if (RetTy && RetTy->isInteger()) {          // Nonvoid return type?
      ExitValue = Result;   // Capture the exit value of the program
    } else {
      memset(&ExitValue, 0, sizeof(ExitValue));
    }
  } else {
    // If we have a previous stack frame, and we have a previous call,
    // fill in the return value...
    ExecutionContext &CallingSF = ECStack.back();
    if (Instruction *I = CallingSF.Caller.getInstruction()) {
      if (CallingSF.Caller.getType() != Type::VoidTy)      // Save result...
        SetValue(I, Result, CallingSF);
      if (InvokeInst *II = dyn_cast<InvokeInst> (I))
        SwitchToNewBasicBlock (II->getNormalDest (), CallingSF);
      CallingSF.Caller = CallSite();          // We returned from the call...
    }
  }
}

void Interpreter::visitReturnInst(ReturnInst &I) {
  ExecutionContext &SF = ECStack.back();
  const Type *RetTy = Type::VoidTy;
  GenericValue Result;

  // Save away the return value... (if we are not 'ret void')
  if (I.getNumOperands()) {
    RetTy  = I.getReturnValue()->getType();
    Result = getOperandValue(I.getReturnValue(), SF);
  }

  popStackAndReturnValueToCaller(RetTy, Result);
}

void Interpreter::visitUnwindInst(UnwindInst &I) {
  // Unwind stack
  Instruction *Inst;
  do {
    ECStack.pop_back ();
    if (ECStack.empty ())
      abort ();
    Inst = ECStack.back ().Caller.getInstruction ();
  } while (!(Inst && isa<InvokeInst> (Inst)));

  // Return from invoke
  ExecutionContext &InvokingSF = ECStack.back ();
  InvokingSF.Caller = CallSite ();

  // Go to exceptional destination BB of invoke instruction
  SwitchToNewBasicBlock(cast<InvokeInst>(Inst)->getUnwindDest(), InvokingSF);
}

void Interpreter::visitUnreachableInst(UnreachableInst &I) {
  cerr << "ERROR: Program executed an 'unreachable' instruction!\n";
  abort();
}

void Interpreter::visitBranchInst(BranchInst &I) {
  ExecutionContext &SF = ECStack.back();
  BasicBlock *Dest;

  Dest = I.getSuccessor(0);          // Uncond branches have a fixed dest...
  if (!I.isUnconditional()) {
    Value *Cond = I.getCondition();
    if (getOperandValue(Cond, SF).Int1Val == 0) // If false cond...
      Dest = I.getSuccessor(1);
  }
  SwitchToNewBasicBlock(Dest, SF);
}

void Interpreter::visitSwitchInst(SwitchInst &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue CondVal = getOperandValue(I.getOperand(0), SF);
  const Type *ElTy = I.getOperand(0)->getType();

  // Check to see if any of the cases match...
  BasicBlock *Dest = 0;
  for (unsigned i = 2, e = I.getNumOperands(); i != e; i += 2)
    if (executeICMP_EQ(CondVal,
                       getOperandValue(I.getOperand(i), SF), ElTy).Int1Val) {
      Dest = cast<BasicBlock>(I.getOperand(i+1));
      break;
    }

  if (!Dest) Dest = I.getDefaultDest();   // No cases matched: use default
  SwitchToNewBasicBlock(Dest, SF);
}

// SwitchToNewBasicBlock - This method is used to jump to a new basic block.
// This function handles the actual updating of block and instruction iterators
// as well as execution of all of the PHI nodes in the destination block.
//
// This method does this because all of the PHI nodes must be executed
// atomically, reading their inputs before any of the results are updated.  Not
// doing this can cause problems if the PHI nodes depend on other PHI nodes for
// their inputs.  If the input PHI node is updated before it is read, incorrect
// results can happen.  Thus we use a two phase approach.
//
void Interpreter::SwitchToNewBasicBlock(BasicBlock *Dest, ExecutionContext &SF){
  BasicBlock *PrevBB = SF.CurBB;      // Remember where we came from...
  SF.CurBB   = Dest;                  // Update CurBB to branch destination
  SF.CurInst = SF.CurBB->begin();     // Update new instruction ptr...

  if (!isa<PHINode>(SF.CurInst)) return;  // Nothing fancy to do

  // Loop over all of the PHI nodes in the current block, reading their inputs.
  std::vector<GenericValue> ResultValues;

  for (; PHINode *PN = dyn_cast<PHINode>(SF.CurInst); ++SF.CurInst) {
    // Search for the value corresponding to this previous bb...
    int i = PN->getBasicBlockIndex(PrevBB);
    assert(i != -1 && "PHINode doesn't contain entry for predecessor??");
    Value *IncomingValue = PN->getIncomingValue(i);

    // Save the incoming value for this PHI node...
    ResultValues.push_back(getOperandValue(IncomingValue, SF));
  }

  // Now loop over all of the PHI nodes setting their values...
  SF.CurInst = SF.CurBB->begin();
  for (unsigned i = 0; isa<PHINode>(SF.CurInst); ++SF.CurInst, ++i) {
    PHINode *PN = cast<PHINode>(SF.CurInst);
    SetValue(PN, ResultValues[i], SF);
  }
}

//===----------------------------------------------------------------------===//
//                     Memory Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::visitAllocationInst(AllocationInst &I) {
  ExecutionContext &SF = ECStack.back();

  const Type *Ty = I.getType()->getElementType();  // Type to be allocated

  // Get the number of elements being allocated by the array...
  unsigned NumElements = getOperandValue(I.getOperand(0), SF).Int32Val;

  // Allocate enough memory to hold the type...
  void *Memory = malloc(NumElements * (size_t)TD.getTypeSize(Ty));

  GenericValue Result = PTOGV(Memory);
  assert(Result.PointerVal != 0 && "Null pointer returned by malloc!");
  SetValue(&I, Result, SF);

  if (I.getOpcode() == Instruction::Alloca)
    ECStack.back().Allocas.add(Memory);
}

void Interpreter::visitFreeInst(FreeInst &I) {
  ExecutionContext &SF = ECStack.back();
  assert(isa<PointerType>(I.getOperand(0)->getType()) && "Freeing nonptr?");
  GenericValue Value = getOperandValue(I.getOperand(0), SF);
  // TODO: Check to make sure memory is allocated
  free(GVTOP(Value));   // Free memory
}

// getElementOffset - The workhorse for getelementptr.
//
GenericValue Interpreter::executeGEPOperation(Value *Ptr, gep_type_iterator I,
                                              gep_type_iterator E,
                                              ExecutionContext &SF) {
  assert(isa<PointerType>(Ptr->getType()) &&
         "Cannot getElementOffset of a nonpointer type!");

  PointerTy Total = 0;

  for (; I != E; ++I) {
    if (const StructType *STy = dyn_cast<StructType>(*I)) {
      const StructLayout *SLO = TD.getStructLayout(STy);

      const ConstantInt *CPU = cast<ConstantInt>(I.getOperand());
      unsigned Index = unsigned(CPU->getZExtValue());

      Total += (PointerTy)SLO->getElementOffset(Index);
    } else {
      const SequentialType *ST = cast<SequentialType>(*I);
      // Get the index number for the array... which must be long type...
      GenericValue IdxGV = getOperandValue(I.getOperand(), SF);

      int64_t Idx;
      unsigned BitWidth = 
        cast<IntegerType>(I.getOperand()->getType())->getBitWidth();
      if (BitWidth == 32)
        Idx = (int64_t)(int32_t)IdxGV.Int32Val;
      else if (BitWidth == 64)
        Idx = (int64_t)IdxGV.Int64Val;
      else 
        assert(0 && "Invalid index type for getelementptr");
      Total += PointerTy(TD.getTypeSize(ST->getElementType())*Idx);
    }
  }

  GenericValue Result;
  Result.PointerVal = getOperandValue(Ptr, SF).PointerVal + Total;
  return Result;
}

void Interpreter::visitGetElementPtrInst(GetElementPtrInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, TheEE->executeGEPOperation(I.getPointerOperand(),
                                   gep_type_begin(I), gep_type_end(I), SF), SF);
}

void Interpreter::visitLoadInst(LoadInst &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  GenericValue *Ptr = (GenericValue*)GVTOP(SRC);
  GenericValue Result = LoadValueFromMemory(Ptr, I.getType());
  SetValue(&I, Result, SF);
}

void Interpreter::visitStoreInst(StoreInst &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue Val = getOperandValue(I.getOperand(0), SF);
  GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  StoreValueToMemory(Val, (GenericValue *)GVTOP(SRC),
                     I.getOperand(0)->getType());
}

//===----------------------------------------------------------------------===//
//                 Miscellaneous Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::visitCallSite(CallSite CS) {
  ExecutionContext &SF = ECStack.back();

  // Check to see if this is an intrinsic function call...
  if (Function *F = CS.getCalledFunction())
   if (F->isDeclaration ())
    switch (F->getIntrinsicID()) {
    case Intrinsic::not_intrinsic:
      break;
    case Intrinsic::vastart: { // va_start
      GenericValue ArgIndex;
      ArgIndex.UIntPairVal.first = ECStack.size() - 1;
      ArgIndex.UIntPairVal.second = 0;
      SetValue(CS.getInstruction(), ArgIndex, SF);
      return;
    }
    case Intrinsic::vaend:    // va_end is a noop for the interpreter
      return;
    case Intrinsic::vacopy:   // va_copy: dest = src
      SetValue(CS.getInstruction(), getOperandValue(*CS.arg_begin(), SF), SF);
      return;
    default:
      // If it is an unknown intrinsic function, use the intrinsic lowering
      // class to transform it into hopefully tasty LLVM code.
      //
      Instruction *Prev = CS.getInstruction()->getPrev();
      BasicBlock *Parent = CS.getInstruction()->getParent();
      IL->LowerIntrinsicCall(cast<CallInst>(CS.getInstruction()));

      // Restore the CurInst pointer to the first instruction newly inserted, if
      // any.
      if (!Prev) {
        SF.CurInst = Parent->begin();
      } else {
        SF.CurInst = Prev;
        ++SF.CurInst;
      }
      return;
    }

  SF.Caller = CS;
  std::vector<GenericValue> ArgVals;
  const unsigned NumArgs = SF.Caller.arg_size();
  ArgVals.reserve(NumArgs);
  for (CallSite::arg_iterator i = SF.Caller.arg_begin(),
         e = SF.Caller.arg_end(); i != e; ++i) {
    Value *V = *i;
    ArgVals.push_back(getOperandValue(V, SF));
    // Promote all integral types whose size is < sizeof(int) into ints.  We do
    // this by zero or sign extending the value as appropriate according to the
    // source type.
    const Type *Ty = V->getType();
    if (Ty->isInteger()) {
      if (Ty->getPrimitiveSizeInBits() == 1)
        ArgVals.back().Int32Val = ArgVals.back().Int1Val;
      else if (Ty->getPrimitiveSizeInBits() <= 8)
        ArgVals.back().Int32Val = ArgVals.back().Int8Val;
      else if (Ty->getPrimitiveSizeInBits() <= 16)
        ArgVals.back().Int32Val = ArgVals.back().Int16Val;
    }
  }

  // To handle indirect calls, we must get the pointer value from the argument
  // and treat it as a function pointer.
  GenericValue SRC = getOperandValue(SF.Caller.getCalledValue(), SF);
  callFunction((Function*)GVTOP(SRC), ArgVals);
}

static GenericValue executeShlInst(GenericValue Src1, GenericValue Src2,
                                   const Type *Ty) {
  GenericValue Dest;
  if (const IntegerType *ITy = cast<IntegerType>(Ty)) {
    unsigned BitWidth = ITy->getBitWidth();
    if (BitWidth <= 8)
      Dest.Int8Val  = ((uint8_t)Src1.Int8Val)   << ((uint32_t)Src2.Int8Val);
    else if (BitWidth <= 16)
      Dest.Int16Val = ((uint16_t)Src1.Int16Val) << ((uint32_t)Src2.Int8Val);
    else if (BitWidth <= 32)
      Dest.Int32Val = ((uint32_t)Src1.Int32Val) << ((uint32_t)Src2.Int8Val);
    else if (BitWidth <= 64)
      Dest.Int64Val = ((uint64_t)Src1.Int64Val) << ((uint32_t)Src2.Int8Val);
    else {
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n";
      abort();
    }
    maskToBitWidth(Dest, BitWidth);
  } else {
    cerr << "Unhandled type for Shl instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeLShrInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  if (const IntegerType *ITy = cast<IntegerType>(Ty)) {
    unsigned BitWidth = ITy->getBitWidth();
    if (BitWidth <= 8)
      Dest.Int8Val = ((uint8_t)Src1.Int8Val)   >> ((uint32_t)Src2.Int8Val);
    else if (BitWidth <= 16)
      Dest.Int16Val = ((uint16_t)Src1.Int16Val) >> ((uint32_t)Src2.Int8Val);
    else if (BitWidth <= 32)
      Dest.Int32Val = ((uint32_t)Src1.Int32Val) >> ((uint32_t)Src2.Int8Val);
    else if (BitWidth <= 64)
      Dest.Int64Val = ((uint64_t)Src1.Int64Val) >> ((uint32_t)Src2.Int8Val);
    else {
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n";
      abort();
    }
    maskToBitWidth(Dest, BitWidth);
  } else {
    cerr << "Unhandled type for LShr instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

static GenericValue executeAShrInst(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty) {
  GenericValue Dest;
  if (const IntegerType *ITy = cast<IntegerType>(Ty)) {
    unsigned BitWidth = ITy->getBitWidth();
    if (BitWidth <= 8)
      Dest.Int8Val  = ((int8_t)Src1.Int8Val)   >> ((int32_t)Src2.Int8Val);
    else if (BitWidth <= 16)
      Dest.Int16Val = ((int16_t)Src1.Int16Val) >> ((int32_t)Src2.Int8Val);
    else if (BitWidth <= 32)
      Dest.Int32Val = ((int32_t)Src1.Int32Val) >> ((int32_t)Src2.Int8Val);
    else if (BitWidth <= 64)
      Dest.Int64Val = ((int64_t)Src1.Int64Val) >> ((int32_t)Src2.Int8Val);
    else {
      cerr << "Integer types > 64 bits not supported: " << *Ty << "\n"; \
      abort();
    } 
    maskToBitWidth(Dest, BitWidth);
  } else { 
    cerr << "Unhandled type for AShr instruction: " << *Ty << "\n";
    abort();
  }
  return Dest;
}

void Interpreter::visitShl(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  const Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  Dest = executeShlInst (Src1, Src2, Ty);
  SetValue(&I, Dest, SF);
}

void Interpreter::visitLShr(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  const Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  Dest = executeLShrInst (Src1, Src2, Ty);
  SetValue(&I, Dest, SF);
}

void Interpreter::visitAShr(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  const Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  Dest = executeAShrInst (Src1, Src2, Ty);
  SetValue(&I, Dest, SF);
}

#define INTEGER_ASSIGN(DEST, BITWIDTH, VAL)     \
  {                                             \
    uint64_t Mask = ~(uint64_t)(0ull) >> (64-BITWIDTH);     \
    if (BITWIDTH == 1) {                        \
      Dest.Int1Val = (bool) (VAL & Mask);       \
    } else if (BITWIDTH <= 8) {                 \
      Dest.Int8Val = (uint8_t) (VAL & Mask);    \
    } else if (BITWIDTH <= 16) {                \
      Dest.Int16Val = (uint16_t) (VAL & Mask);  \
    } else if (BITWIDTH <= 32) {                \
      Dest.Int32Val = (uint32_t) (VAL & Mask);  \
    } else                                      \
      Dest.Int64Val = (uint64_t) (VAL & Mask);  \
  }

GenericValue Interpreter::executeTruncInst(Value *SrcVal, const Type *DstTy,
                                           ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *DITy = cast<IntegerType>(DstTy);
  const IntegerType *SITy = cast<IntegerType>(SrcTy);
  unsigned DBitWidth = DITy->getBitWidth();
  unsigned SBitWidth = SITy->getBitWidth();
  assert(SBitWidth <= 64 && DBitWidth <= 64  && 
         "Integer types > 64 bits not supported");
  assert(SBitWidth > DBitWidth && "Invalid truncate");

  // Mask the source value to its actual bit width. This ensures that any
  // high order bits are cleared.
  uint64_t Mask = (1ULL << DBitWidth) - 1;
  uint64_t MaskedVal = 0;
  if (SBitWidth <= 8)
    MaskedVal = Src.Int8Val  & Mask;
  else if (SBitWidth <= 16)
    MaskedVal = Src.Int16Val & Mask;
  else if (SBitWidth <= 32)
    MaskedVal = Src.Int32Val & Mask;
  else 
    MaskedVal = Src.Int64Val & Mask;

  INTEGER_ASSIGN(Dest, DBitWidth, MaskedVal);
  return Dest;
}

GenericValue Interpreter::executeSExtInst(Value *SrcVal, const Type *DstTy,
                                          ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *DITy = cast<IntegerType>(DstTy);
  const IntegerType *SITy = cast<IntegerType>(SrcTy);
  unsigned DBitWidth = DITy->getBitWidth();
  unsigned SBitWidth = SITy->getBitWidth();
  assert(SBitWidth <= 64 && DBitWidth <= 64  && 
         "Integer types > 64 bits not supported");
  assert(SBitWidth < DBitWidth && "Invalid sign extend");

  // Normalize to a 64-bit value.
  uint64_t Normalized = 0;
  if (SBitWidth <= 8)
    Normalized = Src.Int8Val;
  else if (SBitWidth <= 16)
    Normalized = Src.Int16Val;
  else if (SBitWidth <= 32)
    Normalized = Src.Int32Val;
  else 
    Normalized = Src.Int64Val;

  Normalized = doSignExtension(Normalized, SITy);

  // Now that we have a sign extended value, assign it to the destination
  INTEGER_ASSIGN(Dest, DBitWidth, Normalized);
  return Dest;
}

GenericValue Interpreter::executeZExtInst(Value *SrcVal, const Type *DstTy,
                                          ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *DITy = cast<IntegerType>(DstTy);
  const IntegerType *SITy = cast<IntegerType>(SrcTy);
  unsigned DBitWidth = DITy->getBitWidth();
  unsigned SBitWidth = SITy->getBitWidth();
  assert(SBitWidth <= 64 && DBitWidth <= 64  && 
         "Integer types > 64 bits not supported");
  assert(SBitWidth < DBitWidth && "Invalid sign extend");
  uint64_t Extended = 0;
  if (SBitWidth == 1)
    // For sign extension from bool, we must extend the source bits.
    Extended = (uint64_t) (Src.Int1Val & 1);
  else if (SBitWidth <= 8)
    Extended = (uint64_t) (uint8_t)Src.Int8Val;
  else if (SBitWidth <= 16)
    Extended = (uint64_t) (uint16_t)Src.Int16Val;
  else if (SBitWidth <= 32)
    Extended = (uint64_t) (uint32_t)Src.Int32Val;
  else 
    Extended = (uint64_t) Src.Int64Val;

  // Now that we have a sign extended value, assign it to the destination
  INTEGER_ASSIGN(Dest, DBitWidth, Extended);
  return Dest;
}

GenericValue Interpreter::executeFPTruncInst(Value *SrcVal, const Type *DstTy,
                                             ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  assert(SrcTy == Type::DoubleTy && DstTy == Type::FloatTy &&
         "Invalid FPTrunc instruction");
  Dest.FloatVal = (float) Src.DoubleVal;
  return Dest;
}

GenericValue Interpreter::executeFPExtInst(Value *SrcVal, const Type *DstTy,
                                           ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  assert(SrcTy == Type::FloatTy && DstTy == Type::DoubleTy &&
         "Invalid FPTrunc instruction");
  Dest.DoubleVal = (double) Src.FloatVal;
  return Dest;
}

GenericValue Interpreter::executeFPToUIInst(Value *SrcVal, const Type *DstTy,
                                            ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *DITy = cast<IntegerType>(DstTy);
  unsigned DBitWidth = DITy->getBitWidth();
  assert(DBitWidth <= 64  && "Integer types > 64 bits not supported");
  assert(SrcTy->isFloatingPoint() && "Invalid FPToUI instruction");
  uint64_t Converted = 0;
  if (SrcTy->getTypeID() == Type::FloatTyID)
    Converted = (uint64_t) Src.FloatVal;
  else
    Converted = (uint64_t) Src.DoubleVal;

  INTEGER_ASSIGN(Dest, DBitWidth, Converted);
  return Dest;
}

GenericValue Interpreter::executeFPToSIInst(Value *SrcVal, const Type *DstTy,
                                            ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *DITy = cast<IntegerType>(DstTy);
  unsigned DBitWidth = DITy->getBitWidth();
  assert(DBitWidth <= 64  && "Integer types > 64 bits not supported");
  assert(SrcTy->isFloatingPoint() && "Invalid FPToSI instruction");
  int64_t Converted = 0;
  if (SrcTy->getTypeID() == Type::FloatTyID)
    Converted = (int64_t) Src.FloatVal;
  else
    Converted = (int64_t) Src.DoubleVal;

  INTEGER_ASSIGN(Dest, DBitWidth, Converted);
  return Dest;
}

GenericValue Interpreter::executeUIToFPInst(Value *SrcVal, const Type *DstTy,
                                            ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *SITy = cast<IntegerType>(SrcTy);
  unsigned SBitWidth = SITy->getBitWidth();
  assert(SBitWidth <= 64  && "Integer types > 64 bits not supported");
  assert(DstTy->isFloatingPoint() && "Invalid UIToFP instruction");
  uint64_t Converted = 0;
  if (SBitWidth == 1)
    Converted = (uint64_t) Src.Int1Val;
  else if (SBitWidth <= 8)
    Converted = (uint64_t) Src.Int8Val;
  else if (SBitWidth <= 16)
    Converted = (uint64_t) Src.Int16Val;
  else if (SBitWidth <= 32)
    Converted = (uint64_t) Src.Int32Val;
  else 
    Converted = (uint64_t) Src.Int64Val;

  if (DstTy->getTypeID() == Type::FloatTyID)
    Dest.FloatVal = (float) Converted;
  else
    Dest.DoubleVal = (double) Converted;
  return Dest;
}

GenericValue Interpreter::executeSIToFPInst(Value *SrcVal, const Type *DstTy,
                                            ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *SITy = cast<IntegerType>(SrcTy);
  unsigned SBitWidth = SITy->getBitWidth();
  assert(SBitWidth <= 64  && "Integer types > 64 bits not supported");
  assert(DstTy->isFloatingPoint() && "Invalid SIToFP instruction");
  int64_t Converted = 0;
  if (SBitWidth == 1)
    Converted = 0LL - Src.Int1Val;
  else if (SBitWidth <= 8)
    Converted = (int64_t) (int8_t)Src.Int8Val;
  else if (SBitWidth <= 16)
    Converted = (int64_t) (int16_t)Src.Int16Val;
  else if (SBitWidth <= 32)
    Converted = (int64_t) (int32_t)Src.Int32Val;
  else 
    Converted = (int64_t) Src.Int64Val;

  if (DstTy->getTypeID() == Type::FloatTyID)
    Dest.FloatVal = (float) Converted;
  else
    Dest.DoubleVal = (double) Converted;
  return Dest;
}

GenericValue Interpreter::executePtrToIntInst(Value *SrcVal, const Type *DstTy,
                                              ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *DITy = cast<IntegerType>(DstTy);
  unsigned DBitWidth = DITy->getBitWidth();
  assert(DBitWidth <= 64  && "Integer types > 64 bits not supported");
  assert(isa<PointerType>(SrcTy) && "Invalid PtrToInt instruction");
  INTEGER_ASSIGN(Dest, DBitWidth, (intptr_t) Src.PointerVal);
  return Dest;
}

GenericValue Interpreter::executeIntToPtrInst(Value *SrcVal, const Type *DstTy,
                                              ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  const IntegerType *SITy = cast<IntegerType>(SrcTy);
  unsigned SBitWidth = SITy->getBitWidth();
  assert(SBitWidth <= 64  && "Integer types > 64 bits not supported");
  assert(isa<PointerType>(DstTy) && "Invalid PtrToInt instruction");
  uint64_t Converted = 0;
  if (SBitWidth == 1)
    Converted = (uint64_t) Src.Int1Val;
  else if (SBitWidth <= 8)
    Converted = (uint64_t) Src.Int8Val;
  else if (SBitWidth <= 16)
    Converted = (uint64_t) Src.Int16Val;
  else if (SBitWidth <= 32)
    Converted = (uint64_t) Src.Int32Val;
  else 
    Converted = (uint64_t) Src.Int64Val;

  Dest.PointerVal = (PointerTy) Converted;
  return Dest;
}

GenericValue Interpreter::executeBitCastInst(Value *SrcVal, const Type *DstTy,
                                             ExecutionContext &SF) {
  
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  if (isa<PointerType>(DstTy)) {
    assert(isa<PointerType>(SrcTy) && "Invalid BitCast");
    Dest.PointerVal = Src.PointerVal;
  } else if (DstTy->isInteger()) {
    const IntegerType *DITy = cast<IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    if (SrcTy == Type::FloatTy) {
      Dest.Int32Val = FloatToBits(Src.FloatVal);
    } else if (SrcTy == Type::DoubleTy) {
      Dest.Int64Val = DoubleToBits(Src.DoubleVal);
    } else if (SrcTy->isInteger()) {
      const IntegerType *SITy = cast<IntegerType>(SrcTy);
      unsigned SBitWidth = SITy->getBitWidth();
      assert(SBitWidth <= 64  && "Integer types > 64 bits not supported");
      assert(SBitWidth == DBitWidth && "Invalid BitCast");
      if (SBitWidth == 1)
        Dest.Int1Val = Src.Int1Val;
      else if (SBitWidth <= 8)
        Dest.Int8Val =  Src.Int8Val;
      else if (SBitWidth <= 16)
        Dest.Int16Val = Src.Int16Val;
      else if (SBitWidth <= 32)
        Dest.Int32Val = Src.Int32Val;
      else 
        Dest.Int64Val = Src.Int64Val;
      maskToBitWidth(Dest, DBitWidth);
    } else 
      assert(0 && "Invalid BitCast");
  } else if (DstTy == Type::FloatTy) {
    if (SrcTy->isInteger())
      Dest.FloatVal = BitsToFloat(Src.Int32Val);
    else
      Dest.FloatVal = Src.FloatVal;
  } else if (DstTy == Type::DoubleTy) {
    if (SrcTy->isInteger())
      Dest.DoubleVal = BitsToDouble(Src.Int64Val);
    else
      Dest.DoubleVal = Src.DoubleVal;
  } else
    assert(0 && "Invalid Bitcast");

  return Dest;
}

void Interpreter::visitTruncInst(TruncInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitSExtInst(SExtInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeSExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitZExtInst(ZExtInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeZExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPTruncInst(FPTruncInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPExtInst(FPExtInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitUIToFPInst(UIToFPInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeUIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitSIToFPInst(SIToFPInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeSIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPToUIInst(FPToUIInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPToUIInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPToSIInst(FPToSIInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPToSIInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitPtrToIntInst(PtrToIntInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executePtrToIntInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitIntToPtrInst(IntToPtrInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeIntToPtrInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitBitCastInst(BitCastInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeBitCastInst(I.getOperand(0), I.getType(), SF), SF);
}

#define IMPLEMENT_VAARG(TY) \
   case Type::TY##TyID: Dest.TY##Val = Src.TY##Val; break

void Interpreter::visitVAArgInst(VAArgInst &I) {
  ExecutionContext &SF = ECStack.back();

  // Get the incoming valist parameter.  LLI treats the valist as a
  // (ec-stack-depth var-arg-index) pair.
  GenericValue VAList = getOperandValue(I.getOperand(0), SF);
  GenericValue Dest;
  GenericValue Src = ECStack[VAList.UIntPairVal.first]
   .VarArgs[VAList.UIntPairVal.second];
  const Type *Ty = I.getType();
  switch (Ty->getTypeID()) {
    case Type::IntegerTyID: {
      unsigned BitWidth = cast<IntegerType>(Ty)->getBitWidth();
      if (BitWidth == 1)
        Dest.Int1Val = Src.Int1Val;
      else if (BitWidth <= 8)
        Dest.Int8Val = Src.Int8Val;
      else if (BitWidth <= 16)
        Dest.Int16Val = Src.Int16Val;
      else if (BitWidth <= 32)
        Dest.Int32Val = Src.Int32Val;
      else if (BitWidth <= 64)
        Dest.Int64Val = Src.Int64Val;
      else
        assert("Integer types > 64 bits not supported");
      maskToBitWidth(Dest, BitWidth);
    }
    IMPLEMENT_VAARG(Pointer);
    IMPLEMENT_VAARG(Float);
    IMPLEMENT_VAARG(Double);
  default:
    cerr << "Unhandled dest type for vaarg instruction: " << *Ty << "\n";
    abort();
  }

  // Set the Value of this Instruction.
  SetValue(&I, Dest, SF);

  // Move the pointer to the next vararg.
  ++VAList.UIntPairVal.second;
}

//===----------------------------------------------------------------------===//
//                        Dispatch and Execution Code
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// callFunction - Execute the specified function...
//
void Interpreter::callFunction(Function *F,
                               const std::vector<GenericValue> &ArgVals) {
  assert((ECStack.empty() || ECStack.back().Caller.getInstruction() == 0 ||
          ECStack.back().Caller.arg_size() == ArgVals.size()) &&
         "Incorrect number of arguments passed into function call!");
  // Make a new stack frame... and fill it in.
  ECStack.push_back(ExecutionContext());
  ExecutionContext &StackFrame = ECStack.back();
  StackFrame.CurFunction = F;

  // Special handling for external functions.
  if (F->isDeclaration()) {
    GenericValue Result = callExternalFunction (F, ArgVals);
    // Simulate a 'ret' instruction of the appropriate type.
    popStackAndReturnValueToCaller (F->getReturnType (), Result);
    return;
  }

  // Get pointers to first LLVM BB & Instruction in function.
  StackFrame.CurBB     = F->begin();
  StackFrame.CurInst   = StackFrame.CurBB->begin();

  // Run through the function arguments and initialize their values...
  assert((ArgVals.size() == F->arg_size() ||
         (ArgVals.size() > F->arg_size() && F->getFunctionType()->isVarArg()))&&
         "Invalid number of values passed to function invocation!");

  // Handle non-varargs arguments...
  unsigned i = 0;
  for (Function::arg_iterator AI = F->arg_begin(), E = F->arg_end(); AI != E; ++AI, ++i)
    SetValue(AI, ArgVals[i], StackFrame);

  // Handle varargs arguments...
  StackFrame.VarArgs.assign(ArgVals.begin()+i, ArgVals.end());
}

void Interpreter::run() {
  while (!ECStack.empty()) {
    // Interpret a single instruction & increment the "PC".
    ExecutionContext &SF = ECStack.back();  // Current stack frame
    Instruction &I = *SF.CurInst++;         // Increment before execute

    // Track the number of dynamic instructions executed.
    ++NumDynamicInsts;

    DOUT << "About to interpret: " << I;
    visit(I);   // Dispatch to one of the visit* methods...
  }
}
