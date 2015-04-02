/* 
 * This is the source code for the host.  
 * hostMain is the main function for the host.  It is an infinite
 * loop that repeatedy polls the connection from the manager and
 * its input link.  
 *
 * If there is command message from the manager,
 * it parses the message and executes the command.  Then it
 * will send a reply back to the manager.  
 *
 * If there is a packet on its incoming link, it checks if
 * the packet is destined for it.  Then it stores the packet
 * in its receive packet buffer.
 *
 * There is also a 10 millisecond delay in the loop caused by
 * the system call "usleep".  This puts the host to sleep for a
 * fixed amount of time.  This should keep the all the nodes
 * in the network working at the same rate; all nodes will do
 * some tasks then sleep for a fixed amount of time, then do
 * additional tasks then sleep for a fixed amount of time.  
 * This help ensure that no node does too much work compared
 * to the others.  Thus, the nodes all work at the same rate.
 * This prevents some nodes working too fast for the other
 * nodes to keep up.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "utilities.h"
#include "link.h"
#include "man.h"
#include "host.h"

#define EMPTY_ADDR  0xffff  /* Indicates that the empty address */
/* It also indicates that the broadcast address */
#define MAXBUFFER 1000
#define PIPEWRITE 1 
#define PIPEREAD  0
#define TENMILLISEC 10000   /* 10 millisecond sleep */

/* 
 * hostInit initializes the host.  It calls
 * - hostInitState which initializes the host's state.
 * - hostInitRcvPacketBuff, which initializes the receive packet buffer
 * - hostInitSendPacketBuff, which initializes the send packet buffer
 */
void hostInitState(hostState * hstate, int physid); 
void hostInitRcvPacketBuff(packetBuffer * packetbuff);
void hostInitSendPacketBuff(packetBuffer * packetbuff);

/*
 * hostMain is the main loop for the host. It has an infinite loop.
 * In the loop it first calls
 * hostCommandReceive to check if a command message came from the
 * manager.
 * If a command arrived, then it checks the first word of the
 * message to determine the type of command.  Depending on the
 * command it will call
 * - hostSetNetAddr to set the host's network address
 *      The command message should be "SetNetAddr <network address>"
 * - hostSetMainDir to set the host's main directory
 *      The command message should be "SetMainDir <directory name>"
 * - hostClearRcvFlg to clear the host's receive flag
 * - hostUploadPacket to upload a file to the host's send packet
 *      buffer. The command message should be "UploadPacket <file name>"
 * - hostDownloadPacket to download the payload of the host's
 *      receive packet buffer to a file.  The command message
 *      should be "DownloadPacket <file name>"
 * - hostTransmitPacket to transmit the packet in the send packet buffer.
 *      The command message should be "TransmitPacket <destination address>"
 * - hostGetHostState to get the host's state.  The command message
 *      should be "GetHostState".  
 */
int  hostCommandReceive(managerLink * manLink, char command[]);

void hostSetNetAddr(hostState * hstate, int netaddr, char replymsg[]);
void hostSetMainDir(hostState * hstate, char filename[], char replymsg[]);
void hostClearRcvFlg(hostState * hstate, char replymsg[]);
void hostUploadPacket(hostState * hstate, char fname[], char replymsg[]); 
void hostDownloadPacket(hostState * hstate, char fname[], char replymsg[]); 
void hostTransmitPacket(hostState * hstate, char replymsg[]);
void hostGetHostState(hostState * hstate, managerLink * manLink, char replymsg[]);

/* 
 * After host executes a command from the manager, it sends a reply
 * message to the manager in the format " Replytype <reply[]>".
 * - Replytype can be
 *    o "DISPLAY" which means the rest of the message should be 
 *      displayed on the user's console
 *    o "GetHostStateACK" which means the rest of the message is the
 *      host's state
 */ 
void hostReplySend(managerLink * manLink, char replytype[], char reply[]);

