## 简介

这个 Lab 属于第二章 —— 信息的表示和处理，主要讲解了计算机信息存储（大小端）和布尔代数，以及整数和浮点数的表示方法及相关推导，各种练习题和家庭作业也主要是加深对其理解，部分题目也非常考验思维（由于是家庭作业就没有太认真对待，还是眼高手低，觉得平常用不到，~~主要还是不想动脑子，看了答案开始思考原理，似乎从小就这样~~）。 Lab 的题目看下来也和家庭作业类似，但由于可以写代码并有现成的函数校验是否正确，心态立刻就不一样的，完全当作考试完成，没有参考其他答案。

## 准备

由于 `dlc` 程序似乎无法在 macOS 上运行，所以就直接在 Docker 中运行，并将本地 Lab 的目录挂载进容器中即可：

```shell script
docker run -ti -v {PWD}:/csapp ubuntu:18.04
```

进入容器后需要安装 `gcc` 和 `make` ：

```shell script
apt-get update && apt-get -y install gcc make
```

然后我们就可以愉快的完成第一个 Lab 了，似乎存在多种版本的 Lab ，只是题目不一样，我这里随便找了一个版本完成。

## datalab

### bitXor

- 描述：使用 `~` （按位取反） 和 `&` （按位与） 实现 `^` （异或）
- 合法运算符： `~` `&`
- 运算符最多个数： 14
- 分数： 1

看到这题就想起来这不是离散数学中学过么，可以使用任意两个逻辑运算的组合形式计算出其他逻辑运算的结果（虽然知道，但没有推导过）。

首先列出 `~` 和 `&` 的计算结果 (`P35`)：

| `~` | 0 | 1 | 
| --- | --- | --- |
| | 1 | 0 |

| `&` | 0 | 1 | 
| --- | --- | --- |
| 0 | 0 | 0 |
| 1 | 0 | 1 |

可以发现 `&` 的结果只有均为 1 时才为 1 ，而且它的结果与 `⊙` （同或）只有 1 处不同，而 `x ⊙ y = ~(x ^ y)` ，所以我们就想办法计算出 `⊙` 的结果即可。
 
`⊙` 是只要均为 0 或者均为 1 时才为 1 ，所以我们可以使用 `&` 和 `|` （按位或）得到它的结果，即： `x ⊙ y = (x & y) | (~x & ~y)` 。

那么 `x ^ y = ~((x & y) | (~x & ~y)) = ~(x & y) & ~(~x & ~y)` ，所以推广到多位很快就可以完成这道简单题了。

```c
int bitXor(int x, int y) {
  // x ^ y
  //    = ~(x ⊙ y)
  //    = ~((x & y) | (~x & ~y))
  //    = ~(x & y) & ~(~x & ~y)
  return ~(x & y) & ~(~x & ~y);
}
```

### tmin

- 描述：返回最小的 32 位补码表示的数
- 合法运算符： `!` `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 4
- 分数： 1

也是一道简单题，只要知道有符号数的补码表示方式 (`P45`) 即可，补码中最高位代表负权，其他位都是正权，所以最小的有符号数是最高位为 1 ，其他位为 0 ，32 位机器上就是 `0x80000000` ，由于我们不能使用超过 `0xFF` 的常量，所以需要使用左移来表示，即 `1 << 31` 。

```c
int tmin(void) {
  // tmin = 0x80000000
  return 1 << 31;  
}
```

### isTmax

- 描述：给定一个 32 位的补码表示的数，判断是否是最大的数
- 合法运算符： `!` `~` `&` `^` `|` `+`
- 运算符最多个数： 10
- 分数： 1

假设 x 为最大数，则其补码表示为： `0x7FFFFFFF` ，而 `~x` 为： `0x80000000` ，那么 `~x + ~x` 由于进位溢出结果就为 `0x00000000` ，此时取逻辑非即可判断返回其是否为最大数，即： `!(~x + ~x)`。

运行完测试后，发现少考虑了一种特殊情况，即 `x = -1` 时，其补码表示为 `0xFFFFFFFF` ，那么 `~x` 为 `0x00000000` ，此时不会溢出，但会将其误判为最大数，所以只要排除这种情况 (`!!(~x)`) 即可。

组合这两种情况可得表达式为： `!(~x + ~x) && !!(~x)` ，但由于我们不能使用 `&&` 和 `||`，所以需要先抽一个 `!` 出来： `!((~x + ~x) || !(~x))` ，并且 `||` 可以转换为 `|` （因为它们都只有数均为 0 时才为 0），所以最终表达式为： `!((~x + ~x) | !(~x))`

```c
int isTmax(int x) {
  // tmax -> (~x + ~x) = 0x80000000 + 0x80000000 = 0x00000000
  // -1 -> ~x = 0x00000000
  //    -> ~x + ~x = 0x00000000 + 0x00000000 = 0x00000000
  return !((~x + ~x) | !(~x));
}
```

### allOddBits

- 描述：给定一个 32 位的数，判断其二进制下所有奇数位上是否均为 1
- 合法运算符： `!` `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 12
- 分数： 2

