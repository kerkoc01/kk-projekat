<<<<<<< Updated upstream
add_llvm_library( MyLICMPass MODULE
        MyLICMPass.cpp

=======
add_llvm_library( MyLICMPass MODULE BUILDTREE_ONLY
        MyLICMPass.cpp
        ConstantFolding.cpp
        ConstantPropagation.cpp
        DeadCodeElimination.cpp
        ConstantPropagationInstruction.cpp
        OurCFG.cpp

        DEPENDS
        intrinsics_gen
>>>>>>> Stashed changes
        PLUGIN_TOOL
        opt
)