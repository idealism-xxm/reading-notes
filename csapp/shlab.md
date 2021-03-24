## 简介

Shell Lab 属于《深入理解计算机》第八章——异常控制流。异常控制流发生在计算机系统的各个层次，是计算机系统中提供并发的基本机制。

## 知识点回顾

异常控制流是操作系统用来实现 I/O 、进程、虚拟内存和并发的基本机制。 `P501`

### 异常 `P502`

异常是异常控制流的一种形式，它一部分由硬件实现，一部分由操作系统实现。异常就是控制流中的突变，用来相应处理器状态中的某些变化。 `P502`

![图 8-1 异常的剖析](img/图%208-1%20异常的剖析.png)

当异常处理程序完成处理后，根据引起异常的事件类型，会发生以下 3 种情况中的一种： `P502`

- 处理程序将控制返回给当前指令 `I_curr` ，即当事件发生时正在执行的指令
- 处理程序将控制返回给 `I_next` ，如果没有发生异常将会执行下一条指令
- 处理程序终止被中断的程序

异常可以分为四类，异步异常是由处理器外部的 I/O 设备中的事件产生的，同步异常是执行一条指令的直接产物。 `P504`

| 类别 | 原因 | 同步/异步 | 返回行为 |
| --- | --- | --- | --- |
| 中断 | 来自 I/O 设备的信号 | 异步 | 总是返回到下一条指令 |
| 陷阱 | 有意的异常 | 同步 | 总是返回到下一条指令 |
| 故障 | 潜在可恢复的错误 | 同步 | 可能返回到当前指令 |
| 终止 | 不可恢复的错误 | 同步 | 不会返回 |

![图 8-5 ~ 图 8-8 四种异常处理流程](img/图%208-5%20~%20图%208-8%20四种异常处理流程.png)

### 进程 `P508`

一提到进程就会想到课本里那句经典的话：进程是资源分配的最小单位，线程是 CPU 调度的最小单位。

进程提供给应用程序两个重要的抽象： `P508`

- 一个独立的逻辑控制流：它提供给每个程序一个假象，好像应用程序在独占地使用处理器
- 一个私有的地址空间：它提供给每个程序一个假象，好像应用程序在独占地使用内存系统

进程通常运行在用户模式中，它不能执行特权指令，也不能直接引用地址空间中的内核区内的代码和数据；为了执行这些操作，它必须进入内核模式，而进入内核模式的唯一方法是通过诸如中断、故障或陷入系统调用这样的异常。 `P510`

操作系统内核为每个进程维持一个上下文。上下文就是内核重新启动一个被抢占进程所需的状态。 `P511`

操作系统内核使用上下文切换这种较高层形式的异常控制流来实现多任务。上下文切换建立在前面提到的较低层异常机制之上，它可以将控制转移到新的进程： `P511`

- 保存当前进程的上下文
- 恢复某个先前被抢占的进程被保存的上下文
- 将控制传递给这个新恢复的进程

![图 8-14 进程上下文切换](img/图%208-14%20进程上下文切换.png)

### 进程控制 `P513`

Unix 提供了大量从 C 程序中操作进程的系统调用，书中本节详细介绍了一些重要的函数，相当于部分 API 文档，不需要仔细研究每个参数，只需要了解函数可以干什么即可，在实际使用时再具体钻研。

| 函数 | 作用 |
| --- | --- |
| `getpid` | 获取当前进程的 PID |
| `getppid` | 获取当前进程父进程的 PID |
| `exit` | 以 `status` 退出状态来终止进程 |
| `fork` | 创建一个新的运行的子进程 |
| `waitpid` | 等待子进程终止或停止 |
| `wait` | `waitpid` 的简单版本， `wait(&status)` 等价于 `waitpid(-1, &status, 0)` |
| `sleep` | 当前进程挂起一段指定的时间 |
| `pause` | 当前进程休眠，直到收到一个信号 |
| `execve` | 加载并运行可执行目标文件 |
| `getenv` | 获取一个环境变量 |
| `setenv` | 设置一个环境变量 |
| `unsetenv` | 删除一个环境变量 |

其中 `fork` 和 `execve` 两个函数尤为重要，两者配合起来就可以运行一个程序。 `P524`

`fork` 新创建的子进程几乎但不完全与父进程相同（最大区别就是有不同的 PID）。 `fork` 函数有以下特点： `P514`

