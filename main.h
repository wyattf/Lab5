
#define PAYLOAD_LENGTH 200 /* Maximum payload size */

typedef struct { /* Packet buffer */
   int srcaddr;  /* Source address */
   int dstaddr;  /* Destination addres */
   int length;   /* Length of packet */
   char payload[PAYLOAD_LENGTH + 1];  /* Payload section */
   int valid;   /* Indicates if the contents is valid */ 
   int new;     /* Indicates if the contents has been downloaded */
   int start;
   int end;
} packetBuffer;



//buffer for data, used to associate multiple packets as a single set of data
typedef struct {
    char data[2000]; //buffered data
    int length; //total size of buffered data
    int dstaddr;//destination address
    int srcaddr;//source address
    int pos;    //current position in buffer
    int valid;  //valid flag
    int busy;   //busy flag
    int start;  //flag that a packet is the start packet
    int end;    //flag that a packet is the end packet 
} dataBuffer;
   


