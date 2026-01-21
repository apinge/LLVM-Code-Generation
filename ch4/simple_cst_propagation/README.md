# Build your own simple constant propagation #

In this exercise you need to populate the code in `your_turn/populate_function.cpp` by following the instruction given in the comments of the `myConstantPropagation` function.

At any point in the development, you can build and test your code using the commands given below.

If you get stuck, feel free to look at the reference implementation in `solution/populate_function.cpp`.

When running the program you will see that it prints which implementation managed to transform the input IR.
Try to beat the reference implementation by supporting more cases than it does!

## Configure your build directory ##

```bash
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR=<path/to/llvm/install>/lib/cmake/llvm -Bbuild .
```

This will initialize your build directory in `build` (the `-B` option) with Ninja (`-G` option).

You must have a version of LLVM installed at `<path/to/llvm/install>` for this to succeed.
Either build and install your own llvm (with the `install` target from your LLVM build) or install an [official LLVM release](https://releases.llvm.org/) package.

## Build ##

```bash
ninja -C build
```

This builds the default target in the build directory.

This should produce in the `build` directory a binary named `simple_cst_propagation`.

## Run ##

```bash
./build/simple_cst_propagation [input.ll|.bc]
```

This will run both the reference implementation and your implementation on `input.ll` if specified or the default input if not.

The run will apply both implementions to the input and will check whether an optimization happened, and if the resulting IR is correct.

It will also report which implementation managed to optimize the input IR.

To see how this is articulated, take a look at `main.cpp`.

For each function in the input IR, the output will look like this:
```
Processing function '<current_func>'
[The input IR for function <current_func>]

## Reference implementation
[Resulting IR after the reference optimization]


## Your implementation
[Resulting IR after your optimization]

<message about the correctness of the resulting IRs>

<message about which optimization managed to transform this function>
######
```

The message after each processing will tell you if you did better than the reference implementation, i.e., you transformed the input IR and the reference implementation did not.

## Producing an input ##

Using the following command line, you can produce from a C file an input to give to your program:
```bash
clang -o - -S -emit-llvm input.c -O0 | sed -e 's#optnone##g' | <path/to/llvm/build>/bin/opt -S -passes=mem2reg,instnamer > input.ll
```

optnone => remove the attribute that prevents optimizations
mem2reg => get rid of stack accesses / build SSA
instnamer => get rid of the implicit variables

## IR讲解
原始函数
- foo

```mlir
;Processing function 'foo
define i32 @foo(i32 noundef %arg) {
bb:
  %i = shl i32 5, 3
  %i1 = icmp ne i32 %arg, 0 ; 
  br i1 %i1, label %bb2, label %bb4

bb2:                                              ; preds = %bb
  %i3 = sdiv i32 %i, 5
  br label %bb6

bb4:                                              ; preds = %bb
  %i5 = or i32 %i, 3855
  br label %bb6

bb6:                                              ; preds = %bb4, %bb2
  %.0 = phi i32 [ %i3, %bb2 ], [ %i5, %bb4 ]
  ret i32 %.0
}
```
**icmp ne**`:=` not equal 这句话表示 `bool i1 = (arg !=0);`
**sdiv** = signed divide（有符号除法）这句话 `%i3 = sdiv i32 %i, 5`里的 `i32` **同时修饰左右操作数**是说明这是个i32类型的除法 `int i3 = i/5;`

- bar
```mlir
;Processing function 'bar
define i32 @bar(i32 noundef %arg) {
bb:
  %i = shl i32 -1, 3
  %i1 = icmp ne i32 %arg, 0
  br i1 %i1, label %bb2, label %bb4

bb2:                                              ; preds = %bb
  %i3 = udiv i32 %i, 3
  br label %bb6

bb4:                                              ; preds = %bb
  %i5 = or i32 %i, 3855
  br label %bb6

bb6:                                              ; preds = %bb4, %bb2
  %.0 = phi i32 [ %i3, %bb2 ], [ %i5, %bb4 ]
  %i7 = add i32 %.0, 1
  ret i32 %i7
}

```

### ReversePostOrderTraversal（RPO）

对于`foo` bb->bb2->bb2->bb6
```C++
for(BasicBlock* bb:rpot){
    llvm::outs() << "[DEBUG] bbloop\n";
    for (Instruction &instr : make_early_inc_range(*bb)) {
        auto op = instr.getOpcode();
        llvm::outs() << "[DEBUG]" << instr <<"\n";
    }
  }
```
RPO是post order的反转
![DFS](https://media.geeksforgeeks.org/wp-content/uploads/20250913162554418810/tree_construction_from_given_inorder_and_preorder_traversals_8.webp)

RPO不同于拓扑排序不一定保证父一定传到子