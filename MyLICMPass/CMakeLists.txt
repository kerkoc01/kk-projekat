add_llvm_library( MyLICMPass MODULE

        ConstantFolding.cpp
        
        MyLICMPass.cpp
        
        PLUGIN_TOOL
        opt
)