void hostInitTransmit(hostState * hstate, char word[], char replymsg[]);
void hostInitDataBuffer(dataBuffer * buff);



/* This is used to send a message to the manager */
void hostToManSend(managerLink * manLink, char reply[]);

/*
 * Functions
 */

/*
 * hostTransmitPacket will transmit a packet in the send packet buffer
 */
void hostTransmitPacket(hostState * hstate, char replymsg[])
{
    int i;
    int length = hstate->sendDataBuff.length - hstate->sendDataBuff.pos;

    if (length > PAYLOAD_LENGTH)
        length = PAYLOAD_LENGTH;
    /* Set up the send packet buffer's source and destination addresses */
    hstate->sendPacketBuff.dstaddr = hstate->sendDataBuff.dstaddr;
    hstate->sendPacketBuff.srcaddr = hstate->netaddr;
    hstate->sendPacketBuff.length = length;
    hstate->sendPacketBuff.end = 0;
    hstate->sendPacketBuff.start = 0;

    /* raise the start flag if the current position is 0 */
    if (hstate->sendDataBuff.pos == 0)
        hstate->sendPacketBuff.start = 1;
    
    /* rais the valid flag if the data is valid in the data buffer */
    if (hstate->sendDataBuff.valid)
        hstate->sendPacketBuff.valid = 1;

    /* transfer data from the data buffer to the packet buffer */
    for (i = 0; i < length; i++)
        hstate->sendPacketBuff.payload[i] = hstate->sendDataBuff.data[i+hstate->sendDataBuff.pos];

    /* increment the position */
    hstate->sendDataBuff.pos += length;

    /* set the end flag if the final index has been reached */
    if (hstate->sendDataBuff.pos >= hstate->sendDataBuff.length)
    {
        hstate->sendDataBuff.pos = 0;
        hstate->sendDataBuff.busy = 0;
        hstate->sendPacketBuff.end = 1;
    }

    /* Transmit the packet on the link */
    if ( linkSend(&(hstate->linkout), &(hstate->sendPacketBuff)) == -1 )
    {
        strcpy(replymsg,"Error: Could not send packet, aborting transmit");
        hostInitDataBuffer(&(hstate->sendDataBuff));
        return;
    }

    /* Message to be sent back to the manager */
    if (hstate->sendPacketBuff.end == 1) 
        strcpy(replymsg,"Final Packet Sent");

    else
        strcpy(replymsg,"Packet sent");
}

/* 
 * Main loop of the host node
 *
 * It polls the manager connection for any requests from
 * the manager, and repliies
 *
 * Then it polls any incoming links and downloads any
 * incoming packets to its receive packet buffer
 *
 * Then it sleeps for 10 milliseconds
 *
 * Then back to the top of the loop
 *
 */
