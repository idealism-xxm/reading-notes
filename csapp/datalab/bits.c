/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  // x ^ y
  //    = ~(x ⊙ y)
  //    = ~((x & y) | (~x & ~y))
  //    = ~(x & y) & ~(~x & ~y)
  return ~(x & y) & ~(~x & ~y);
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  // tmin = 0x80000000
  return 1 << 31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  // tmax -> (~x + ~x) = 0x80000000 + 0x80000000 = 0x00000000
  // -1 -> ~x = 0x00000000
  //    -> ~x + ~x = 0x00000000 + 0x00000000 = 0x00000000
  return !((~x + ~x) | !(~x));
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  // a ^ b = 0 -> a = b
  int mask = 0xAA;
  mask |= mask << 8;
  mask |= mask << 16;
  return !((x & mask) ^ mask);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  //-x = 0 - x
  //   = -1 + 1 - T2B(x)
  //   = 0b1111 + 1 - T2B(x)
  //   = 0b1111 - T2B(x) + 1
  //   = ~T2B(x) + 1
  //   = ~x + 1
  return ~x + 1;   
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
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
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
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
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int sign_x = (x >> 31) & 1;
  int sign_y = (y >> 31) & 1;
  int is_sign_same = !(sign_x ^ sign_y);
  // different sign -> x must be negative
  // same sign -> y - x must be non-negative
  return ((!is_sign_same) & sign_x) | (is_sign_same & !((y + (~x + 1)) >> 31 & 1));
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
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
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
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
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
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
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
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
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
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
