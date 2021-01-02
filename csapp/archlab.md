## 简介

本章为《深入理解计算机系统》第四章——处理器体系结构，从一个精简版的指令集 `Y86-64` 开始，将指令的执行分成五个阶段，并首先实现了一个顺序版的处理器 `SEQ` 使得我们能在一个超长的周期内执行完一条指令，再引入流水线并增加寄存器保存每个阶段状态以并行执行，又增加转发逻辑使得两条指令相互依赖的值能快速传递，最后引入了流水线控制逻辑处理特殊情况，最终完成了一个代表 20 世纪 80 年代中期的处理器设计水平的处理器 `PIPE` 。

由于本章涉及的知识很多都没接触过，很容易就懵，看了两遍才大致理解一些理论方面的知识，各种书本上具体的用例讲解基本能够理解但没有太多深究，在此也仅回顾本章的核心知识点，其他方面还需要结合 Lab 再研究。

## 知识点回顾

### 出入栈指令

`pushq %rsp`: 先将寄存器 `%rsp` 内的值放入栈顶，再将寄存器 `%rsp` 内的值减去 `8` ，即： `movq %rsp, -8(%rsp); subq 8, %rsp;`

`popq %rsp`: 先取出栈顶的值 `valM` ，再将寄存器 `%rsp` 内的值加上 `8` ，最后再将 `valM` 放入寄存器 `%rsp` 内，即： `addq 8, %rsp; movq -8(%rsp), %rsp;`

出入栈指令其实很简单明了，可以发现写成对应的指令后互为相反操作。寄存器 `%rsp` 与其他寄存器没有任何区别，不需要单独记忆，不要被特殊寄存器干扰。

对应习题： `4.7`,`4.8` (`P255`) ，对应家庭作业： `4.45`, `4.46` (`P327`)，可以查看书中 `P268` 所示的出入栈每个阶段的硬件操作加深记忆，从硬件本质上理解是如何处理的。

![图 4-20 Y86-64 指令 pushq 和 popq 在顺序实现中的计算.png](archlab/图%204-20%20Y86-64%20指令%20pushq%20和%20popq%20在顺序实现中的计算.png)

### 硬件设计

实现一个数字系统需要三个主要的组成部分：组合逻辑、存储器和时钟信号。 `P256`

#### 组合逻辑

组合逻辑简单地响应输入的变化，产生等于输入的某个函数的输出，不会进行部分求值（高级语言中的逻辑计算存在部分求值，即短路语义） `P258`

#### 存储器

存储器按位存储信息，由同一个时钟控制将新值加载到存储器中，考虑以下两类存储器： `P262`
- 时钟寄存器（简称寄存器）：存储单个位或字
- 随机访问存储器（简称内存）：存储多个字，用地址来选择读写哪个字。包括：虚拟内存系统、寄存器文件

在硬件中，寄存器直接将它的输入和输出线连接到电路的其他部分，可称为硬件寄存器（时钟寄存器）；在机器级编程中，寄存器代表 CPU 中为数不多的可寻址的字，地址为寄存器 ID ，可称为程序寄存器。 `P262`

硬件寄存器大多数时候保持在稳定状态，产生的输出等于它的当前状态。只要时钟是低电位，寄存器的输出就保持不变；当时钟变成高电位时，输入信号就会加载到寄存器中，从而输出也会变化为新的状态。硬件寄存器作为电路中不同部分中的组合逻辑之间的屏障，当每个时钟到达上升沿时，值才会从硬件寄存器的输入传送到输出。 `P262`

### SEQ 处理器 `P264`

SEQ 处理器是 `Y86-64` 的顺序实现，即每个周期执行处理完一条完整指令所需的所有步骤。

SEQ 的实现中有四个硬件单元需要通过时钟信号来控制：程序计数器、条件码寄存器、数据内存和寄存器文件。这四个硬件单元存储的数据视作状态，而这些所有状态更新实际上是同时发生的，且只在时钟上升开始下一个周期时。 `P275`

- 本周期内执行的指令阶段操作都通过组合逻辑进行传递，不存储任何状态，完成全部组合逻辑计算后，在下一个周期开始时才将状态存储到对应的硬件中。
- 状态虽然会在下一个周期才会被存储，但在下一条指令执行前，所有的状态都会更新（书中 `P276` 有以下相关图示）

![图 4-25 信号传播通过组合逻辑，在下一个周期开始时，新值会被加载到状态单元中.png](archlab/图%204-25%20信号传播通过组合逻辑，在下一个周期开始时，新值会被加载到状态单元中.png)

`Y86-64` 指令集遵循从不回读的原则：处理器从来不需要为了完成一条指令的执行而去读由该指令更新了的状态。 `P275`

- 第一遍读到这里时很懵，前面没有看懂导致这里怎么都想不通了；第二遍再看时把前后相关的部分串起来理解，很多问题都迎刃而解了。这里的状态如上所说是前面介绍的四个硬件单元中存储的状态（并非具体阶段中组合逻辑计算出来的值，也非硬件寄存器中存储的阶段状态），即当前指令写入的状态不会由当前指令后续的阶段读取到，因为这些状态只会在当前指令完成后才被存储到硬件单元中。

