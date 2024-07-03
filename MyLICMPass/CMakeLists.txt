add_llvm_library( MyLICMPass MODULE
        MyLICMPass.cpp

        PLUGIN_TOOL
        opt
)