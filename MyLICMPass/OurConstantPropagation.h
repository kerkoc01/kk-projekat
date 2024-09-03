#ifndef CONSTPROPAGATION_H
#define CONSTPROPAGATION_H

#include "llvm/IR/Function.h"

using namespace llvm;

bool performConstantPropagation(Function &F);

#endif // CONSTPROPAGATION_H

