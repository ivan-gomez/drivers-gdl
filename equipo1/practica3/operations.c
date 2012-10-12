#include "operations.h"

long sum (long num1, long num2)
{
return num1+num2;
}
long substraction (long num1, long num2)
{
return num1-num2;
}
long mult (long num1, long num2)
{
return num1*num2;
}
long div (long num1, long num2)
{
return num1/num2;
}
long oper (long num1, long num2, char cmd){
if(cmd=='+')
return sum(num1,num2);
if(cmd=='-')
return substraction(num1,num2);
if(cmd=='*')
return mult(num1,num2);
if(cmd=='/')
return div(num1,num2);
}
