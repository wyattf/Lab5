/*
 * This is the source code for the switch.
 */

#include <stdio.h>
#include "main.h"
#include "link.h"
#include "switch.h"

/***************************/
/*   Functions for queue   */
/***************************/
// Initialize Queue
void queueInit(Queue * pqueue)
{
    pqueue->size = 0;
    pqueue->head = 0;
    pqueue->tail = -1;
}

// Check if queue is empty
int queueEmpty(Queue * pqueue)
{
    if(pqueue->size == 0)
        return 1;
    else return 0;
}

// Check if queue is full
int queueFull(Queue * pqueue)
{
    if(pqueue->size == MAXQUEUE)
        return 1;
    else return 0;
}

// Append element to queue
void queueAppend(Queue * pqueue, Element element)
{
    if(!queueFull(pqueue))
    {
        pqueue->tail = (pqueue->tail+1)%MAXQUEUE;
        pqueue->elements[pqueue->tail] = element;
        pqueue->size++;
    }
    else
    {
        printf("Error: queue is full\n");
    }
}

// Remove element from queue
Element queueServe(Queue * pqueue)
{
    Element deleted;
    if(!queueEmpty(pqueue))
    {
        deleted = pqueue->elements[pqueue->head];
        pqueue->head = (pqueue->head+1)%MAXQUEUE;
        pqueue->size--;
        return deleted;
    }
    else
    {
        printf("Error: queue is empty\n");
    }
}

// Display elements in queue
void queueDisplay(Queue * pqueue)
{
    int i;

    printf("Source Address\tDest Address\tLength\tPayload\n");

    for(i=pqueue->head; i <= pqueue->tail; i++)
    {
        printf("\t%d\t\t%d\t%d\t%s\n", pqueue->elements[i].srcaddr, pqueue->elements[i].dstaddr, pqueue->elements[i].length, pqueue->elements[i].payload);
    }
}



/**************************************/
/*   Functions for Forwarding Table   */
/**************************************/
// Initialize table
void tableInit(Table * ftable)
{
    ftable->size = 0;
}

// Add a new entry that does not exist in the table
void tableAddEntry(Table * ftable, int valid, int dstaddr, int linkOut)
{
    TableEntry newentry;
    newentry.valid = valid;
    newentry.dstAddr = dstaddr;
    newentry.linkOut = linkOut;
    ftable->entries[ftable->size] = newentry;
    ftable->size++;
}

// Find the index of an entry in the table
int tableEntryIndex(Table * ftable, int dstaddr)
{
    int i;
    for(i=0; i<ftable->size; i++)
    {
        if(ftable->entries[i].dstAddr == dstaddr)
            return i;
    }
    return -1;
}

// Update an existing entry in the table
void tableUpdateEntry(Table * ftable, int i, int valid, int linkOut)
{
    ftable->entries[i].valid = valid;
    ftable->entries[i].linkOut = linkOut;
}

// Update the table
void tableUpdate(Table * ftable, int valid, int dstaddr, int linkOut)
{
    int i;
    
    i = tableEntryIndex(ftable, dstaddr);

    if(i == -1)
        tableAddEntry(ftable, valid, dstaddr, linkOut);
    else tableUpdateEntry(ftable, i, valid, linkOut);
}

// Retrieve the value of the out link
int tableGetOutLink(Table * ftable, int dstaddr)
{
    int i;
    int linkOut;

    i = tableEntryIndex(ftable, dstaddr);

    if(i < ftable->size)
    {
        linkOut = ftable->entries[i].linkOut;
        return linkOut;
    }
    return i;
}

// Display table
void tableDisplay(Table * ftable)
{
    int i;

    printf("Valid\tDestination Address\tLink Out\n");

    for(i = 0; i < ftable->size; i++)
    {
        printf("%d\t%d\t\t\t%d\n", ftable->entries[i].valid, ftable->entries[i].dstAddr, ftable->entries[i].linkOut);
    }
}



/****************************/
/*   Functions for Switch   */
/****************************/
// Initialize state of switch
void switchInit(switchState * sstate, int physID)
{
    sstate->physId = physID;
    sstate->numInLinks = 0;
    sstate->numOutLinks = 0;
    tableInit(&(sstate->forwardingTable));
    queueInit(&(sstate->packetQueue));
}

// Main loop for switch
void switchMain(switchState * sstate)
{

    while(1){
        // Check all incoming links for arriving packets
            // If there is an incoming packet
                // Put in packet queue
                // Update forwarding table
        // If queue is not empty, transmit a packet
            //
        // Sleep for 10 milliseconds
    }

}

