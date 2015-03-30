/*
 * This is the source code for the switch.
 */

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
    if(!queueFull)
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
    if(!queueEmpty)
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

    printf("Source Address\t\tDestination Address\n");

    for(i=0; i < pqueue->size; i++)
    {
        printf("\t%d\t\t\t%d\n", pqueue->element[i].srcaddr, pqueue->element[i].dstaddr);
    }
    
}


/**************************************/
/*   Functions for Forwarding Table   */
/**************************************/



/****************************/
/*   Functions for Switch   */
/****************************/
// Initialize state of switch
void switchInit(switchState * sstate, int physID)
{

}

// Main loop for switch
void switchMain(switchState * sstate)


