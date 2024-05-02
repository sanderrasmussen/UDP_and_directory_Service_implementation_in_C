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

#define MAX_PACKET_SIZE 1024
#define ALLOC_ERROR -10;

//./d1_test_client localhost 1000

//helping methods
uint16_t calculate_checksum(struct D1Header* header, char* data, size_t data_size) {
    //printf("%d header size in checksum\n", sizeof(D1Header)); //for debugging
    //printf("%d message size \n", data_size);

    //i have one odd and one even byte, i will xor every byte taking turns on these to variables
    uint8_t odd = 0;
    uint8_t even = 0;
    
    //creating buffer and merging header and data together into it
    char *buffer = (char* )calloc(1, sizeof(D1Header) + data_size ); 
    if (buffer == NULL) {
        fprintf(stderr, "Allocation of memory failed \n");
        free(buffer);
        return ALLOC_ERROR;
    }
    memcpy(buffer, header, sizeof(D1Header));
    memcpy(buffer + sizeof(D1Header), data, data_size); 

    int totalPacketSize = sizeof(header) + data_size;//how many iterations i will need
    
    for (int i = 0; i< totalPacketSize; i++ ){
        if (i%2 == 0){ //taking turns on xoring on odd and even
            odd ^= buffer[i];
        }
        else{
            even ^= buffer[i];
        }
        
    }

    free(buffer);
    return (odd << 8 ) | even; //setting the first 8 bits to odd and the last to even
}


D1Peer* d1_create_client( )
{
    int socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //creating socket using UDP
    if (socketfd < 0) {
        printf("Could not make UDP socket");
        return NULL;
    }
    //allocate memory
    D1Peer* peer = (D1Peer*)calloc(1, (sizeof(D1Peer)));

    if (peer == NULL){
        fprintf(stderr,"Could not allocate memory for D1Peer struct \n");
        close(socketfd);
        free(peer);
        return ALLOC_ERROR;
    }
    
    peer->socket =socketfd;
    peer->next_seqno = 0;  

    return peer;
}

D1Peer* d1_delete( D1Peer* peer )
{
    if (peer != NULL){
        close(peer->socket); //closing the socket if peer is not NULL
        free(peer); //freeing pointer 
        peer = NULL;
    }

    return NULL;
}

int d1_get_peer_info(struct D1Peer* peer, const char* peername, uint16_t server_port) {
    //discover the address information for the server we want to contact.
    struct addrinfo hints, *res;

    //Initialize hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      
    hints.ai_socktype = SOCK_DGRAM; 

    //resolve the servers name to a list of socket addresses
    if (getaddrinfo(peername, NULL, &hints, &res) != 0) {
        printf("Failed to resolve the name for %s \n ", peername);
        return 0; 
    }

    //copy the first resolved address into the peers address structure
    if (res->ai_family == AF_INET) {
        struct sockaddr_in* addr_in = (struct sockaddr_in*)res->ai_addr;
        peer->addr.sin_family = AF_INET;
        peer->addr.sin_port = htons(server_port);
        peer->addr.sin_addr = addr_in->sin_addr;
    } else {
        printf(stderr, "No compatible address found for %s \n ", peername);
        freeaddrinfo(res);
        return 0; 
    }

    //free the memory allocated by getaddrinfo()
    freeaddrinfo(res);

    return 1; 
}

int d1_recv_data( struct D1Peer* peer, char* buffer, size_t sz )
{
    /** Call this to wait for a single packet from the peer. The function checks if the
 *  size indicated in the header is correct and if the checksum is correct.
 *  In case of success, remove the D1 header and store only the payload in the buffer
 *  that is passed a parameter. The size sz is the size of the buffer provided by the
 *  caller.
 *  If size or checksum are incorrect, an ACK with the opposite value is sent (this should
 *  trigger the sender to retransmit).
 *  In other cases, an error message is printed and a negative number is returned to the
 *  calling function.
 *  Returns the number of bytes received as payload in case of success, and a negative value
 *  in case of error. Empty data packets are allowed, and a return value of 0 is therefore
 *  valid. 
 */
    char* packet = (char*)malloc(sz); 
    if (packet == NULL) {
        fprintf(stderr, "Allocation of memory failed \n");
        free(packet);
        return ALLOC_ERROR;
    }
    int received_bytes = recv(peer->socket, packet, sz, 0); //storing data in packet buffer

    if (received_bytes < 0) {
        printf("No bytes received");
        return -1;
    }

    struct D1Header *header = (D1Header *)packet; //casting packet buffer to header
    
    uint16_t packetChecksum = ntohs(header->checksum); //making checksum into host order and storing it in a variable before whiping it
    header->checksum = 0; //setting checksum to 0 so that i can calculate it againg with my own function to verify the packets integrity

    uint16_t checksum = calculate_checksum(header, packet+ sizeof(D1Header), received_bytes-sizeof(D1Header));
    // converting fields to host order
    header->size = ntohl(header->size); 
    header->flags = ntohs(header->flags);
    
    if (packetChecksum==checksum){ 
        printf("riktig checksum \n");
         
        if (header->size == (uint32_t) received_bytes){
            printf("correct size on packet \n ");
            //remove header and store in buffer
            memcpy(buffer, packet+ sizeof(D1Header), received_bytes-sizeof(D1Header) );
            d1_send_ack(peer, (header->flags &SEQNO));//sending the same ack value as is stored in received packets header
            
        }
        else{ 
            printf("Feil size på pakke \n");
            d1_send_ack(peer, !(header->flags &SEQNO));//send ack with oposite ackno
        }
    }
    else{
        printf("\n feil checksum \n");
        d1_send_ack(peer, !(header->flags &SEQNO));//sending oposite ackno 
    }
    free(packet);
    
    return received_bytes;
}