void hostMain(hostState * hstate)
{
    char buffer[1000]; /* The message from the manager */
    char word[1000];
    int  value;
    char replymsg[1000];   /* Reply message to be displayed at the manager */
    packetBuffer tmpbuff[10]; /* array of packet buffers to store multiple incoming packets */
    int packetCount, i;

    while(1)
    {
        /* Check if there is a command message from the manager */
        int length; /* Size of string in pipe */
        length = hostCommandReceive(&(hstate->manLink),buffer);

        if (length > 1)
        { /* Execute the manager's command */
            findWord(word, buffer, 1);

            if (strcmp(word, "SetNetAddr")==0) 
            {
                findWord(word, buffer, 2); /* Find net address */
                value = ascii2Int(word);   /* Convert it to integer */
                hostSetNetAddr(hstate, value, replymsg);
                hostReplySend(&(hstate->manLink),"DISPLAY",replymsg);
            }

            else if (strcmp(word, "SetMainDir")==0)
            {
                findWord(word, buffer, 2); /* Find directory name */
                hostSetMainDir(hstate, word, replymsg);
                hostReplySend(&(hstate->manLink),"DISPLAY",replymsg);
            }
            else if (strcmp(word, "ClearRcvFlg")==0) 
            {
                hostClearRcvFlg(hstate, replymsg);
                hostReplySend(&(hstate->manLink),"DISPLAY",replymsg);
            }

            else if (strcmp(word, "UploadPacket")==0) 
            {
                findWord(word, buffer, 2); /* Find file name */
                hostUploadPacket(hstate, word, replymsg); 
                hostReplySend(&(hstate->manLink), "DISPLAY",replymsg);
            }

            else if (strcmp(word, "DownloadPacket")==0) 
            {
                findWord(word, buffer, 2); /* Find file name */
                hostDownloadPacket(hstate, word, replymsg);
                hostReplySend(&(hstate->manLink), "DISPLAY",replymsg);
            }

            else if (strcmp(word, "GetHostState")==0)
            {
                hostGetHostState(hstate, &(hstate->manLink), replymsg);
                hostReplySend(&(hstate->manLink), "GetHostStateAck",replymsg);
            }

            else if (strcmp(word, "TransmitPacket")==0) 
            {
                hostInitTransmit(hstate, buffer, replymsg);
                hostReplySend(&(hstate->manLink), "DISPLAY", replymsg);
            }
        } /* end of if */

        /* transmit a packet */
        if (hstate->sendDataBuff.busy == 1)
            hostTransmitPacket(hstate, replymsg);

        /* Check if there are any incoming packets */
        packetCount = linkReceive(&(hstate->linkin), tmpbuff);

        /*
         * If there is at least one packet and if the packet's destination address
         * is the host's network address then store the packet in the
         * receive packet buffer
         */
        for (i = 0; i < packetCount; i++) 
        {
            if (tmpbuff[i].dstaddr == hstate->netaddr && tmpbuff[i].valid == 1) 
            {
                /* clear old data and reinitialize values if the start of a new packet is read */
                if (tmpbuff[i].start == 1) 
                {
                    memset(hstate->rcvDataBuff.data, 0, sizeof(hstate->rcvDataBuff.data));
                    hstate->rcvDataBuff.length = 0;
                    hstate->rcvflag = 0;
                }

                /* add new data to rcv buffer */
                strcat(hstate->rcvDataBuff.data, tmpbuff[i].payload);
                hstate->rcvDataBuff.length += tmpbuff[i].length;
                hstate->rcvDataBuff.srcaddr = tmpbuff[i].srcaddr;
                hstate->rcvDataBuff.dstaddr = tmpbuff[i].dstaddr;

                /* raise the rcv flag if the final packet of a payload has been received */
                if (tmpbuff[i].end == 1) 
                {
                    hstate->rcvflag = 1;
                    hstate->rcvDataBuff.valid = 1;
                }
            }
        }

        usleep(TENMILLISEC);

    } /* End of while loop */

}

/*
 * Sets the host's network address.  Also creates a reply message
 * to the manager.
 */
void hostSetNetAddr(hostState * hstate, int netaddr, char replymsg[])
{
    hstate->netaddr = netaddr;
    strcpy(replymsg, "Network address is set");
}

/* 
 * The host checks the connection from the manager.  If the manager sent
 * a command, the host stores it in "command[]".  It returns the length
 * of the received message + 1.
 */ 

int hostCommandReceive(managerLink * manLink, char command[])
{
    int n;
    n = read(manLink->toHost[PIPEREAD],command,250);
    command[n] = '\0';
    return n+1;
}

/* 
 * The host sends a message to the manager.
 */
void hostToManSend(managerLink * manLink, char reply[])
{
    write(manLink->fromHost[PIPEWRITE],reply,strlen(reply));
}

/* 
 * hostReplySend is called after the host executes a command 
 * from the manager. It sends a reply
 * message to the manager in the format " Replytype <reply[]>".
 * - Replytype can be
 *    o "DISPLAY" which means the rest of the message should be 
 *      displayed on the user's console
 *    o "GetHostStateACK" which means the rest of the message is the
 *      host's state
 */ 
