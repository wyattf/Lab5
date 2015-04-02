#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "utilities.h"
#include "link.h"
#include "man.h"
#include "host.h"
#include "switch.h"
#include "net.h"

#define EMPTY_ADDR  0xffff  /* Indicates the empty address */
                             /* It also indicates that the broadcast address */
#define MAXBUFFER 1000
#define PIPEWRITE 1 
#define PIPEREAD  0

int getTopology(int sourceId[], int destId[], int * numHosts, int * numSwitches, int * numLinks);


void main()
{
    hostState hstate;             /* The host's state */
    linkArrayType linkArray;
    manLinkArrayType manLinkArray;

    pid_t pid;  /* Process id */
    int physid; /* Physical ID of host */

    int i, k;
    int numHosts, numSwitches, numLinks;
    switchState sState;

    int sourceId[20];
    int destId[20];

    if ( getTopology(sourceId, destId, &numHosts, &numSwitches, &numLinks) )
        return;


    /* Allocate space for link arrays. */
    manLinkArray.link = (managerLink*)malloc(numHosts*sizeof(managerLink));
    linkArray.link = (LinkInfo*)malloc(numLinks*sizeof(LinkInfo));
    sState.inLinks = (LinkInfo*)malloc(numHosts*sizeof(LinkInfo));
    sState.outLinks = (LinkInfo*)malloc(numHosts*sizeof(LinkInfo));



    /* 
     * Create nonblocking (pipes) between manager and hosts 
     * assuming that hosts have physical IDs 0, 1, ... 
     */
    manLinkArray.numlinks = numHosts;
    netCreateConnections(& manLinkArray);

    /* Create links between nodes but not setting their end nodes */
    linkArray.numlinks = numLinks;
    netCreateLinks(& linkArray);

    /* Set the end nodes of the links */
    netSetNetworkTopology(& linkArray, sourceId, destId);

    /* Create nodes and spawn their own processes, one process per node */ 
    for (physid = 0; physid < numHosts; physid++) 
    {
        pid = fork();

        if (pid == -1) 
        {
            printf("Error:  the fork() failed\n");
            return;
        }

        else if (pid == 0) 
        { /* The child process -- a host node */

            /* Initialize host's state */
            hostInit(&hstate, physid);

            /* Initialize the connection to the manager */ 
            hstate.manLink = manLinkArray.link[physid];

            /* 
             * Close all connections not connect to the host
             * Also close the manager's side of connections to host
             */
            netCloseConnections(& manLinkArray, physid);

            /* Initialize the host's incident communication links */
            k = netHostOutLink(&linkArray, physid); /* Host's outgoing link */
            hstate.linkout = linkArray.link[k];

            k = netHostInLink(&linkArray, physid); /* Host's incoming link */
            hstate.linkin = linkArray.link[k];

            /* Close all other links -- not connected to the host */
            netCloseHostOtherLinks(& linkArray, physid);

            /* Go to the main loop of the host node */
            hostMain(&hstate);
        }  
        
        usleep(20000);
    }

    for (physid = numHosts; physid < numHosts + numSwitches; physid++)
    {
        pid = fork();

        if (pid == -1) 
        {
            printf("Error: the fork() failed\n");
            return;
        }

        else if (pid == 0) 
        {
            /* initialize the switch's state */
            switchInit(&sState, physid);

            /* Close unnecessary links to manager */
            netCloseAllManLinks(&manLinkArray);

            /* initlialize the switch's links */
            netSwitchLinks(&linkArray, &sState, physid);

            /* close all other connections that are not connected to switch */
            netCloseSwitchOtherLinks(&linkArray, physid);

            switchMain(&sState);
        }

        usleep(20000);
    }


    /* Manager */

    /* 
     * The manager is connected to the hosts and doesn't
     * need the links between nodes
     */

    /* Close all links between nodes */
    netCloseLinks(&linkArray);

    /* Close the host's side of connections between a host and manager */
    netCloseManConnections(&manLinkArray);

    /* Go to main loop for the manager */
    manMain(& manLinkArray);

    /* 
     * We reach here if the user types the "q" (quit) command.
     * Now if we don't do anything, the child processes will continue even
     * after we terminate the parent process.  That's because these
     * child proceses are running an infinite loop and do not exit 
     * properly.  Since they have no parent, and no way of controlling
     * them, they are called "zombie" processes.  Actually, to get rid
     * of them you would list your processes using the LINUX command
     * "ps -x".  Then kill them one by one using the "kill" command.  
     * To use the kill the command just type "kill" and the process ID (PID).
     *
     * The following system call will kill all the children processes, so
     * that saves us some manual labor
     */
    kill(0, SIGKILL); /* Kill all processes */
}





int getTopology(int sourceId[], int destId[], int * numHosts, int * numSwitches, int * numLinks)
{
    int err = 0;
    int i = 0;
    char fName[100];
    char word[100];
    char line[100];
    FILE * file;

    *numHosts = -1;
    *numSwitches = -1;
    *numLinks = -1;

    printf("Please enter a file to read from:  ");
    scanf("%s", fName);

    file = fopen(fName, "r");

    if ( file != NULL )
    {
        fgets(line, sizeof line, file);
        findWord(word, line, 1);
        *numHosts = ascii2Int(word);
        findWord(word, line, 2);
        *numSwitches = ascii2Int(word);
        findWord(word, line, 3);
        *numLinks = ascii2Int(word);


        while ( fgets(line, 100, file) != NULL ) 
        {
            findWord(word, line, 1);
            sourceId[i] = ascii2Int(word);
            findWord(word, line, 2);
            destId[i] = ascii2Int(word);
            i++;
        }

        fclose(file);
    }

    else
    {
        printf("Error opening the file!");
        err = 1;
    }

    if (numHosts < 0 || numSwitches < 0 || numLinks < 0 )
    {
        printf("The file was invalid!");
        err = 2;
    }

    return err;
}
