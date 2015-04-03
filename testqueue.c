/*
 * File to test packet queue
 */

#include <stdio.h>
#include <string.h>
#include "switch.h"

main()
{
    Element e1, e2, e3, e4, e5;
    Queue q;

    e1.srcaddr = 1;
    e1.dstaddr = 2;
    e1.length = 3;
    strcpy(e1.payload, "abc");
    e1.valid = 1;
    e1.new = 0;

    e2.srcaddr = 4;
    e2.dstaddr = 5;
    e2.length = 6;
    strcpy(e2.payload, "abcdef");
    e2.valid = 1;
    e2.new = 0;

    e3.srcaddr = 7;
    e3.dstaddr = 8;
    e3.length = 9;
    strcpy(e3.payload, "abcdefghi");
    e3.valid = 1;
    e3.new = 0;

    e4.srcaddr = 10;
    e4.dstaddr = 11;
    e4.length = 12;
    strcpy(e4.payload, "abcdefghijkl");
    e4.valid = 1;
    e4.new = 0;

    queueInit(&q);
    queueAppend(&q, e1);
    queueAppend(&q, e2);
    queueAppend(&q, e3);
    queueAppend(&q, e4);
    queueDisplay(&q);
    e5 = queueServe(&q);
    printf("Removed packet: %d\t%d\t%d\t%s\n", e5.srcaddr, e5.dstaddr, e5.length, e5.payload);
    queueDisplay(&q);
}
