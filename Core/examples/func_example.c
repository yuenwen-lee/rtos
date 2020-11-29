/*
 * func_call.c
 *
 *  Created on: Jul 16, 2013
 *      Author: Y.W. Lee
 */

#include <math.h>
#include "func_example.h"

int api_A(int a0, int b0)
{
    int c0 = a0 + b0;
    return(c0);
}


int api_B(int a1, int b1)
{
    int c1 = a1 * b1;
    return(c1);
}


int api_C(int a2, int b2)
{
    int c2 = a2 - b2;
    return(c2);
}


float api_sinf(float w)
{
    volatile float u, v;
    u = sinf(w);
    v = asinf(u);
    return v;
}


float api_tanf(float w)
{
    volatile float u, v;
    u = tanf(w);
    v = atanf(u);
    return v;
}
