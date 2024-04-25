/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "d1_udp.h"
//./d1_test_client localhost 1000

//helping methods
 /* These are the possible values of D1Header.flags in host byte order.
 * When you send D1Headers over the network, they must be send in network
* byte order.
 * 1 is set 0 is unset
* #define FLAG_DATA       (1 << 15)  sets 15 byte to 1, is a datapacket
* #define FLAG_ACK        (1 << 8) if this is set, this has to be ack packet
* #define SEQNO           (1 << 7) if flag_data is not set and seqNO is set, this must be a connect packet
* #define ACKNO           (1 << 0) if Flag_ack is null and Ackno is 1 this must be a disconnect packet
*/
struct packet{ //this is the packet that will be sent beetween peers
    struct D1Header header;
    char* buffer; //in order to keep packet equal or under 1024 byte
};

/*
uint16_t calculate_checksum(struct D1Header* header, char* data, size_t data_size) {
    printf("%d header size in checksum\n", sizeof(D1Header));
    printf("%d message size \n", data_size);
    uint16_t checksum = 0;
    uint8_t odd = 0;
    uint8_t even = 0;
    //size_t bufferWithPaddingSize = 
    uint8_t *buffer = (char* )calloc(1, sizeof(D1Header) + data_size);
    if (data_size%2==1){
        free(buffer);
        uint8_t *buffer = (char* )calloc(1, sizeof(D1Header) + data_size +1);
    }
    
    
    memcpy(buffer, header, sizeof(D1Header));
    memcpy(buffer + sizeof(D1Header), data, data_size);// --
    printf("data : %.*s \n",data_size,data);


    int totalPacketSize = sizeof(header) + data_size;
    if (totalPacketSize%2==1){
        totalPacketSize+=1;
    }
    for (int i = 0; i< (totalPacketSize); i++ ){
        if (i%2 == 0){
            even ^= buffer[i];
            printf("even");
        }
        else{
            odd ^= buffer[i];
            printf("odd");
        }
    }
    checksum = (odd << 8 ) | even;
    free(buffer);
    return checksum;
}
*/
uint16_t calculate_checksum(struct D1Header* header, char* data, size_t data_size) {
    //printf("%d header size in checksum\n", sizeof(D1Header));
    //printf("%d message size \n", data_size);
    
    uint8_t odd = 0;
    uint8_t even = 0;
  
    char *buffer = (char* )calloc(1, sizeof(D1Header) + data_size );
    
    memcpy(buffer, header, sizeof(D1Header));
    memcpy(buffer + sizeof(D1Header), data, data_size);// --

    int totalPacketSize = sizeof(header) + data_size;
    
    for (int i = 0; i< totalPacketSize; i++ ){
        if (i%2 == 0){
            odd ^= buffer[i];
            //printf("even");
        }
        else{
            even ^= buffer[i];
            //printf("odd");
        }
        
    }
    /*if (data_size%2!=0){
        even ^=0;
    }*/

    free(buffer);
    return (odd << 8 ) | even;
}


D1Peer* d1_create_client( )
{
    /* implement this */
    /** Create a UDP socket that is not bound to any port yet. This is used as a
 *  client port.
 *  Returns the pointer to a structure on the heap in case of success or NULL
 *  in case of failure.
 */
    int socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd < 0) {
        perror("Could not make UDP socket");
        return NULL;
    }

    //allocate memory for d1Peer struct
    D1Peer* peer = (D1Peer*)calloc(1, (sizeof(D1Peer)));

    if (peer == NULL){
        perror("Could not allocate memory for D1Peer struct");
        close(socketfd);
        return NULL;
    }

    peer->socket =socketfd;
    peer->next_seqno = 0;
    
    return peer;
}

D1Peer* d1_delete( D1Peer* peer )
{
    /* implement this */
    /** Take a pointer to a D1Peer struct. If it is non-NULL, close the socket
 *  and free the memory of the server struct.
 *  The return value is always NULL.
 */
    if (peer != NULL){
        close(peer->socket); //closing the socket if peer is not NULL
        free(peer); //freeing pointer 
        peer = NULL;
    }

    return NULL;
}

int d1_get_peer_info(struct D1Peer* peer, const char* peername, uint16_t server_port) {
    // Discover the address information for the server we want to contact.
    struct addrinfo hints, *res;

    // Initialize hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP

    // Resolve the server's name to a list of socket addresses
    if (getaddrinfo(peername, NULL, &hints, &res) != 0) {
        fprintf(stderr, "Failed to resolve the name for %s\n", peername);
        return 0; // Error
    }

    // Copy the first resolved address into the peer's address structure
    if (res->ai_family == AF_INET) {
        struct sockaddr_in* addr_in = (struct sockaddr_in*)res->ai_addr;
        peer->addr.sin_family = AF_INET;
        peer->addr.sin_port = htons(server_port);
        peer->addr.sin_addr = addr_in->sin_addr;
    } else {
        fprintf(stderr, "No compatible address found for %s\n", peername);
        freeaddrinfo(res);
        return 0; // Error
    }

    // Free the memory allocated by getaddrinfo()
    freeaddrinfo(res);

    return 1; // Success
}

