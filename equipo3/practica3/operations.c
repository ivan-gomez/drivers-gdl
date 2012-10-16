#include "operations.h"

long sum (long num1, long num2)
{
    return num1 + num2;
}

long substract (long num1, long num2)
{
    return num1 - num2;
}

long multiply (long num1, long num2)
{
    return num1 *num2;
}

long division (long num1, long num2)
{
    return num1 / num2;
}

long operation (char operand, long num1, long num2)
{
    if (operand == '+')
        return sum (num1, num2);

    if (operand == '-')
        return substract (num1, num2);

    if (operand == '*')
        return multiply (num1, num2);

    if (operand == '/')
        return division (num1, num2);
}