void hostReplySend(managerLink * manLink, char replytype[], char replymsg[])
{
    char reply[1000];

    reply[0] = '\0';
    appendWithSpace(reply, replytype);
    appendWithSpace(reply, replymsg);
    hostToManSend(manLink, reply);
}

/* 
 * Initializes the host.   
 */
void hostInit(hostState * hstate, int physid)
{

    hostInitState(hstate, physid);     /* Initialize host's state */

    /* Initialize the receive and send packet buffers */
    hostInitRcvPacketBuff(&(hstate->rcvPacketBuff));  
    hostInitSendPacketBuff(&(hstate->rcvPacketBuff)); 
}

/* 
 * Initialize send packet buffer 
 */
void hostInitSendPacketBuff(packetBuffer * packetbuff)
{
    packetbuff->valid = 0;
    packetbuff->new = 0;
    packetbuff->end = 0;
    packetbuff->start = 0;
}


/* 
 * Upload a file in the main directory into the send packet buffer
 */
void hostUploadPacket(hostState * hstate, char fname[], char replymsg[]) 
{
    char c;
    FILE * fp;
    char path[MAXBUFFER];  /* Path to the file */
    char tempbuff[MAXBUFFER]; /* A temporary buffer */
    int length;
    int i;

    /* 
     * Upload the file into tempbuff 
     */

    if (hstate->maindirvalid == 0) 
    {
        strcpy(replymsg, "Upload aborted:  the host's main directory is not yet chosen");
        return;
    }

    /* Create a path to the file and then open it */
    strcpy(path,"");
    strcat(path, hstate->maindir);
    strcat(path, "/");
    strcat(path, fname);

    fp = fopen(path,"rb"); 
    if (fp == NULL) 
    { /* file didn't open */
        strcpy(replymsg, "Upload aborted: the file didn't open");
        return;
    }

    length = fread(tempbuff, 1, PAYLOAD_LENGTH+1, fp);

    if (length==0) 
    {
        strcpy(replymsg, "Upload aborted: error in reading the file");
        return;
    }

    else if (length > PAYLOAD_LENGTH)
    {
        strcpy(replymsg, "Upload aborted: file is too big");
        return;
    }

    tempbuff[length] = '\0';
    hstate->sendDataBuff.valid = 1;
    hstate->sendDataBuff.length = length;

    /* Fill in send packet buffer */

    hstate->sendPacketBuff.valid=1;
    hstate->sendPacketBuff.length=length;

    for (i=0; i<length; i++)
    { /* Store tempbuff in payload of packet buffer */
        hstate->sendPacketBuff.payload[i] = tempbuff[i];
    }

    /* Message to the manager */
    strcpy(replymsg, "Upload successful");

    fclose(fp);
}

/* 
 * Initialize receive packet buffer 
 */ 

void hostInitRcvPacketBuff(packetBuffer * packetbuff)
{
    packetbuff->valid = 0;
    packetbuff->new = 0;
    packetbuff->end = 0;
    packetbuff->start = 0;
}

/*
 * Download the payload of the packet buffer into a 
 * file in the main directory
 */

void hostDownloadPacket(hostState * hstate, char fname[], char replymsg[]) 
{
    char c;
    FILE * fp;
    char path[MAXBUFFER];  /* Path to the file */

    /* Create a path to the file and then open it */

    if (hstate->rcvPacketBuff.valid == 0) 
    {
        strcpy(replymsg, "Download aborted: the receive packet buffer is empty");
        return;
    }

    if (hstate->maindirvalid == 0) 
    {
        strcpy(replymsg, "Download aborted: the host's main directory is not yet chosen");
        return;
    }

    strcpy(path,"");
    strcat(path, hstate->maindir);
    strcat(path, "/");
    strcat(path, fname);
    printf("host:  path to the file: %s\n",path);
    fp = fopen(path,"wb"); 

    /* Download the packet buffer payload into the file */
    if (hstate->rcvPacketBuff.new == 1) 
    {
        fwrite(hstate->rcvPacketBuff.payload,1,hstate->rcvPacketBuff.length,fp);
        hstate->rcvPacketBuff.new=0;
    }

    /* Message sent to the manager */
    strcpy(replymsg, "Download successful");
    fclose(fp);
} 

