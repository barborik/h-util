/*
+----------------------------------------------+
|                                              |
|  Adam Barborík's big integer implementation  |
|                                              |
|   - dlist.h from the h-util                  |
|     collection is required                   |
|                                              |
+----------------------------------------------+
*/

#ifndef __BIGINT_
#define __BIGINT_

#include <stdio.h>
#include <stdlib.h>

#include "dlist.h"

typedef struct
{
    dlist_t *data;
} bigint_t;

void bi_init(bigint_t *b)
{
    b->data = (dlist_t *)malloc(sizeof(dlist_t));
    dl_init(b->data, sizeof(signed char));
}

void bi_free(bigint_t *b)
{
    dl_free(b->data);
    free(b->data);
}

void bi_load(bigint_t *b, char *val)
{
    int i = 0, sign = 1;
    int len = strlen(val);

    dl_clear(b->data);

    if (val[0] == '+' || val[0] == '-')
    {
        i = 1;
    }

    if (val[0] == '-')
    {
        sign = -1;
    }

    while (i < len)
    {
        dl_ins(b->data, &(signed char){(val[i] - '0') * sign}, 0);
        i++;
    }
}

bigint_t *bi_add(bigint_t *op_, bigint_t *op__)
{
    bigint_t *l = op__, *s = op_;
    bigint_t *res = (bigint_t *)malloc(sizeof(bigint_t));
    bi_init(res);

    if (op_->data->used > op__->data->used)
    {
        l = op_;
        s = op__;
    }

    // copy bytes from the larger operand
    for (int i = 0; i < l->data->used; i++)
    {
        dl_add(res->data, l->data->get[i]);
    }
    dl_add(res->data, &(signed char){0});

    // add bytes from the smaller operand to the result (larger operand)
    for (int i = 0; i < s->data->used; i++)
    {
        (*(signed char *)res->data->get[i]) += *(signed char *)s->data->get[i];
    }

    // carry over
    for (int i = 0; i < res->data->used - 1; i++)
    {
        while (*(signed char *)res->data->get[i] > 9)
        {
            *(signed char *)res->data->get[i] -= 10;
            *(signed char *)res->data->get[i + 1] += 1;
        }

        while (*(signed char *)res->data->get[i] < -9)
        {
            *(signed char *)res->data->get[i] += 10;
            *(signed char *)res->data->get[i + 1] -= 1;
        }
    }

    // remove leading zero
    if (!*(signed char *)res->data->get[res->data->used - 1])
    {
        dl_rem(res->data, res->data->used - 1);
    }

    return res;
}

bigint_t *bi_sub(bigint_t *op_, bigint_t *op__)
{
    // create a copy of the second operand with inverted sign
    bigint_t inv;
    bi_init(&inv);

    for (int i = 0; i < op__->data->used; i++)
    {
        dl_add(inv.data, op__->data->get[i]);
        *(signed char *)inv.data->get[i] *= -1;
    }

    // add like a boss B-)
    bigint_t *res = bi_add(op_, &inv);

    bi_free(&inv);
    return res;
}

void debug_print(bigint_t *b)
{
    if (*(signed char *)b->data->get[b->data->used - 1] < 0)
    {
        printf("- ");
    }

    for (int i = b->data->used; i > 0; i--)
    {
        // putchar(() + '0');
        printf("%d ", abs(*(signed char *)b->data->get[i - 1]));
    }
    printf("\n");
}

#endif
