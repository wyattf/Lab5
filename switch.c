/*
 * This is the source code for the switch.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "main.h"
#include "link.h"
#include "switch.h"


#define TENMILLISEC 10000

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

    if(i == -1)
        return i;

    else
    {
        linkOut = ftable->entries[i].linkOut;
        return linkOut;
    }
}

// Display table
void tableDisplay(Table * ftable)
{
    int i;

    printf("Valid\tDestination Address\tLink Out\n");

    for(i=0; i<ftable->size; i++)
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
    int i;

    sstate->physId = physID;
    sstate->numInLinks = 0;
    sstate->numOutLinks = 0;
    tableInit(&(sstate->forwardingTable));
    queueInit(&(sstate->packetQueue));

    sstate->parent = physID;
    sstate->root = physID;
    sstate->distance = INFINITY;

    for(i = 0; i < MAXLINKS; i++)
    {
        sstate->child[i] = 0;
        sstate->nodeLinks[i] = 1;
    }
}

// Main loop for switch
void switchMain(switchState * sstate)
{
    int l;                  // Counter for incoming links
    int i;                  // Counter for packets
    int j;                  // Counter for outgoing links
    int packetCount = 0;    // Number of packets on link
    int outLink;            // Link to transmit packet on
    int inLink;             // Link incoming packet arrived on
    packetBuffer outPacket;
    packetBuffer packets[10];
    int count = 10;         // Count to send packets

    while(1)
    {
        // Check all incoming links for arriving packets
        for(l=0; l<sstate->numInLinks; l++)
        {
            // Check link for packets
            packetCount = linkReceive(&(sstate->inLinks[l]), packets);

            // For all incoming packets on link
            for(i=0;i<packetCount;i++)
            {
                if ( packets[i].type == STATEPACKET )
                {

                    sstate->nodeLinks[l] = 0;

                        if ( sstate->root > packets[i].root )
                        {
                            sstate->root = packets[i].root;
                            sstate->parent = l;
                            sstate->distance = INFINITY;
                        }

                        else if ( packets[i].distance + 1 < sstate->distance )
                        {
                            if ( packets[i].root <= sstate->root )
                            {
                                sstate->parent = l;
                                sstate->distance = packets[i].distance + 1;
                            }
                        }

                        if ( packets[i].child )   sstate->child[l] = 1;
                        else                    sstate->child[l] = 0;
                }

                else 
                {
                    // Put in packet queue
                    queueAppend(&(sstate->packetQueue), packets[i]);

                    // Update forwarding table
                    tableUpdate(&(sstate->forwardingTable), packets[i].valid, packets[i].srcaddr, l);
                }
            }
        }

        // If count is 0
        if(count == 0)
        {
            // Reset count to 10
            count = 10;

            outPacket.type = STATEPACKET;
            outPacket.valid = 1;
            outPacket.length = sstate->physId;
            outPacket.distance= sstate->distance;
            outPacket.root = sstate->root;

            // Send packet to all neighbor switches
            for (i=0; i<sstate->numOutLinks; i++)
            {
                if (sstate->parent == i)    outPacket.child = 1;
                else                        outPacket.child = 0;
                linkSend(&(sstate->outLinks[i]), &outPacket);
            }
        }


        // If queue is not empty, transmit a packet
        else if(sstate->packetQueue.size != 0)
        {
            // Get packet from head of packet Queue
            outPacket = queueServe(&(sstate->packetQueue));

            // Check forwarding table for outgoing link
            outLink = tableGetOutLink(&(sstate->forwardingTable), outPacket.dstaddr);

            if (outPacket.srcaddr != -1 )
            {
                // If out going link exists in table
                if(outLink != -1)
                {
                    // Transmit packet on the outgoing link
                    linkSend(&(sstate->outLinks[outLink]), &outPacket);
                }

                // Else send to all links except for the incoming one
                else
                {
                    // Get source link of packet
                    inLink = tableGetOutLink(&(sstate->forwardingTable), outPacket.srcaddr);

                    // For all outgoing links
                    for(j=0; j<sstate->numOutLinks; j++)
                    {
                        // Send on link if its not the incoming link
                        if(j != inLink && (sstate->child[j] || sstate->parent == j || sstate->nodeLinks[j]))
                            linkSend(&(sstate->outLinks[j]), &outPacket);
                    }
                }
            }
        }

        // Update count
        count--;

        // Sleep for 10 milliseconds
        usleep(TENMILLISEC);
    }
}

