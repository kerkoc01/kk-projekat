# kk-projekat
## Loop Invariant Code Motion (LICM) LLVM Optimization

This is an implementation of **Loop Invariant Code Motion** LLVM optimization.

## Getting Started

To run it, follow these steps:

1. Place the `make_llvm.sh` script in the `llvmproject/` directory.
2. Move the entire `MyLICMPass` directory to `llvmproject/llvm/lib/Transforms/` directory.
3. Update the `llvmproject/llvm/lib/Transforms/CMakeLists.txt` and add the following line:
	```plaintext
	add_subdirectory(MyLICMPass)
   
## Running the Optimization

To compile and run your code with this optimization:

1. Place your `.c` file in the `llvmproject/build/` directory.
2. From the `llvmproject/build/` directory, execute the following commands:
	```bash
	./bin/clang -S -emit-llvm your-c-file-name.c
	./bin/opt -S -load lib/MyLICMPass.so -enable-new-pm=0 -my-licm your-c-file-name.ll -o -output.ll
3. The optimized code will be available in `output.ll`.