- 调用一次，返回两次。一次返回到父进程，返回值为子进程 PID ；一次返回到子进程，返回值为 0
- 并发执行。父进程和子进程是并发运行的独立进程
- 相同但是独立的地址空间：子进程得到与父进程用户虚拟地址空间相同但独立的一份副本，包括代码和数据段、堆、共享库以及用户栈
- 共享文件：子进程获得与父进程任何打开文件描述符相同的副本，这意味着子进程可以读写父进程在调用 `fork` 前打开的任何文件

![图 8-17 嵌套 fork 的进程图](img/图%208-17%20嵌套%20fork%20的进程图.png)

`execve(filename, argv, envp)` 加载并运行可执行目标文件 `filename` ，且带有参数列表 `argv` 和环境变量列表 `envp` 。 `execve` 函数有以下特点： `P521`

- 调用一次，从不返回。调用成功则不返回，调用失败才会返回到调用进程，返回值为 -1
- 控制传递给新程序的主函数

从程序员的角度，我们可以认为进程总是处于下面三种状态之一：

- 运行：进程要么在 CPU 上执行，要么在等待被执行且最终会被内核调度
- 停止：进程的执行被挂起，且不会被调度。当收到 `SIGSTOP`, `SIGTSTP`, `SIGTTIN`, `SIGTTOU` 信号时，进程就停止，并且保持停止直到它收到一个 `SIGCONT` 信号，在这个时刻，进程再次开始运行
- 终止：进程永远地停止了。进程会因为三种原因终止：
    - 收到一个信号，该信号的默认行为是终止进程
    - 从主程序中返回
    - 调用 `exit` 函数

### 信号 `P526`

Linux 信号允许进程和内核中断其他进程，它通知进程系统中发生了一个某种类型的事件。 `P526`

传送一个信号到目的程序是由两个不同步骤组成的： `P527`

- 发送信号。内核通过更新目的进程上下文中的某个状态，发送一个信号给目的进程。一个进程可以发送信号给自己。
- 接受信号。当目的进程被内核强迫以某种方式对信号的发送作出反应时，它就接受了信号。进程可以忽略这个信号，终止或者通过执行一个称为信号处理程序的用户层函数捕获这个信号。

![图 8-27 信号处理](img/图%208-27%20信号处理.png)

一个发出而没有被接收的信号叫做待处理信号。 **在任何时刻，一种类型至多会有一个待处理信号，后来的相同待处理信号会被简单地丢弃。** 一个进程可以有选择性地阻塞接收某种信号，此时该信号仍然可以被发送，但是不会被接受，直至进程取消对该信号的阻塞。 `P528`

书中本节同样详细介绍了一些重要的函数，相当于部分 API 文档，不需要仔细研究每个参数，只需要了解函数可以干什么即可，在实际使用时再具体钻研。

| 函数 | 作用 |
| --- | --- |
| `getpgrp` | 获取当前进程的进程组 ID （默认一个子进程和它的父进程同属于一个进程组） |
| `setpgid` | 设置一个进程的进程组 ID |
| `kill` | 向一个进程发送 `SIGKILL` 信号 |
| `alarm` | 向一个进程发送 `SIGALRM` 信号 |
| `signal` | 修改某个信号的处理程序（`SIGSTOP` 和 `SIGKILL` 的默认行为不能修改） |
| `sigprocmask` | 改变当前阻塞的信号集合 |
| `sigemptyset` | 初始化一个信号集合为空集合 |
| `sigfillset` | 把每个信号都添加到一个信号集合中 |
| `sigaddset` | 把一个信号添加到一个信号集合中 |
| `sigdelset` | 把一个信号从一个信号集合中删除 |
| `sigismember` | 判断一个信号是否在一个信号集合中 |
| `sigaction` | 设置信号处理函数时，明确指定想要的信号处理语义，不常使用 |
| `sigsuspend` | 用一个信号集合替换当前的阻塞集合，然后挂起当前进程，直到收到一个信号 |

信号处理程序可以被其他信号处理程序中断。 `P531`

信号处理程序的属性： `P533`

- 处理程序与主程序并发运行，共享同样的全局变量，因此可能与主程序和其他处理程序相互干扰
- 如何以及何时接收信号的规则常常有违直觉
- 不同的系统有不同的信号处理语义

编写安全、正确和可移植的信号处理程序的基本规则： `P533`