当同一个周期内两个写端口都试图对一个程序寄存器继续写时，只有较高优先级端口上的写才会发生。寄存器文件中的 M 写端口优先级高于 E 写端口，因此执行 `popq %rsp` 时能满足前面所提到的确定行为，使得行为类似： `addq 8, %rsp; movq -8(%rsp), %rsp;` （对应习题： `4.22` (`P279`)）

### 预测下一个 PC

为了使得每个周期都有一个新指令开始执行，那么必须在取出当前指令后马上确定下一个位置。有时候我们不能确定当前指令的下一条指令的位置，这就需要我们进行预测。 `P294`

- 条件转移指令：下一条指令的地址要么是 `valC` （选择了分支），要么是 `valP` （没有选择分支）。我们实现的流水线总是预测选择了分支，即下一条指令的地址是 `valC`
- `ret` 指令：下一条指令需要等当前指令通过访存阶段后才能确定。我们实现的流水线仅简单暂停处理新指令，直到 `ret` 指令通过写回阶段
    - 开始执行 `ret` 指令后，由于前面的指令还在流水线中，很可能栈顶不是返回地址（回想函数的汇编代码可以发现， `ret` 指令前面往往还有一条 `subq xx, %rsp` 的指令，用于释放当前函数使用的栈空间）
    - 对大多数程序来说，预测返回值很容易，因为 `call` 和 `ret` 是成对出现的。可以通过在取指单元中放入一个硬件栈，每次执行 `call` 指令时，将其返回地址压入栈中；当执行 `ret` 指令时就弹出栈顶的值，作为预测的返回地址
- `call` 和 `jmq` 指令：下一条指令的地址是指令中的常数数字 `valC`
- 其他指令：下一条指令的地址就是根据 PC 和当前指令长度计算后的数字 `valP`

分支预测错误会极大地降低程序的效率，因此在可能的时候，要使用条件数据传送而不是条件控制转移。 `P295`

