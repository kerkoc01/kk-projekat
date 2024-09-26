add_llvm_library( MyLICMPass MODULE BUILDTREE_ONLY
        MyLICMPass.cpp
        ConstantFolding.cpp
        ConstantPropagation.cpp
        DeadCodeElimination.cpp
        ConstantPropagationInstruction.cpp
        OurCFG.cpp

        DEPENDS
        intrinsics_gen
        PLUGIN_TOOL
        opt
)