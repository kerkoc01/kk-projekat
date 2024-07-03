#ifndef CONSTANT_FOLDING_H
#define CONSTANT_FOLDING_H

#include "llvm/IR/Function.h"
#include <vector>

namespace llvm {
    bool performConstantFolding(Function &F);
}

#endif // CONSTANT_FOLDING_H