- 安全地处理信号
    - 处理程序要尽可能简单
    - 在处理程序中只调用一部信号安全的函数（函数要么可重入，要么不能被信号处理程序中断）
    - 保存和恢复 `errno`
    - **阻塞所有的信号，保护对共享全局数据结构的访问**
    - 用 `volatile` 声明全局变量
    - 用 `sig_atomic_t` 声明标志
- 正确地处理信号：由于每种类型最多只能有一个未处理的信号，所以信号可能会被丢弃，不能用信号来对其他进程中发生的事件计数
- 可移植地处理信号：使用 `sigaction` 来明确信号处理语义

### 非本地跳转 `P546`

非本地跳转通过 `setjmp` 和 `longjmp` 函数来实现。它将控制直接从一个函数转移到另一个当前正在执行的函数，而不需要经过正常的调用-返回序列。 `P547`

看到非本地跳转的用法，很快就能想到 C++ 和 Java 中的异常机制。后面书中也提到 C++ 和 Java 中的异常机制是较高层次的，是非本地跳转的更加结构化版本。

`setjmp` 只被调用一次，但返回多次：一次是当第一个次调用 `setjmp` ，而调用环境保存在缓冲区 `env` 中时；一次是为每个相应的 `longjmp` 调用。 `P547`

`longjmp` 被调用一次，但从不返回。 `P547`

## 准备