void d1_send_ack( struct D1Peer* peer, int seqno )
{  
    struct D1Header *header = (D1Header *) calloc(1, sizeof(D1Header));
    if (header == NULL) {
        fprintf(stderr, "Allocation of memory failed \n");
        free(header); 
        return;  //this is a void funtion so just returning
    }
    header->flags |= FLAG_ACK;
    if (seqno > 0){ //i had problems with seqno being either 128 or 0, these are still two values(either positive or negativ) so the ACKNO flag is set if seqno is bigger than 0
        header->flags |= ACKNO;
    }

    header->size = htonl(sizeof(D1Header));
    header->flags = htons(header->flags);

    uint16_t checksum = calculate_checksum(header, NULL, 0);//calculating only header checksum
    header->checksum = htons(checksum);
    int sent_bytes = sendto(peer->socket, header, sizeof(D1Header),0 ,(struct sockaddr *)&peer->addr, sizeof(peer->addr));
     
    if (sent_bytes<0){
        printf("error");
    }
    free(header);
}


int d1_wait_ack( D1Peer* peer, char* buffer, size_t sz )
{
    //make a new buffer for receiving ack
    struct D1Header *recvedData = (D1Header*)malloc(sizeof(D1Header));
    if (recvedData == NULL) {
        fprintf(stderr, "Allocation of memory failed \n");
        free(recvedData);
        return ALLOC_ERROR;
    }
    uint8_t recvedBytes = recv(peer->socket,  recvedData, sizeof(D1Header),0); 

    if (recvedBytes<0){
        printf("Error");
        return -1;
    }

    D1Header* receivedHeader = (D1Header*)recvedData;//casting to header
    receivedHeader->flags = ntohs(receivedHeader->flags);//converting all fields to host order
    receivedHeader->size = ntohl(receivedHeader->size);
    receivedHeader->checksum = ntohs(receivedHeader->checksum);
    
    if (receivedHeader->flags & FLAG_DATA){//checking if packet is wrong type(data packet)
        printf("This is not an ack packet /n");
        
    }
    if (receivedHeader->flags & FLAG_ACK){ //checking if ack packet

        int ack_seqno =receivedHeader->flags & ACKNO;

        if (peer->next_seqno == ack_seqno){ //checking if seqno is matching

            peer->next_seqno = !peer->next_seqno; // flipping the bit from 0 to 1, or 0 to 1
            printf("Correct seqno: %d \n", ack_seqno);
            free(recvedData);

            return 0;
        }
        else {
            printf("Wrong seqno \n ");
            d1_send_data(peer, buffer, sz);//resending packet with message in buffer
        }
    }
    else{
        printf("Did not find Ack packet \n");
        d1_send_data(peer, buffer, sz);
    }
    free(recvedData);

    return 0;
}

int d1_send_data( D1Peer* peer, char* buffer, size_t sz )
{
    if (sz > MAX_PACKET_SIZE){ //checking if buffer exeeds packet size
        printf("Buffer exceed packet size \n ");
        return -1; //ending if buffer to big
    }

    D1Header *header = (D1Header *)calloc(1,sizeof(D1Header));
    if (header == NULL) {
        fprintf(stderr, "Allocation of memory failed \n");
        free(header);
        return ALLOC_ERROR;
    }
    uint32_t totalPcketSize =  sizeof(D1Header)+sz;

    header->flags |= FLAG_DATA; //make it data packet
    if (peer->next_seqno==1){
        header->flags |= SEQNO; //if seqno = 1 set SEQNO flag
    }
    
    header->size = htonl(totalPcketSize); //converting to network order
    header->flags = htons(header->flags);

    uint16_t checksum =calculate_checksum(header, buffer, sz);
    
    header->checksum = htons(checksum); 

    char *data = (char *)calloc(1, totalPcketSize); 
    if (data == NULL) {
        fprintf(stderr, "Allocation of memory failed \n");
        free(data);
        return ALLOC_ERROR;
    }
    //copying header and buffer into data buffer to be sendt
    memcpy(data, header, sizeof(D1Header));
    memcpy(data+sizeof(D1Header), buffer, sz);
    printf("Sending package \n");
    int sent_bytes = sendto(peer->socket, data, totalPcketSize,0 ,(struct sockaddr *)&peer->addr, sizeof(peer->addr));
    d1_wait_ack(peer, buffer,sz); 

    free(header);
    free(data);

    return sent_bytes;
}



