#ifndef CONSTPROPAGATION_H
#define CONSTPROPAGATION_H

#include "llvm/IR/Function.h"

bool performConstantPropagation(llvm::Function &F);

#endif // CONSTPROPAGATION_H