int d1_recv_data( struct D1Peer* peer, char* buffer, size_t sz )
{

    int received_bytes = recv(peer->socket, buffer, sizeof(D1Header) + sz, 0);
    if (received_bytes < 0) {
        perror("Feil ved mottak av data");
        return -1;
    }

    // Data er mottatt og lagret i bufferet
    return received_bytes;

}



void d1_send_ack( struct D1Peer* peer, int seqno )
{
    /* implement this */
    /** Send an ACK for the given sequence number.
     * 
     * I have to send an ACK package 
 */

}


int d1_wait_ack( D1Peer* peer, char* buffer, size_t sz )
{
/* Called by d1_send_data when it has successfully sent its packet. This functions waits for
 *  an ACK from the peer.
 *  If the sequence number of the ACK matches the next_seqno value in the D1Peer stucture,
 *  this function changes the next_seqno in the D1Peer data structure
 *  (0->1 or 1->0) and returns to the caller.
 *  If the sequence number does not match, d1_send_data followed by d1_wait_ack is called
 *  again.
 *  
 *  This function is only meant to be called by d1_send_data. You don't have to implement it.
 */
    while(1){
        struct D1Header *recvedData = (D1Header*)malloc(sizeof(D1Header));
        //0 terminating buffer
        //memset(recvedData, "\0",sizeof(D1Header)+ sz+1 );
printf("Før recvfrom\n");
uint8_t recvedBytes = recv(peer->socket,  recvedData, sizeof(D1Header),0);
printf("Etter recvfrom\n");
        if (recvedBytes<0){
            printf("Error");
            return -1;
        }
        printf("received %d \n", recvedBytes);
printf("Innhold av mottatt data: %s\n", recvedData);

        D1Header* receivedHeader = (D1Header*)recvedData;
        receivedHeader->flags = ntohs(receivedHeader->flags);
        receivedHeader->size = ntohl(receivedHeader->size);
        receivedHeader->checksum = ntohs(receivedHeader->checksum);
        
        if (receivedHeader->flags & FLAG_DATA){
            printf("DEtte ER EN DATA PACKET IKKE ACK /n");
            return -1;
        }
        if (receivedHeader->flags & FLAG_ACK){
            printf("ACK packet funnet \n");
            uint16_t ack_seqno = (receivedHeader->flags & ACKNO) ? 1 : 0;
            if (peer->next_seqno == ack_seqno){ //dersom peer sin seqno == flag seqno
                peer->next_seqno = !peer->next_seqno; // 0->1 or 1->0
                return 0;
            }
            else {
                //d1_send_data(peer, buffer, sz);
                return 0;
            }


        }
        else{
            printf("fant ikke ACK pakke \n");
            //d1_send_data(peer, buffer, sz);
            return 0;
        }
    }
    

    return 0;
}




int d1_send_data( D1Peer* peer, char* buffer, size_t sz )
{
    /* implement this */
    /** If the buffer does not exceed the packet size, send add the D1 header and send
 *  it to the peer.
 */

   printf("sending \n");


    
    //D1Header *header = (D1Header *)calloc(1, sizeof(D1Header));
    D1Header *header = (D1Header *)calloc(1,sizeof(D1Header));

   

    uint32_t totalPcketSize =  sizeof(D1Header);


    header->flags = FLAG_DATA;
    header->flags |= peer->next_seqno << 7;
    header->size = htonl(totalPcketSize);
    header->flags = htons(header->flags);

    // Beregner sjekksum
    //printf("%d header size \n", sizeof(D1Header));
    //printf("%d message size \n", sz);

    uint16_t checksum =calculate_checksum(header, buffer, sz);
  
    //printf("checksum in host order: %d \n", header->checksum );
    // Kopierer størrelsen til headeren
    
    header->checksum = htons(checksum);
    //printf("checksum in network order: %d \n", header->checksum );

    char *data = (char *)calloc(1, totalPcketSize); 
  
    memcpy(data, header, sizeof(D1Header));
    memcpy(data+sizeof(D1Header), buffer, sz);

    int sent_bytes = sendto(peer->socket, data, totalPcketSize,0 ,(struct sockaddr *)&peer->addr, sizeof(peer->addr));
    d1_wait_ack(peer, buffer,sz);
    //printf("sent_bytes %d \n", sent_bytes);

    free(header);
    free(data);


    printf("ack is sent");

    return sent_bytes;
}