可以在 [官网](http://csapp.cs.cmu.edu/3e/labs.html) 下载 Shell Lab 相关的程序。

开始前需要阅读 [Shell Lab writeup](http://csapp.cs.cmu.edu/3e/shlab.pdf) ，可以知道本次 Lab 已经帮我们搭建好了一个 shell 程序的框架，只需要我们完成以下几个必备的功能即可：

- `eval`: 解析并执行命令行，大约 70 行
- `bulitin_cmd`: 识别并执行内置命令 (`quit`, `fg`, `bg`, `jobs`)，大约 25 行
- `do_bgfg`: 模拟内置命令 `fg` 和 `bg` ，大约 50 行
- `waitfg`: 等待一个前台作业完成，大约 20 行
- `sigchld_handler`: 响应 `SIGCHLD` 信号，大约 80 行
- `sigint_handler`: 响应 `SIGINT` (`ctrl-c`) 信号，大约 15 行
- `sigtstp_handler`: 响应 `SIGTSTP` (`ctrl-z`) 信号，大约 15 行

本次需要使用的程序依旧需要在 Docker 中运行，将本地 Lab 的目录挂载进容器中即可：

```shell script
docker run -ti -v {PWD}:/csapp ubuntu:18.04
```

进入容器后需要安装一些必须软件以便后续能成功运行：

```shell script
apt-get update && apt-get -y install gcc make libgetopt-complete-perl
```

然后就可以愉快地开始闯关了。

## 闯关

闯关开始前，再仔细看看我们需要实现的 `tsh` 所需具备的特性：

- 提示符为 `tsh> ` ，这个自带框架已经具备了，无需修改
- 命令行由一个命令 `name` 和任意数量的参数组成，它们之间通过一个或多个空格分隔。如果 `name` 是一个内置命令，则立刻执行，然后等待下一条命令行；其他情况 `name` 被认为是一个可执行文件的路径，则在子进程中运行这个可执行文件
- 无需支持管道 (`|`) 和 I/O 重定向 (`<`, `>`)
- `ctrl-c` (`ctrl-z`) 会引起一个 `SIGINT` (`SIGTSTP`) 信号，传给当前的前台作业和它所有的子进程；如果没有前台作业，则无事发生
- 如果命令行以 `&` 结尾，则作业会运行在后台，否则作业会运行在前台
- 每个作业都有唯一的进程 ID (PID) 和作业 ID (JID) 。在命令行中 `<job>` 可通过以下方式引用：`5` 表示 PID ，`%5` 表示 JID
- 支持以下内置命令
    - `quit`: 终止 shell
    - `jobs`: 列出所有后台作业
    - `bg <job>`: 通过发送 `SIGCONT`信号重启作业 `<job>` 并运行在后台中
    - `fg <job>`: 通过发送 `SIGCONT`信号重启作业 `<job>` 并运行在前台中
- 处理所有的僵尸进程。如果一个作业由于一个未响应的信号终止，那么 `tsh` 应该识别出这个事件，并打印该作业的 PID 和该信号的描述

### 解析命令并执行

目前只有一个 shell 框架，编译后运行，输入任何命令都无作用，所以我们首先就是要实现 `eval` 函数，确保能解析命令并执行。

这一部分在书中 `P525` 给出过示例，我们可以参考并完善。框架已经帮我们抽离了 `eval` 所需要执行的三个子函数，并已经实现了 `parseline` 去解析命令行，让我们专心于 shell 本身的逻辑。

我们先完成剩余的两个子函数：

#### `builtin_cmd`

识别并执行内置命令，如果是内置命令则执行并返回 1 ，不是内置命令则返回 0 。其中内置命令对应的函数 `listjobs` 和 `do_bgfg` 还未实现，等我们进一步完善了 `jobs` 相关的部分再去实现。

```c
int builtin_cmd(char **argv) {
    // 识别到 quiz 内置命令，直接终止 shell
    if (!strcmp(argv[0], "quit")) {
        exit(0);
    }
    // 识别到 jobs 内置命令，执行 listjobs 后返回 1
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    // 识别到 bg <job> 或者 fg <job> 内置命令，执行 do_bgfg 后返回 1
    if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }

    return 0;     /* not a builtin command */
}
```

#### `waitfg`

阻塞当前进程，并等待指定的子进程完成，用于等待前台作业完成。

书中 `P525` 的示例给出了如下的等待前台作业的代码：

```c
void waitfg(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        unix_error("waitfg: waitpid error");
    }
}
```

不过我们在信号处理程序 `sigchld_handler` 中也会调用 `waitpid` ，所以官方提示建议我们使用轮询的方式等待前台作业完成，仅在 `sigchld_handler` 中调用 `waitpid` 。

```c
void waitfg(pid_t pid) {
    // 当作业列表中的前台作业是 pid ，则先睡 1s
    while(fgpid(jobs) == pid) {
        sleep(0);
        // 书中 P545 提到使用 sigsuspend 是一个更合适的解决方法
    }
}
```

#### `eval`

现在我们可以完成 `eval` 函数了，实现解析并执行命令行的功能，并支持前台作业等待子进程完成。

```c
void eval(char *cmdline) {
    // 存储本次命令行的命令和参数
    char *argv[MAXARGS];

    // 判断是否是后台作业
    // 框架已经完成了 parseline ，我们直接调用解析即可
    int is_bg = parseline(cmdline, argv);
    // 忽略空行
    if (argv[0] == NULL) {
        return;
    }

    // 执行内置命令，如果是内置命令，则直接返回
    if (builtin_cmd(argv)) {
        return;
    }

    // 子进程 PID
    pid_t pid;
    // 执行可执行文件
    if ((pid = fork()) == 0) {
        // 子进程使用 execve 运行可执行文件
        if (execve(argv[0], argv, environ) < 0) {
            printf("tsh: command not found: %s\n", argv[0]);
            exit(0);
        }
        // execve 只有失败才会返回，所以成功了不会执行后面的语句，
        // 会执行可执行文件的主函数
    }
    // 这里开始必定是父进程才会执行

    if (is_bg) {
        // 如果是后台作业，则输出提示信息
        printf("[%d] (%d) %s", 0, pid, cmdline);
    } else {
        // 如果是前台作业，则阻塞至子进程结束
        waitfg(pid);
    }
}
```

至此，我们基本完成了 `eval` 的全部功能，但是还缺少 `jobs` 相关的功能，所以我们现在不能执行其余三个内置命令，也无法打印后台作业的 JID 。

现在可以先运行测试用例 `make test01`, `make test02`, `make test03` ，发现输出都与标准程序输出一致，证明我们代码正确无误。而 `make test04` 需要我们正确输出后台作业的 JID ，这部分还未实现，所以接下来实现后台作业相关部分。

### 正确处理后台作业

后台作业列表相关的子函数已经被实现了，现在需要我们在适当的位置正确调用即可。书中 8.5.6 节 (`P540`) 已经讲过添加作业和删除作业分别在主程序和信号处理程序中，且都会操作全局变量 `jobs` ，虽然它们不是并发的，但由于中断可能打破非原子操作，那么还是有可能存在并发问题，所以我们需要在进行这两个操作时阻塞所有信号防止被打断。

#### 添加作业

我们需要在 `eval` 中再加上添加作业的相关代码，这里为了防止出现并发问题，就需要在 `fork` 前阻塞 `SIGCHLD` 信号，防止在 `addjob` 调用前， `SIGCHLD` 的信号处理程序先调用 `deletejob` 。

同时闯关开始时提到需要具备以下特性：

> `ctrl-c` (`ctrl-z`) 会引起一个 `SIGINT` (`SIGTSTP`) 信号，传给当前的前台作业和它所有的子进程；如果没有前台作业，则无事发生

所以我们要将给子进程重新开一个进程组，方便后面直接通过调用 `kill(-pid, sig)` 将信号传递给前台作业 `pid` 及其所有的子进程。

```c
void eval(char *cmdline) {
    // 存储本次命令行的命令和参数
    char *argv[MAXARGS];

    // 判断是否是后台作业
    // 框架已经完成了 parseline ，我们直接调用解析即可
    int is_bg = parseline(cmdline, argv);
    // 忽略空行
    if (argv[0] == NULL) {
        return;
    }

    // 执行内置命令，如果是内置命令，则直接返回
    if (builtin_cmd(argv)) {
        return;
    }

    // 先阻塞 SIGCHLD 信号
    sigset_t mask_one, prev_one;
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);
    Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

    // 子进程 PID
    pid_t pid;
    // 执行可执行文件
    if ((pid = Fork()) == 0) {
        // 子进程继承了父进程的被阻塞集合，先解除阻塞
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
        // 子进程重新开一个进程组，方便后续将 `ctrl-c` (`ctrl-z`)
        // 引起的 `SIGINT` (`SIGTSTP`) 信号传递给前台作业及其所有子进程
        if (setpgid(0, 0) < 0) {
           printf("setpgid error\n");
           exit(0);
        }
        // 子进程使用 execve 运行可执行文件
        if (execve(argv[0], argv, environ) < 0) {
            printf("tsh: command not found: %s\n", argv[0]);
            exit(0);
        }
        // execve 只有失败才会返回，所以成功了不会执行后面的语句，
        // 会执行可执行文件的主函数
    }
    // 这里开始必定是父进程才会执行

    // 子进程添加至作业列表中
    int state = is_bg ? BG : FG;
    addjob(jobs, pid, state, cmdline);
    // 恢复原本的被阻塞集合
    Sigprocmask(SIG_SETMASK, &prev_one, NULL);
    if (is_bg) {
        // 后台作业需要输出提示信息
        int jid = pid2jid(pid);
        printf("[%d] (%d) %s", jid, pid, cmdline);
    } else {
        // 前台作业需要阻塞至子进程结束
        waitfg(pid);
    }
}
```

#### `sigchld_handler`

删除作业部分就需要在 `sigchld_handler` 信号处理程序中进行，我们可以同时完成 `sigchld_handler` 相关的所有操作。

我们再看看闯关开始时提到的需要具备的特性：

> - 处理所有的僵尸进程。如果一个作业由于一个未响应的信号终止，那么 `tsh` 应该识别出这个事件，并打印该作业的 PID 和该信号的描述
> - 停止的子进程被 `bg <job>` 或者 `fg <job>` 发送的 `SIGCONT` 信号重启

书中 8.4.3 节 (`P516`) 提到使用 `waitpid` 会等待子进程终止或者停止，并且会通过 `status` 传递终止或者停止的子进程的状态信息，可通过 `wait.h` 中定义的宏进行判断。

导致 `waitpid` 返回的原因有三种，我们列出对应的处理方式：

- 子进程调用 `exit` 或者调用 `return` 正常终止：直接从作业列表中删除
- 子进程因为一个未被捕获的信号终止：先打印相关信息，再从作业列表中删除
- 子进程被停止（要检测到这个状态，需要在调用时加上 `WUNTRACED` 选项）：先打印相关信息，再更改状态为停止 `ST`

```c
void sigchld_handler(int sig) {
    // 保留原本的错误码，结束调用时恢复
    int olderrno = errno;
    int status;
    pid_t pid;
    // 由于没有其他信号处理程序会使用 jobs ，所以无需阻塞信号

    // 处理所有终止或者停止的子进程，如果没有立即返回
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        // 正常退出，直接从作业列表删除
        if (WIFEXITED(status)) {
            deletejob(jobs, pid);
        }
        // 由于未捕获的信号终止，先打印相关信息，再从作业列表删除
        if (WIFSIGNALED(status)) {
            int jid = pid2jid(pid);
            int signal = WTERMSIG(status);
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, signal);
            deletejob(jobs, pid);
        }
        // 由于信号被停止，先打印相关信息，再将作业状态更改为 ST
        if (WIFSTOPPED(status)) {
            struct job_t *job = getjobpid(jobs, pid);
            int signal = WSTOPSIG(status);
            printf("Job [%d] (%d) stopped by signal %d\n", job -> jid, pid, signal);
            job -> state = ST;
        }
    }
    // 使用 WNOHANG 选项时，如果还有子进程存在，那么就会返回 0
    // 这种情况不算错误
    if (pid != 0 && errno != ECHILD) {
        unix_error("waitpid error");
    }

    // 恢复错误码
    errno = olderrno;
}
```

现在可以继续运行测试用例 `make test04`, `make test05` ，发现输出都与标准程序输出一致，证明我们代码正确无误。而 `make test06` 没有输出子进程被 `SIGINT` 终止的的信息，说明 `SIGINT` 信号没有正常处理，所以接下来需要实现其余信号处理程序了。

### 实现信号处理程序

#### `sigint_handler`

当 shell 收到 `SIGINT` 信号时，对当前的前台作业及其所有子进程发送 `SIGINT` 信号，即对前台作业所在进程组发送 `SIGINT` 信号。

```c
void sigint_handler(int sig) {
    // 获取前台作业的 PID
    pid_t pid = fgpid(jobs);
    // 对前台作业所在进程组发送 SIGINT 信号
    if (pid) {
        kill(-pid, sig);
    }
}
```

#### `sigtstp_handler`

当 shell 收到 `SIGTSTP` 信号时，对当前的前台作业及其所有子进程发送 `SIGTSTP` 信号，即对前台作业所在进程组发送 `SIGTSTP` 信号。

```c
void sigtstp_handler(int sig) {
    // 获取前台作业的 PID
    pid_t pid = fgpid(jobs);
    // 对前台作业所在进程组发送 SIGTSTP 信号
    if (pid) {
        kill(-pid, sig);
    }
}
```

现在可以继续运行测试用例 `make test06`, `make test07`, `make test08` ，发现输出都与标准程序输出一致，证明我们代码正确无误。而 `make test09` 没有输出 JID 为 2 的子进程被唤醒到后台作业的信号，所以接下来需要实现 `do_bgfg` 函数实现。

### 实现 `do_bgfg`

这个函数实现起来很简单：找到对应的作业，然后按照命令更改状态，再向其所在的进程组发送 `SIGCONT` 信号，最后再输出后台作业的信息或者等待前台作业完成即可。

简单写完后，可以直接正确运行到第 13 个测试用例 ， `make test14` 时出现了不一致，检查一下标准输出后，发现我们需要在边界情况输出对应的信息，对照着进行修改后即可完美通过全部 16 个测试用例。

```c
void do_bgfg(char **argv) {
    // 校验参数
    if (argv[1] == NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    struct job_t *job;
    int id;
    if (sscanf(argv[1], "%%%d", &id) > 0) {
        // 获取 jid 成功，然后获取对应的作业
         job = getjobjid(jobs, id);
         if (job == NULL) {
            printf("%%%d: No such job\n", id);
            return;
         }
    } else if (sscanf(argv[1], "%d", &id) > 0) {
        // 获取 pid 成功，然后获取对应的作业
        job = getjobjid(jobs, id);
        if (job == NULL) {
            printf("(%d): No such process\n", id);
            return;
        }
    } else {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    // 不存在，则直接返回
    if (job == NULL) {
        return;
    }

    // 修改状态，然后给子进程所在的进程组发送 SIGCONT 信号唤醒
    int is_bg = 0;
    if (!strcmp(argv[0], "bg")) {
        is_bg = 1;
    }
    job -> state = is_bg ? BG : FG;
    kill(-(job -> pid), SIGCONT);

    // 标记是否唤醒到后台作业
    if (is_bg) {
        // 后台作业则需要输出相关信息
        printf("[%d] (%d) %s", job -> jid, job -> pid, job -> cmdline);
    } else {
        // 前台作业则需要等到其运行完成
        waitfg(job -> pid);
    }
}
```

## 总结

基于 Lab 自带的框架，我们添加了不到 200 行代码就支持了 shell 的核心功能，并成功通过了全部 16 个测试用例。由于框架已经帮我们把功能拆分得很细，所以整体上不算太难，只要我们理解 shell 工作的流程，并知道所需功能对应的接口，就能写出基本可以运行的代码，主要还是跟着测试用例排查处理遗漏的各种情况。

最后顺便搜索了一下管道、 I/O 重定向的原理，都是通过提供的简单接口实现的，完成功能依旧很简单，难点还是在于处理边界情况。