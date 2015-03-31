#include <stdio.h>
#include "main.h"
#include "link.h"

#define MAXQUEUE 10000
#define MAXTABLE 100

/* Data types for queue */
typedef packetBuffer Element;
typedef struct {
    int head;
    int tail;
    int size;
    Element elements[MAXQUEUE];
    } Queue;


/* Data types for forwarding table */
typedef struct {
    int valid;
    int dstAddr;
    int linkOut;
    } TableEntry;

typedef struct{
    int size;
    TableEntry entries[MAXTABLE];
    } Table;


/* Data types for switch */
typedef struct {
    int physId;             // Physical ID
    int numInLinks;         // Number of incoming links
    int numOutLinks;        // Number of outgoing links
    LinkInfo * inLinks;      // Incoming communication link
    LinkInfo * outLinks;     // Outgoing communication link
    Table forwardingTable;  // Forwarding table
    Queue packetQueue;      // Queue to hold packets
    } switchState;


/* Functions for queue */
void queueInit(Queue * pqueue);
int queueEmpty(Queue * pqueue);
int queueFull(Queue * pqueue);
void queueAppend(Queue * pqueue, Element entry);
Element queueServe(Queue * pqueue);
void queueDisplay(Queue * pqueue);


/* Functions for forwarding table */
void tableInit(Table * ftable);
void tableAddEntry(Table * ftable, );
int tableEntryIndex(Table * ftable, int dstaddr);
void tableUpdateEntry(Table * ftable, );
void tableUpdate(Table * ftable, );
int tableGetOu:w
tLink(Table * ftable, );
void tableDisplay(Table * ftable);


/* Functions for switch */
void switchInit(switchState * sstate, int physid);
void switchMain(switchState * sstate);