家庭作业 `2.64` 的升级版，还是需要使用掩码 `0xAAAAAAAA` 判断，先通过 `0xAA` 移位获取到 `mask = 0xAAAAAAAA` ，然后再判断 `x & mask` 是否为 `mask` 即可。

判断一个数 `a` 是否为某一个确定的数 `b` 可以使用 `a ^ b` ，当结果为 0 时表示它们相同。

```c
int allOddBits(int x) {
  // a ^ b = 0 -> a = b
  int mask = 0xAA;
  mask |= mask << 8;
  mask |= mask << 16;
  return !((x & mask) ^ mask);  
}
```

### negate

- 描述：返回 -x
- 合法运算符： `!` `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 5
- 分数： 2

现在一看到 `-x` 就会想到特殊情况： `-tmin = tmin` ，看来已经深入大脑了。

不过该题与这个特殊情况无关，只需要使用 `-x = ~x + 1` 即可，我们以 4 位有符号数来推导，主要是把 `-x` 看成 `0 - x` ，并将 `0` 拆成 `-1 + 1` ，然后运用结合律，补码二进制表示时按加减法正常运算即可。

```c
int negate(int x) {
  //-x = 0 - x 
  //   = -1 + 1 - T2B(x)
  //   = 0b1111 + 1 - T2B(x)
  //   = 0b1111 - T2B(x) + 1
  //   = ~T2B(x) + 1
  //   = ~x + 1
  return ~x + 1;   
}
```

### isAsciiDigit

- 描述：判断一个数对应的 ASCII 码是否是数字，即： `0x30 <= x <= 0x39`
- 合法运算符： `!` `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 15
- 分数： 2

这一题我们使用一个比较通用的解法，即判断两个数的大小关系（假设无溢出）：
- 若 x <= y ，则有 `y - x >= 0` ，即 `y - x = y + (~x + 1)` 的符号位为 0 ，对应的表达式为：`!(((y + (~x + 1)) >> 31) & 1)`
- 而 x > y 则可转换为 `!(x <= y)` 的判断，即： `((y + (~x + 1)) >> 31) & 1`

```c
int isAsciiDigit(int x) {
  // 0x30 <= x
  //    -> sign of `x - 0x30` equals 0
  // x <= 0x39
  //    -> x < 0x3A
  //    -> sign of `x - 0x3A` equal 1
  int lower_bound = !(((x + (~0x30 + 1)) >> 31) & 1);
  int upper_bound = ((x + (~0x3A + 1)) >> 31) & 1;
  return lower_bound & upper_bound;
}
```

### conditional

- 描述：给定三个数 x, y, z ，若 x 不为 0 ，则返回 y ，否则返回 z
- 合法运算符： `!` `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 16
- 分数： 3

这里还是需要使用掩码的方式去处理，关键点就在于如何使用掩码把 y 和 z 的值组合起来。可以关注唯一可使用的逻辑运算符 `!` ，当 x 为 0 时， `!x` 为 1 ，而 -1 的二进制表示为 `0xFFFFFFFF` ，所以可以使用 `mask = ~(!x) + 1` 得到掩码 `0xFFFFFFFF` ；而当 x 不为 0 时， `mask = ~(!x) + 1` 为 `0x00000000` 。刚好可以对应不同条件下的返回值，即返回结果为 `(mask & z) | (~mask & y)` 。

```c
int conditional(int x, int y, int z) {
  // x == 0
  //    -> mask = 0xFFFFFFFF
  // x != 0
  //    -> mask = 0x00000000
  int mask = !x;
  // ignoring compiler warning ('~' on a boolean expression [-Wbool-operation])
  mask = ~mask + 1;
  return (mask & z) | (~mask & y);
}
```

### isLessOrEqual

- 描述：判断 x <= y 是否成立
- 合法运算符： `!` `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 24
- 分数： 3