很久以前就看到 StackOverflow 上一个分支预测相关的问题：[为什么处理一个有序数组比处理一个无序数组快？](https://stackoverflow.com/questions/11227809/why-is-processing-a-sorted-array-faster-than-processing-an-unsorted-array) 第一条回答中也提到了 `GCC` 在 `-O3` 下会在可能的时候会使用条件数据传送。（结合前段时间看的一篇关于编译器优化的分享后，觉得我们写代码时其实不用过分关注每一段具体代码的相关效率，很多都能被编译器优化（例如：`i++` 和 `++i` 开了优化都一样），我们更应该关注整体设计方面的效率。

### 流水线冒险

当相邻指令间存在相关时会导致出现问题，这些相关可能会导致流水线产生计算错误，称为冒险。 `P295`

- 数据冒险：下一条指令会用到这一条指令计算出的结果
- 控制冒险：一条指令要确定下一条指令的位置，例如执行跳转、调用和返回指令时

数据冒险的可能性分析： `P298`

- 程序寄存器：寄存器文件的读写在不同的阶段，不同指令之间可能出现相互作用
- 程序计数器：更新和读取程序计数器之间的冲突导致了控制冒险，只有预测错误的分支和 `ret` 指令才会产生冒险，需要特殊处理
- 内存：对数据内存的读和写都发生在访存阶段，当一条读内存的指令到达访存阶段时，前面所有要写内存的指令都已经执行完成这个阶段了，所以一般情况下不会出现冒险（有些能修改自身代码的程序会出现冒险，因为指令内存的访问在取指阶段）
- 条件码寄存器：整数操作会在执行阶段写这些寄存器，而条件传送指令会在执行阶段读这些寄存器、条件转移指令会在访存阶段读这些寄存器，当条件传送指令/条件转移指令到达执行阶段时，前面所有要写条件码寄存器的指令都已经执行完成这个阶段了，所以不会出现冒险
- 状态寄存器：每条指令都有与之相关的状态码相关联，当异常发生时，处理器需要有条理地停止

综上所述：我们只需要处理程序寄存器数据冒险、控制冒险，并确保能够正确处理异常即可。 `P298`

1. 用暂停来避免数据冒险：当存在数据冒险时，处理器会让接下来的一条指令停顿在译码阶段，直到它的源操作数的指令通过了写回阶段。机器通过动态插入一条 `nop` 指令，使得当前指令的后续阶段执行 `nop` 指令 `P298`
2. 用转发来避免数据冒险：又称旁路，将结果值直接从一个流水线阶段传到较早阶段。通过增加一些额外的数据连接和控制逻辑，将以下五个转发源直接传到译码阶段，转发目的为 `valA` 和 `valB`： `P300`
    - 写回阶段对程序寄存器未进行的写：转发源为 `W_valE` 和 `W_valM`
    - 访存阶段对程序寄存器未进行的写：这个写操作的寄存器地址和值已经确定，将会在写回阶段执行，转发源为 `M_valE` 和 `m_valM` ，其中 `m_valM` 为刚刚从内存中读出的值且其目标为写端口 `M`
    - 执行阶段对程序寄存器未进行的写：这个写操作的寄存器地址和值已经确定，将会在写回阶段执行，转发源为 `e_valE` ，其中 `e_valE` 为 ALU 刚刚计算出的值且其目标为写端口 `E`
    ![图 4-52 流水线化的最终实现—— PIPE 的硬件结构.png](archlab/图%204-52%20流水线化的最终实现——%20PIPE%20的硬件结构.png)
3. 加载/使用数据冒险：加载/使用数据冒险不能单纯通过转发来解决，因为内存读在流水线发生的比较晚。例如 `mrmovq (%rdx), %rax; addq %rbx, %rax;` 就存在这种冒险，当 `mrmovq` 指令刚刚从内存中读出值时， `addq` 指令早在上一个周期已经使用了对应的寄存器，转发逻辑无法将值送回到过去。我们可以将暂停和转发结合起来避免加载/使用数据冒险（这种方法称为加载互锁），这种情况下只需要暂停 `addq` 指令一个周期即可转发 `mrmovq` 从内存中读出的值 `P303`
4. 避免控制冒险：当处理器无法根据处于取指阶段的当前指令来确定下一条指令的地址时，就会出现控制冒险。正如前面所提到的，控制冒险只会发生在 `ret` 指令（暂停处理新指令）和条件转移指令（总是预测会进行跳转），且条件传送指令只有在分支预测错误时才会造成麻烦。因此我们只需要处理条件传送指令分支预测错误这种情况，此时我们需要取消错误指令的执行，并开始执行正确的指令（条件传送指令后面的那条指令） `P304`
    - 条件传送指令会在执行阶段就计算出条件码，而此时后面只有两条错误指令，分别处于取指阶段和译码阶段，这两个阶段都不会修改程序员可见状态，所以可以简单取消它们后续阶段的执行 `P306`

### 异常处理

`Y86-64` 指令集体系结构包括三种不同的内部产生的异常，暂不处理网络接口接收新包、点击鼠标等外部异常。 `P306`

- `halt` 指令
- 有非法指令和功能码组合的指令
- 取指或数据读写试图访问一个非法地址

在设计流水线的处理器时，我们将异常状态和该指令的其他信息一起沿着流水线传播，直到它到达写回阶段。在此，流水线控制逻辑发现出现了异常，并停止执行。 `P307`

指令的异常有三个细节需要注意： `P307`

- 同时存在多个指令的异常：让流水线中最深的指令（最早开始的指令）引起的异常优先级最高
- 一条引起异常的指令由于分支预测错误被取消：如前面所说，异常状态会沿流水线传播，且只在访存/写回阶段才会进行处理，而由于分支错误被取消的指令最多只达到译码阶段，所以被取消的异常指令不会修改程序员可见状态，也不会造成其他影响
- 所有引起异常的指令后面的指令都不能改变程序员可见状态：当处于访存/写回阶段中的指令引起异常时，流水线控制逻辑需要禁止更新条件码寄存器和数据内存

### 流水线控制逻辑

前面提到的数据转发和分支预测只能处理部分逻辑，以下四种情况只能由流水线控制逻辑处理： `P314`

- 加载/使用冒险：在一条从内存中读出一个值的指令和一条使用该值的指令之间，流水线必须暂停一个周期
- `ret` 指令：流水线必须暂停直到 `ret` 指令到达写回阶段
- 分支预测错误：取消错误的指令，并从跳转指令后面的那条指令开始取指
- 异常：禁止异常指令后面的指令更新程序员可见状态，并且在异常指令到达写回阶段时，停止执行
    - 禁止执行阶段的指令设置条件码
    - 向内存阶段中插入气泡，以禁止向数据内存中写入
    - 当写回阶段有异常指令时，暂停写回阶段，从而暂停流水线

## 准备

可以在 [官网](http://csapp.cs.cmu.edu/3e/labs.html) 下载 Architecture Lab 相关的程序。

开始前需要阅读 [Architecture Lab writeup](http://csapp.cs.cmu.edu/3e/archlab.pdf)  （刚刚下载的压缩包内也有），可以知道本次 Lab 由 3 部分组成。

本次需要使用的程序依旧需要在 Docker 中运行，将本地 Lab 的目录挂载进容器中即可：

```shell script
docker run -ti -v {PWD}:/csapp ubuntu:18.04
```

进入容器后需要安装一些必须软件以便后续能成功运行：

```shell script
apt-get update && apt-get -y install gcc make flex bison libgetopt-complete-perl
```

由于我们不使用 GUI 模式且没有安装相关依赖，所以需要注释掉 `sim/Makefile` 文件中的 `GUIMODE`, `TKLIBS`, `TKINC` 这三行，然后在 `sim` 文件夹下运行 `make clean && make` 生成本次所需的程序。

然后就可以愉快的开始闯关了。

## 闯关

### 第一部分

第一部分主要是写 3 个简单的 `Y86-64` 程序，并且熟悉相关的工具，本部分所有操作均在 `sim/misc` 文件夹内进行。这三个程序都是使用 `Y86-64` 指令集的汇编程序，与常用的汇编语言无太大差别，看完本章就开始做不需要额外的参考资料。

#### 循环求链表和

第一个程序 `sum.ys` 需要实现一个函数 `sum_list` 使用循环对一个链表求和，对应的 C 语言代码如下：

```c
/* linked list element */
typedef struct ELE {
    long val;
    struct ELE *next;
} *list_ptr;

/* sum_list - Sum the elements of a linked list */
long sum_list(list_ptr ls)
{
    long val = 0;
    while (ls) {
        val += ls->val;
        ls = ls->next;
    }
    return val;
}
```

刚开始还不知道怎么写，发现 `sim/y86-code/asum.ys` 内包含完整的程序，就直接复制一份开始改写即可：
- 将全局变量替换为我们的 3 节点链表
- 将 `sum` 函数替换为我们的需要实现的链表求和逻辑 `sum_list`
- 将 `main` 函数中的逻辑改为执行 `sum_list(ele1)` 的逻辑

改写完的代码如下：

```assemble
# Execution begins at address 0 
    .pos 0
    irmovq stack, %rsp  # Set up stack pointer
    call main           # Execute main program
    halt                # Terminate program 

# Sample linked list
    .align 8
ele1:
    .quad 0x00a
    .quad ele2
ele2:
    .quad 0x0b0
    .quad ele3
ele3:
    .quad 0xc00
    .quad 0

main:
    irmovq ele1,%rdi
    call sum_list     # sum_list(ele1)
    ret

# long sum_list(list_ptr ls)
# ls in %rdi
sum_list:
    irmovq $8,%r8        # Constant 8
    xorq %rax,%rax       # sum = 0
    andq %rdi,%rdi       # Set CC
    jmp     test         # Goto test
loop:
    mrmovq (%rdi),%r9    # Get ls -> val
    addq %r9,%rax        # Add to sum
    mrmovq 8(%rdi),%rdi  # ls = ls -> next
    andq %rdi,%rdi       # Set CC
test:
    jne    loop          # Stop when 0
    ret                  # Return

# Stack starts here and grows to lower addresses
    .pos 0x200
stack:
```

然后可以测试我们的程序：

```shell script
> ./yas sum.ys && ./yis sum.yo
Stopped in 27 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:	0x0000  0x0cba
%rsp:	0x0000  0x0200
%r8:	0x0000  0x0008
%r9:	0x0000  0x0c00

Changes to memory:
0x01f0:	0x0000  0x005b
0x01f8:	0x0000  0x0013
```

可以发现程序运行完的状态没有问题，寄存器 `%rax` 中的值为 `0xcba` ，与实际求和结果相符，表明我们程序没有问题。内存更改有两处，都是栈中的值，分别是调用 `main` 和 `sum_list` 的指令的下一条指令，用于函数内部返回用。

#### 递归求链表和

第二个程序 `rsum.ys` 需要实现一个函数 `rsum_list` 使用递归对一个链表求和，对应的 C 语言代码如下：

```c
/* rsum_list - Recursive version of sum_list */
long rsum_list(list_ptr ls)
{
    if (!ls) {
        return 0;
    } else {
        long val = ls->val;
        long rest = rsum_list(ls->next);
        return val + rest;
    }
}
```

我们直接复制 `sum.ys` 为 `rsum.ys` ，然后将 `sum_list` 的更改为递归求和逻辑即可：

```assemble
...
main:
    irmovq ele1,%rdi
    call rsum_list       # rsum_list(ele1)
    ret

# long rsum_list(list_ptr ls)
# ls in %rdi
rsum_list:
    xorq %rax,%rax       # sum = 0
    andq %rdi,%rdi       # Set CC
    je     return        # Goto return
    pushq  %rbx          # Save callee-saved register
    mrmovq (%rdi),%rbx   # Get ls -> val
    mrmovq 8(%rdi),%rdi  # Set arg
    call rsum_list       # rsum_list(ls -> next)
    addq %rbx,%rax       # Add to sum
    popq %rbx            # Restore callee-saved register
return:
    ret                  # Return
...
```

然后可以测试我们的程序：

```shell script
> ./yas rsum.ys && ./yis rsum.yo
Stopped in 40 steps at PC = 0x13.  Status 'HLT', CC Z=0 S=0 O=0
Changes to registers:
%rax:	0x0000  0x0cba
%rsp:	0x0000  0x0200

Changes to memory:
0x01c0:	0x0000  0x0088
0x01c8:	0x0000  0x00b0
0x01d0:	0x0000  0x0088
0x01d8:	0x0000  0x000a
0x01e0:	0x0000  0x0088
0x01f0:	0x0000  0x005b
0x01f8:	0x0000  0x0013
```

可以发现程序运行完的状态没有问题，寄存器 `%rax` 中的值为 `0xcba` ，与实际求和结果相符，表明我们程序没有问题。内存更改有七处，都是栈中的值，其中五处用于保存调用指令的返回地址，两处 (`0x01c8`, `0x01d8`) 用于保存被调用者保存寄存器。

#### 拷贝数组

第三个程序 `copy.ys` 需要实现一个函数 `copy_block` 使用循环，将 `src` 中的 `len` 个数复制到 `dest` 中，并返回这些数的异或和，对应的 C 语言代码如下：

```c
/* copy_block - Copy src to dest and return xor checksum of src */
long copy_block(long *src, long *dest, long len)
{
    long result = 0;
    while (len > 0) {
        long val = *src++;
        *dest++ = val;
        result ^= val;
        len--;
    }
    return result;
}
```

我们直接复制 `rsum.ys` 为 `copy.ys` ，然后做如下修改即可：

- 将全局变量替换为长度为 3 的 `long` 数组
- 将 `rsum_list` 函数替换为我们的需要实现的拷贝函数 `copy_block`
- 将 `main` 函数中的逻辑改为执行 `copy_block(src, dest, 3)` 的逻辑

```assemble
...
    .align 8
# Source block
src:
    .quad 0x00a
    .quad 0x0b0
    .quad 0xc00
# Destination block
dest:
    .quad 0x111
    .quad 0x222
    .quad 0x333

main:
    irmovq src,%rdi
    irmovq dest,%rsi
    irmovq $3,%rdx
    call copy_block       # copy_block(src, desc, 3)
    ret

# long copy_block(long *src, long *dest, long len)
# src in %rdi, dest in %rsi, len in rdx
copy_block:
    irmovq $8,%r8        # Constant 8
    irmovq $1,%r9        # Constant 1
    xorq %rax,%rax       # result = 0
    jmp test             # Goto test
loop:
    mrmovq (%rdi),%r10   # Get *src
    xorq %r10,%rax       # result ^= *src
    rmmovq %r10,(%rsi)   # *dest = *src
    addq %r8,%rdi        # src++
    addq %r8,%rsi        # dest++
    subq %r9,%rdx        # len--
test:
    andq %rdx,%rdx       # Set CC （把 test 写在最后可以在循环中减少一次 jmq 跳转）
    jne loop             # Go to loop
    ret                  # Return
...
```

然后可以测试我们的程序：

```shell script
> ./yas copy.ys && ./yis copy.yo
Stopped in 39 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:	0x0000  0x0cba
%rsp:	0x0000  0x0200
%rsi:	0x0000  0x0048
%rdi:	0x0000  0x0030
%r8:	0x0000  0x0008
%r9:	0x0000  0x0001
%r10:	0x0000  0x0c00

Changes to memory:
0x0030:	0x0111  0x000a
0x0038:	0x0222  0x00b0
0x0040:	0x0333  0x0c00
0x01f0:	0x0000  0x006f
0x01f8:	0x0000  0x0013
```

可以发现程序运行完的状态没有问题，寄存器 `%rax` 中的值为 `0xcba` ，与实际异或和结果相符，且 `dest` 中的值已经改为 `src` 中对应的值，表明我们程序没有问题。其余的内存更改还有两处，都是栈中的值，分别是调用 `main` 和 `copy_block` 的指令的下一条指令，用于函数内部返回用。

### 第二部分

第二部分主要是改写 `sim/seq/seq-full.hcl` 文件，使得已有的 `SEQ` 处理器支持 `iaddq` 这条指令（对应家庭作业 `4.51`, `4.52` (`P328`)），本部分所有操作均在 `sim/seq` 文件夹内进行。

#### `iaddq` 的不同阶段处理

![图 4-18 指令 OPq, rrmovq 和 irmoq 在顺序实现中的计算](archlab/图%204-18%20指令%20OPq,%20rrmovq%20和%20irmoq%20在顺序实现中的计算.png)

首先就是需要参照图 4-18 根据 `addq` 指令列出 `iaddq` 指令的不同阶段处理情况。

| 阶段 | `iaddq V, rB` |
| --- | --- |
| 取指 | icode:ifun ← M1[PC] <br/> rA:rB ← M1[PC + 1] <br/> valC ← M8[PC + 2] <br/> valP ← PC + 10|
| 译码 | valB ← R[rB] |
| 执行 | valE ← valB + valC <br/> Set CC|
| 访存 | |
| 写回 | R[rB] ← valE |
| 更新 PC | PC ← valP |

其中 `icode = 0xc`, `ifun = 0`, `rA = 0xf` 。

#### 修改控制逻辑

有了每个阶段需要处理的事情之后，我们就需要修改控制逻辑，使得原本的 `SEQ` 处理器能处理我们新增的 `iaddq` 指令。

我们观察 `addq` 和 `iaddq` 及电路的控制逻辑可以发现，有以下 8 处需要特殊处理：
1. 修改 `instr_valid` 判断逻辑，识别出 `iaddq` 指令合法

    ```hcl
    bool instr_valid = icode in 
        { INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ,
            IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ, IIADDQ }; 
    ```
2. 修改 `need_regids` 的判断逻辑，识别出 `iaddq` 指令需要从指令中读出寄存器 ID ：

    ```hcl
    # Does fetched instruction require a regid byte?
    bool need_regids =
        icode in { IRRMOVQ, IOPQ, IPUSHQ, IPOPQ, 
            IIRMOVQ, IRMMOVQ, IMRMOVQ, IIADDQ };
    ```
3. 修改 `need_valC` 的判断逻辑，识别出 `iaddq` 指令需要从指令中读出一个常数：

    ```hcl
    # Does fetched instruction require a constant word?
    bool need_valC =
        icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL, IIADDQ };
    ```
4. 修改 `srcB` 的选择逻辑，选择 `valB` 的源寄存器地址 `srcB` 为 `rB` ：

    ```hcl
    ## What register should be used as the B source?
    word srcB = [
        # iaddq 指令选择 rB 为 srcB
        icode in { IOPQ, IRMMOVQ, IMRMOVQ, IIADDQ  } : rB;
        icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
        1 : RNONE;  # Don't need register
    ];
    ```
5. 修改 `dstE` 的选择逻辑，选择 `valE` 的目的寄存器地址 `dstE` 为 `rB` ：

    ```hcl
    ## What register should be used as the E destination?
    word dstE = [
        icode in { IRRMOVQ } && Cnd : rB;
        # iaddq 指令选择 rB 为 dstE
        icode in { IIRMOVQ, IOPQ, IIADDQ} : rB;
        icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
        1 : RNONE;  # Don't write any register
    ];
    ```
6. 修改 `aluA` 的选择逻辑，选择传给 ALU 的 `aluA` 为指令的立即数 `valC` ：

    ```hcl
    ## Select input A to ALU
    word aluA = [
        # 最开始先特殊处理 iaddq 的指令，让其选择 valC
        icode == IIADDQ: valC;
        icode in { IRRMOVQ, IOPQ } : valA;
        icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ } : valC;
        icode in { ICALL, IPUSHQ } : -8;
        icode in { IRET, IPOPQ } : 8;
        # Other instructions don't need ALU
    ];
    ```
7. 修改 `aluB` 的选择逻辑，选择传给 ALU 的 `aluB` 为寄存器 `rB` 的值 `valB`

    ```hcl
    ## Select input B to ALU
    word aluB = [
        icode in { IRMMOVQ, IMRMOVQ, IOPQ, ICALL, 
                  IPUSHQ, IRET, IPOPQ, IIADDQ } : valB;
        icode in { IRRMOVQ, IIRMOVQ } : 0;
        # Other instructions don't need ALU
    ];
    ```
8. 修改 `set_cc` 逻辑，识别出 `iaddq` 指令同样需要设置条件码寄存器：

    ```hcl
    ## Should the condition codes be updated?
    # iaddq 指令也需要更新条件码寄存器
    bool set_cc = icode in { IOPQ, IIADDQ };
    ```

修改完毕后，我们首先使用一个简单的程序自测一下，如果有问题就需要根据错误提示继续修改，没有问题就可以执行完整的测试了：

```shell script
> make VERSION=full && ./ssim -t ../y86-code/asumi.yo
# Building the seq-full.hcl version of SEQ
../misc/hcl2c -n seq-full.hcl <seq-full.hcl >seq-full.c
gcc -Wall -O2  -I../misc  -o ssim \
	seq-full.c ssim.c ../misc/isa.c  -lm
Y86-64 Processor: seq-full.hcl
137 bytes of code read
...
32 instructions executed
Status = HLT
Condition Codes: Z=1 S=0 O=0
Changed Register State:
%rax:	0x0000000000000000	0x0000abcdabcdabcd
%rsp:	0x0000000000000000	0x0000000000000100
%rdi:	0x0000000000000000	0x0000000000000038
%r10:	0x0000000000000000	0x0000a000a000a000
Changed Memory State:
0x00f0:	0x0000000000000000	0x0000000000000055
0x00f8:	0x0000000000000000	0x0000000000000013
ISA Check Succeeds
```

可以发现我们的修改逻辑能通过一个简单的测试，现在需要运行完整的测试逻辑了：

```shell script
# 测试结果正确性
> cd ../y86-code; make testssim
...
asum.seq:ISA Check Succeeds
asumr.seq:ISA Check Succeeds
cjr.seq:ISA Check Succeeds
j-cc.seq:ISA Check Succeeds
poptest.seq:ISA Check Succeeds
prog1.seq:ISA Check Succeeds
prog2.seq:ISA Check Succeeds
prog3.seq:ISA Check Succeeds
prog4.seq:ISA Check Succeeds
prog5.seq:ISA Check Succeeds
prog6.seq:ISA Check Succeeds
prog7.seq:ISA Check Succeeds
prog8.seq:ISA Check Succeeds
pushquestion.seq:ISA Check Succeeds
pushtest.seq:ISA Check Succeeds
ret-hazard.seq:ISA Check Succeeds
...

# 测试 iaddq 执行前后的所有状态的正确性
> cd ../ptest; make SIM=../seq/ssim TFLAGS=-i
./optest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 58 ISA Checks Succeed
./jtest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 96 ISA Checks Succeed
./ctest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 22 ISA Checks Succeed
./htest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 756 ISA Checks Succeed
```

通过所有测试用例，表明我们的修改正确有效。这一部分涵盖了设计一条指令的完整流程，先分析一条新指令的各个阶段的操作，然后再分析出每个操作设计的控制逻辑，再对相应的控制逻辑进行修改，最后使得原有的指令集能支持一条全新的指令。如果仅仅读书自学，还是很容易懵；结合实践就感觉很简单，也加深了对各种操作和控制逻辑的关联的理解。

### 第三部分

第三部分主要是改写 `sim/pipe/ncopy.ys`和 `sim/pipe/pipe-full.hcl` 文件，使得已有的 `PIPE` 处理器运行 `ncopy.ys` 时尽可能快，本部分所有操作均在 `sim/seq` 文件夹内进行。其中函数 `ncopy` 的功能是拷贝数组元素，并返回其中整数的个数。。

第三部分具体要求如下：

- 正确修改 `ncopy.ys` 和 `pipe-full.hcl` 文件，通过所有测试用例
- 假设程序可以在 C 个周期内拷贝 N 个元素，那么定义 `CPE = C / N` ，目标是使这个值尽可能小，初始程序 `CPE = 897/63 = 14.24` ，当 `CPE < 7.50` 时可取得满分（已知最小的 `CPE` 为 `7.48`），通常情况应该让 `CPE < 9.00`

#### 分析优化点

在知识点回顾部分中，已经了解过这里可以如何优化了：

- 需要将条件转移指令修改为条件传送指令，避免分支预测这个过程，从而避免分支预测错误导致取消指令
- 优化 `ncopy.ys` 代码，尽可能减少需要执行的指令
    - 循环内最后再执行 `subq %r8, %rdx` ，可以减少循环内的 1 条指令 `andq %rdx,%rdx`
    - 在循环外设置需要的常量，可以减少循环内的 2 条指令 `irmovq $1, %r10`, `irmovq $8, %r10`
    - 使用 `iaddq` 指令，可以减少循环外的 1 条常量设置的指令
    - 将 `mrmovq` 和 `rmmovq` 指令分开，可以减少因为加载/使用冒险而产生的一个周期暂停（也可以修改控制逻辑，使这种情况不需要暂停，控制逻辑的修改对应家庭作业 `4.57` (`P329`)）

#### 支持 `iaddq` 指令

首先就是需要实现 `iaddq` 指令的流水线控制逻辑，我们根据第二部分中的分析结果及流水线的控制逻辑可知需要修改以下 8 处相应的控制逻辑（可对照书中 `P302` 图 4-52 进行修改）：

![图 4-52 流水线化的最终实现—— PIPE 的硬件结构](archlab/图%204-52%20流水线化的最终实现——%20PIPE%20的硬件结构.png)

1. 修改 `instr_valid` 判断逻辑，识别出 `iaddq` 指令合法

    ```hcl
    # Is instruction valid?
    bool instr_valid = f_icode in 
        { INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ,
          IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ, IIADDQ };
    ```
2. 修改 `need_regids` 的判断逻辑，识别出 `iaddq` 指令需要从指令中读出寄存器 ID ：

    ```hcl
    # Does fetched instruction require a regid byte?
    bool need_regids =
        f_icode in { IRRMOVQ, IOPQ, IPUSHQ, IPOPQ, 
                 IIRMOVQ, IRMMOVQ, IMRMOVQ, IIADDQ };
    ```
3. 修改 `need_valC` 的判断逻辑，识别出 `iaddq` 指令需要从指令中读出一个常数：

    ```hcl
    # Does fetched instruction require a constant word?
    bool need_valC =
        f_icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL, IIADDQ };
    ```
4. 修改 `d_srcB` 的选择逻辑，选择 `d_rvalB` 的源寄存器地址 `d_srcB` 为 `D_rB` ：

    ```hcl
    ## What register should be used as the B source?
    word d_srcB = [
        # iaddq 指令需要读寄存器 rB
        D_icode in { IOPQ, IRMMOVQ, IMRMOVQ, IIADDQ } : D_rB;
        D_icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
        1 : RNONE;  # Don't need register
    ];
    ```
5. 修改 `d_dstE` 的选择逻辑，选择 `d_valE` 的目的寄存器地址 `d_dstE` 为 `D_rB` ：

    ```hcl
    ## What register should be used as the E destination?
    word d_dstE = [
        # iaddq 指令需要写寄存器 rB
        D_icode in { IRRMOVQ, IIRMOVQ, IOPQ, IIADDQ } : D_rB;
        D_icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
        1 : RNONE;  # Don't write any register
    ];
    ```
6. 修改 `aluA` 的选择逻辑，选择传给 ALU 的 `aluA` 为指令的立即数 `E_valC` ：

    ```hcl
    ## Select input A to ALU
    word aluA = [
        # iaddq 指令的操作数 A 来自指令中的立即数
        E_icode == IIADDQ : E_valC;
        E_icode in { IRRMOVQ, IOPQ } : E_valA;
        E_icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ } : E_valC;
        E_icode in { ICALL, IPUSHQ } : -8;
        E_icode in { IRET, IPOPQ } : 8;
        # Other instructions don't need ALU
    ];
    ```
7. 修改 `aluB` 的选择逻辑，选择传给 ALU 的 `aluB` 为寄存器 `rB` 的值 `E_valB`

    ```hcl
    ## Select input B to ALU
    word aluB = [
        E_icode in { IRMMOVQ, IMRMOVQ, IOPQ, ICALL, 
                 IPUSHQ, IRET, IPOPQ, IIADDQ } : E_valB;
        E_icode in { IRRMOVQ, IIRMOVQ } : 0;
        # Other instructions don't need ALU
    ];
    ```
8. 修改 `set_cc` 逻辑，识别出 `iaddq` 指令同样需要设置条件码寄存器：

    ```hcl
    ## Should the condition codes be updated?
    bool set_cc = E_icode in { IOPQ, IIADDQ } &&
        # State changes only during normal operation
        !m_stat in { SADR, SINS, SHLT } && !W_stat in { SADR, SINS, SHLT };
    ```

修改完成后，我们需要先验证其是否正确：

```shell script
# 重新构建模拟器
> make psim VERSION=full

# 测试小用例
> ./psim -t sdriver.yo
Y86-64 Processor: pipe-full.hcl
349 bytes of code read
...
65 instructions executed
Status = HLT
...
ISA Check Succeeds
CPI: 61 cycles/52 instructions = 1.17
```

发现两个基本用例都通过了，表明我们的 `iaddq` 指令的控制逻辑没有问题，接下来就需要修改函数 `ncopy` 了，使其尽可能快。 

#### 优化函数 `ncopy`

根据前面分析的那样，函数 `ncopy` 有 4 处地方可以优化，按照相应的分析结果进行优化，可得一下代码：

```assemble
##################################################################
# Do not modify this portion
# Function prologue.
# %rdi = src, %rsi = dst, %rdx = len
ncopy:

##################################################################
# You can modify this portion
    # Loop header
    xorq %rax,%rax      # count = 0;
    irmovq $1, %r8
    andq %rdx,%rdx		# len > 0?
    jmp Test            # Goto Test:

Loop:
    mrmovq (%rdi), %r10 # read val from src...
    xorq %r9, %r9       # diff = 0
    # 将 rmmovq 与 mrmovq 分开，减少因为加载/使用冒险而产生的一个周期暂停
    # （也可以修改控制逻辑，使这种情况不需要暂停，控制逻辑的修改对应家庭作业 `4.57` (`P329`)）
    rmmovq %r10, (%rsi) # ...and store it to dst
    andq %r10, %r10     # val > 0?
    cmovg %r8, %r9      # if so, diff = 1
    addq %r9, %rax		# count += diff
    iaddq $8, %rdi		# src++
    iaddq $8, %rsi		# dst++
    iaddq $-1, %rdx		# len--
Test:
    jg Loop             # if so, goto Loop:
##################################################################
# Do not modify the following section of code
# Function epilogue.
Done:
	ret
##################################################################
```

然后运行测试程序：

```shell script
# 验证正确性
> ./correctness.pl -p
...
68/68 pass correctness test

# 验证性能
> ./benchmark.pl
...
Average CPE	11.26
Score	0.0/60.0
```

一顿操作猛如虎，一看得分 0.0 ，有点震惊🤯，这怎么能忍，还得继续分析优化。

#### 尝试再次优化函数 `ncopy`

我们回看刚刚优化的程序：其中循环部分有 5 条指令必不可少（`mrmovq (%rdi), %r10`, `rmmovq %r10, (%rsi)`, `iaddq $8, %rdi`, `iaddq $8, %rsi`, `iaddq $-1, %rdx`），而剩余的 4 条指令就是为了使用条件传送给 `%rax` 加上 `0/1` ，这一个简单操作占用了 4 条指令太不应该了，那么就需要把这块优化掉。

先看看给 `%rax` 加上 `0/1` 为什么要使用 4 条指令：
- 先将一个寄存器 `%r9` 设置为 `0`
- 再对寄存器 `%r10` 执行 `andq` 用于设置条件码寄存器
- 然后执行条件传送，当寄存器 `%r10` 内的数大于 `0` 时，将寄存器 `%r9` 设置为 `0`
- 最后再将寄存器 `%r9` 的值加到寄存器 `%rax` 中

可以发现最后一条对寄存器 `%rax` 进行加法操作的指令必不可少，那我们就关注前三条指令是不是可以优化：
- 假如我们有一条指令 `ciaddg V, rB` 可以执行条件立即数加法，那么寄存器 `%r9` 和条件传送指令就不需要，可以优化掉两条指令
    - 如果使用 `ciaddg $1, %rax` ，那么前面必定会再次出现加载/使用冒险，我们可以将 `iaddq $8, %rdi` 提前避免暂停一个周期（如前面提到的，同样还可以修改控制逻辑在这种情况下不会暂停）
- 假如指令 `rmmovq`/`mrmovq` 可以设置条件码寄存器，那么 `andq` 指令也不需要了。但是观察硬件逻辑可以发现：条件码寄存器的值是在执行阶段运行完 `ALU` 的逻辑后才会设置的，但这两条指令的执行阶段计算与内部的值毫无关系，所以这个假设无法实现
    - 使用寄存器 `%r10` 的指令除了这两个再也没有其他了，所以 `andq` 指令无法优化掉

想法很美好，但是 `Y86-64` 指令集不支持添加其他指令，如果要强行增加指令，必须修改模拟器底层相关的部分，那么只能在本地通过骗骗自己，所以还是需要其他方法，搜索了一下得知需要第五章——优化程序性能的知识，所以先暂停继续学习。

## 总结

第一部分和第二部分只与第四章的知识有关，编写汇编程序和增加 `iaddq` 指令都很简单，轻易就能获得满分。

第三部分开始也以为很简单，仍旧以为只与第四章的知识相关，所以想法都是通过优化指令的形式去优化代码。虽然仅靠第四章的知识只能获得正确性的 `40` 分，但在优化的过程中基本想到了第四章中影响性能的各个方面（加载/使用冒险会暂停一个周期、分支预测错误会取消错误的指令浪费两个周期），表明知识吸收得不错。

第四章结合本 Lab 整体下来感觉上学习曲线陡峭，自学过程中发现太多没 ~~yi~~ 接 ~~wang~~ 触 ~~ji~~ 的知识，导致经常性发懵，但第二遍再看时很多都豁然开朗，最后再做 Lab 时也能够得心应手。
