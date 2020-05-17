# Tomasulo 实验报告

本次实验要求实现一个 Tomasulo 的模拟器。在完成基本要求的基础上，实现了 Jump 指令的支持，并且另外编写了一个不采用 Tomasulo 直接逐指令模拟的模拟器，通过比对两个模拟器的输出，判断执行结果是否正确。

## 代码组织

主要有三个源文件：

1. parser.cpp: 解析 NEL 代码
2. tomasulo.cpp：实现 Tomasulo 模拟器
3. simulator.cpp：实现非 Tomasulo 模拟器

编译方法：

```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ make
```

编译会生成两个可执行文件：`tomasulo` 和 `simulator`，分别对应上面的两个模拟器。

## Tomasulo 模拟器的执行

在上一步编译完成后，可以运行 `build/tomasulo` 进行一次模拟，它的参数如下：

```
Usage: ./tomasulo input_nel output_log [output_trace] [output_reg]
```

第一个参数必选，为输入的 nel 文件。
第二个参数必选，为输出的 log 文件，满足实验要求。
第三个参数可选，为输出的 trace 文件，内容是每个周期的调试信息。
第四个参数可选，为输出的 reg 文件，内容是执行完毕时寄存器的信息。

如果只需要输出 log 文件，传递两个参数即可。

Trace 文件里面有每个周期的信息，一个例子（取自 Example）：

```
Cycle: 1
Reservation Stations:
LB 9: LOAD vj=00000002
Execution Unit:
Load 5: LOAD rd=01 vj=00000002 cycles=3
Register Status:
R[01]=9 
```

表示了各个保留站、执行单元和寄存器状态的信息。为了简单期间，Load Buffer 和 Reservation Station 进行了合并，这里的编号都是全局的，从 0 开始，与课件上表示方法不同，有如下的对应关系：

```
保留站：
Ars 1 <-> Ars 0
Ars 2 <-> Ars 1
Ars 3 <-> Ars 2
Ars 4 <-> Ars 3
Ars 5 <-> Ars 4
Ars 6 <-> Ars 5
Mrs 1 <-> Mrs 6
Mrs 2 <-> Mrs 7
Mrs 3 <-> Mrs 8
LB 1 <-> LB 9
LB 2 <-> LB 10
LB 3 <-> LB 11

功能单元：
Add 1 <-> Add 0
Add 2 <-> Add 1
Add 3 <-> Add 2
Mult 1 <-> Mult 3
Mult 2 <-> Mult 4
Load 1 <-> Load 5
Load 2 <-> Load 6
```

Reg 文件每行表示一个寄存器的值。

## 非 Tomasulo 模拟器的执行

为了验证 Tomasulo 模拟器的正确性，额外编写了一个逐指令模拟的模拟器 `build/simulator`。参数如下：

```
Usage: ./simulator input_nel output_trace output_reg
```

第一个参数是输入的 nel 文件
第二个参数是输出的 trace 文件，记录每个周期写回的信息。
第三个参数是输出的 reg 文件，记录执行完毕时寄存器的状态。

## Tomasulo 模拟器正确性验证

首先，按照所给的 Example 的输出，生成其对应的 trace，与 pdf 课件上一一对比，完全吻合。

其次，对于提供的所有 nel 测例，都用两个模拟器执行，将执行结果的寄存器状态进行比对（在 TestCase 目录下执行 `make diff` 即可）。
