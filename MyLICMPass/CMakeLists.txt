add_llvm_library( MyLICMPass MODULE BUILDTREE_ONLY
  	ConstantPropagationInstruction.cpp
	OurConstantPropagation.cpp
        ConstantFolding.cpp
        
        MyLICMPass.cpp
        
        PLUGIN_TOOL
        opt
)