/* 
 * Clear the receive packet buffer
 */
void hostClearRcvFlg(hostState * hstate, char replymsg[])
{
    hstate->rcvflag = 0;
    hstate->rcvPacketBuff.valid = 0;
    hstate->rcvPacketBuff.new = 0;

    /* Message to the manager */
    strcpy(replymsg, "Host's packet received flag is cleared");
}

/* 
 * Change main directory of the host 
 */

void hostSetMainDir(hostState * hstate, char dirname[], char replymsg[])
{
    DIR * dir = opendir(dirname);

    if ( dir == NULL )
    {
        strcpy(replymsg, "The provided directory name does not exist.");
        closedir(dir);
    }

    else
    {
        strcpy(hstate->maindir, dirname);
        hstate->maindirvalid = 1;

        /* Message to the manager */
        strcpy(replymsg, "Host's main directory name is changed");
    }
}

/*
 * Get the host's state  
 * - host's physical ID
 * - host's main directory
 * - host's network address
 * - host's neighbor address
 * - host's receive flag
 * and create a message that has all this information to be sent
 * to the manager
 */
void hostGetHostState(hostState * hstate, managerLink * manLink, char replymsg[])
{
    char word[1000];
    char empty[7] = "Empty";

    /* Create reply message */

    replymsg[0] = '\0';  /* Clear reply message */

    int2Ascii(word, hstate->physid);
    appendWithSpace(replymsg, word);

    if (hstate->maindirvalid==0) appendWithSpace(replymsg, empty);
    else appendWithSpace(replymsg, hstate->maindir);

    if (hstate->netaddr == EMPTY_ADDR) appendWithSpace(replymsg, empty);

    else
    {
        int2Ascii(word, hstate->netaddr);
        appendWithSpace(replymsg, word);
    }

    if (hstate->nbraddr == EMPTY_ADDR) appendWithSpace(replymsg, empty);

    else
    {
        int2Ascii(word, hstate->nbraddr);
        appendWithSpace(replymsg, word);
    }

    int2Ascii(word, hstate->rcvPacketBuff.new);
    appendWithSpace(replymsg, word);

}

/* 
 * Initialize the state of the host 
 */
void hostInitState(hostState * hstate, int physid)
{
    hstate->physid = physid;
    hstate->maindirvalid = 0; /* main directory name is empty*/
    hstate->netaddr = physid; /* default address */  
    hstate->nbraddr = EMPTY_ADDR;  
    hstate->rcvPacketBuff.valid = 0;
    hstate->rcvPacketBuff.new = 0;
}



void hostInitDataBuffer(dataBuffer * buff) 
{
    buff->pos = 0;
    buff->busy = 0;
    buff->valid = 0;
    buff->length = 0;
    buff->start = 0;
    buff->end = 0;
}



void hostInitTransmit(hostState * hstate, char word[], char replymsg[]) 
{
    if (hstate->sendDataBuff.busy == 1) 
    {
        strcpy(replymsg, "Already busy trasmitting.");
        return;
    }
    if (hstate->sendDataBuff.valid == 0)
    {
        strcpy(replymsg, "Invalid entry.");
        return;
    }

    char dest[1000];
    int  dstaddr;

    findWord(dest, word, 2);
    dstaddr = ascii2Int(dest);
    hstate->sendDataBuff.dstaddr = dstaddr;
    hstate->sendDataBuff.busy = 1;
    hstate->sendDataBuff.pos = 0;
    strcpy(replymsg, "Transmiting...");
}