看起来像是 `isAsciiDigit` 简化版，但由于 x 和 y 都不固定，所以存在溢出情况需要特判。如果 x 和 y 符号不同，只有 x 为负数时才返回 1 ；如果 x 和 y 符号相同，那么结果不会溢出，使用 `isAsciiDigit` 的方法判断即可。

```c
int isLessOrEqual(int x, int y) {
  int sign_x = (x >> 31) & 1;
  int sign_y = (y >> 31) & 1;
  int is_sign_same = !(sign_x ^ sign_y);
  // different sign -> x must be negative
  // same sign -> y - x must be non-negative
  return ((!is_sign_same) & sign_x) | (is_sign_same & !((y + (~x + 1)) >> 31 & 1));
}
```

### logicalNeg

- 描述：使用以下合法运算符实现逻辑非
- 合法运算符： `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 12
- 分数： 4

最开始想了一种比较暴力的方法，但是运算符个数超了：把 x 的所有位或在一起，如果 x 是 0 ，那么结果为 `0x00000000` ；如果 x 不是 0 ，那么结果为 `0xFFFFFFFF` 。我们对结果 + 1 ，可得 `0x00000001` 和 `0x00000000` ，刚好满足逻辑非。

看了自带的解法发现真的十分精妙，依旧是通过符号位去做判断：如果 x 为 0 ，那么 x 和 -x 的符号位都为 1 ；如果 x 不为 0 ，那么 x 和 -x 的符号位至少有一个为 1 （当且仅当 x = tmin 时，x 和 -x 的符号位均为 1）。

所以我们只要使用 `x | -x = x | (~x + 1)` 就可以区分出来，此时我们再使用算数右移 31 位，对其 + 1 ，如果 x 是 0 ，那么结果为 `0x00000001` ，否则结果为 `0x00000000` 。

```c
int logicalNeg(int x) {
  // x == 0
  //    -> x = -x = 0
  //    -> sign of `x` equals 0
  // x > 0
  //    -> sign of `-x` equals 1
  // x < 0
  //    -> sign of `x` equals 1
  // so if x = 0, x | (~x + 1) = 0, the sign will be 0.
  // otherwise, the sign will be 1, 
  // so we can get 0xFFFFFFFF after >> 31 (shift arithmetic right)
  return ((x | (~x + 1)) >> 31) + 1;
}
```

### howManyBits

- 描述：给定一个数 x ，判断其最少能用多少位的补码表示
- 合法运算符： `!` `~` `&` `^` `|` `+` `<<` `>>`
- 运算符最多个数： 90
- 分数： 4

如果 x 是正数，那么我们只需要找到最左侧的 1 即可；如果 x 是负数，那么我们需要找到 `-(x + 1) = -x - 1 = ~x + 1 - 1 = ~x` 最左侧的 1 即可。为了统一这两种情况，我们还是通过符号位生成的掩码 (`mask = x >> 31`) 来处理，当 x 为正数时， `mask = 0x00000000` ，当 x 为 负数时， `mask = 0xFFFFFFFF` ，可得需要处理的数为 `num = (~mask & x) | (mask & ~x)` 。

然后就茫然了，只知道暴力循环去处理，看了自带的思路发现仍旧非常精妙，使用二分的方式处理，可以精简大量冗余的操作判断。

- 如果最左侧 1 在高 16 位，那么 `!!(num >> 16)` 为 1 ，其对所需位数的权重为 `right16 = 16 = 1 << 4` ，然后接下来只需要判断高 16 位 (`num >> 16`) 中最左侧的 1 的位置即可。
- 如果最左侧 1 在低 16 位，那么 `!!(num >> 16)` 为 0 ，其对所需位数的权重为 `right16 = 0 = 0 << 4` ，然后接下来只需要判断低 16 位 (`num >> 16`) 中最左侧的 1 的位置即可。

然后依次每 `8`, `4`, `2`, `1`, `0`位判断即可（最后需要仍需处理每 `0` 位的情况，因为当 `num` 剩下 1 位数后，无法继续二分但还需考虑当前 1 位数对结果造成的影响），得到对应的权重 `right8`, `right4`, `right2`, `right1`, `right0` ，最后把所有权重加起来，即可得到最左侧 1 的位置，再加上符号位 1 即可得到最终所需的二进制位数。

```c
int howManyBits(int x) {
  /*
  *  for positive numbers, we need find the most significant 1. for negative
  *  numbers, we need find the most significant 0. So if x < 0, x = ~x, thus 
  *  we just need find the most significant 1. Then we use dichotomy. Each 
  *  time we check if there are 1s in half bits.
  */
  int right16, right8, right4, right2, right1, right0;
  int mask = x >> 31;
  int num = (~mask & x) | (mask & ~x);
  right16 = !!(num >> 16) << 4;
  num >>= right16;
  right8 = !!(num >> 8) << 3;
  num >>= right8;
  right4 = !!(num >> 4) << 2;
  num >>= right4;
  right2 = !!(num >> 2) << 1;
  num >>= right2;
  right1 = !!(num >> 1);
  num >>= right1;
  right0 = num;
  return right16 + right8 + right4 + right2 + right1 + right0 + 1;
}
```

### floatScale2

- 描述：给定一个单精度浮点数 x ，返回 2 * x ，若 x 是 NaN 则返回 NaN ，注意参数和返回值都用 `unsigned` 表示，但要按照二进制位识别成单精度浮点数
- 合法运算符： 有符号/无符号整数的所有运算， `||` `&&` `if` `while`
- 运算符最多个数： 30
- 分数： 4

简单题模拟并注意边界情况即可。单精度从高到底依次由以下部分组成： 1 位 (`sign`) 编码符号位 ， 8 位 (`exp`) 编码阶码， 23 位 (`frac`) 编码尾数。(`P78`) 

需要根据以下四种情况进行处理：

- 非规格化 (`exp == 0`): 需要对 `frac` 左移动 1 位，注意如果 `frac` 最高位为 1 ，则还需对 exp + 1 ，由于 exp 为 0 ，所以可以直接对 x 左移 1 位，然后加上设置符号位即可
- 规格化 (`exp != 0 && exp != 255`): 只需对 exp + 1 ，注意 exp 为 254 时，则 2 * x 为无穷大，需要设置： exp = 255, frac = 0
- 无穷大 (`exp == 255 && frac == 0`): 无穷大，可直接返回本身
- NaN (`exp = 255 && frac != 0`): NaN ，需要返回本身

```c
unsigned floatScale2(unsigned uf) {
  int sign = (uf >> 31) & 1;
  int exp = (uf & 0x7F800000) >> 23;
  // 非规格化
  if (exp == 0) {
    return (uf << 1) | (sign << 31);
  }
  // 无穷大 或者 NaN
  if (exp == 255) {
    return uf;
  }
  // 此时为规格化浮点数
  exp++;
  // 如果 2 * x 溢出，则需要表示为无穷大，
  // 设置 exp = 255, frac = 0
  if (exp == 255) {
    return (sign << 31) | 0x7F800000;
  }
  return (uf & 0x807FFFFF) | (exp << 23);
}
```

### floatFloat2Int

- 描述：给定一个单精度浮点数 x ，返回 (int) x ，注意参数用 `unsigned` 表示，但要按照二进制位识别成单精度浮点数
- 合法运算符： 有符号/无符号整数的所有运算， `||` `&&` `if` `while`
- 运算符最多个数： 30
- 分数： 4

注意浮点数的舍入规则为：四舍六入五成双 (`P83`) ，但此数是类型转换，所以是小数位直接舍去。

首先还是拆解成三部分： sign, exp 和 frac ，然后根据以下四种情况讨论即可：

- `exp < 127`: 阶码 E = exp - 127 < 0 ，该浮点数为纯小数，直接舍去到 0
- `exp == 255`: 表示无穷大和 NaN ，直接返回 `0x80000000`
- `exp > 157`: 阶码 E = exp - 127 > 30 ，尾数 M = 1.f ，那么其对应的数需要对 M 左移超过 30 位，即超出了 int 的表示范围，需要返回 `0x80000000`
- `127 <= exp <= 157`: 阶码 0 <= E = exp - 127 <= 30 ，尾数 M = 1.f ，那么如果 E <= 23 ，需要对 frac 右移 23 - E ；如果 23 < E < 30 ，需要对 frac 左移 E - 23 。这两种情况产生的结果对应 0.f << E ，然后还要加上 1 << E 。最后再判断 sign ，如果 sign 为 1 ，则原数为负数，需要返回 -num ，否则返回 num 。

```c
int floatFloat2Int(unsigned uf) {
  int num;
  int sign = (uf >> 31) & 1;
  int E = ((uf & 0x7F800000) >> 23) - 127;
  int frac = (uf & 0x007FFFFF);

  // smaller than 1.0
  if (E < 0) {
    return 0;
  }
  // bigger than tmax or NaN or infinity
  if (E > 30) {
    return 0x80000000;
  }

  // num = 0.f << E
  if(E > 23){
    num = frac << (E - 23);
  } else {
    num = frac >> (23 - E);
  }
  // num = (0.f << E) + (1 << E)
  num |= 1 << E;

  // uf is non-negative, return num
  if(sign == 0) {
    return num;
  }
  // uf is negative, return -num
  return -num;
}
```

### floatPower2

- 描述：给定一个有符号整数 x ，返回 2.0 ^ x 并返回，注意返回值用 `unsigned` 表示，但要按照二进制位识别成单精度浮点数
- 合法运算符： 有符号/无符号整数的所有运算， `||` `&&` `if` `while`
- 运算符最多个数： 30
- 分数： 4

最后一道题看起来简单一点，没有那么多需要讨论的情况。 2.0 ^ x 的 frac 必定全为 0 ， sign 也必定为 0 ，所以只用考虑 exp 的值即可。

- `exp < 0`: 非法的 exp 无法表示，即： x = E = exp - 127 < -127 时，返回 0
- `exp >= 255`: 数字太大或者是 NaN 或无穷大，即： x = E = exp - 127 >= 128 时，返回 `0xFF << 23`
- `0 <= exp < 255`: 数字可正常表示，即： exp = x + 127 ，返回 `(x + 127) << 23`

```c
unsigned floatPower2(int x) {
  // exp < 0, illegal
  if(x < -127) {
    return 0;
  }
  // exp >= 255, illegal or NaN or infinity
  if(x >= 128) {
    return 0xFF << 23;
  }
  // 0 <= exp < 255
  return (x + 127) << 23;
}
```

## 小结

花了 10 个小时左右完成了本 Lab ，刚开始还挺简单的，随便想想就知道如何处理了，后面大脑已经转不动了，可能一天超负荷运转了， `logicalNeg`, `howManyBits`, `floatFloat2Int` 这三关没有完全靠自己解答出来，或多或少参考了原有的思路（主要是前 2 关思路太精妙了，完全没接触过）。

Lab 自带的验证程序还是有一些边界条件没有考虑到，导致我看的解法部分会存在溢出/错误返回的结果却没有被发现。

看完书上第二章，印象最深刻的应该就是 `tmin = -tmin` 以及一道练习题：对于一种具有 n 位小数的浮点格式，给出不能准确描述的最小正整数的公式（因为要准确表示它需要 n + 1 位小数），假设阶码字段长度 k 足够大，可以表示的阶码范围不会限制这个问题。 (`P83`)

做完本 Lab ，印象最深刻的就是如何使用各种运算完成类似三元运算符的逻辑：根据不同的条件生成掩码，假设条件成立时生成掩码为 `mask = 0xFFFFFFFF` ，条件不成立时生成掩码为 `mask = 0x00000000` ，那么 `num = (mask & y) | (~mask & z)` 。