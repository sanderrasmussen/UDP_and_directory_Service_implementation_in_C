/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "d2_lookup.h"

D2Client* d2_client_create( const char* server_name, uint16_t server_port )
{
    /* implement this */
    /* Create the information that is required to use the server with the given
 * name and port. Use D1 functions for this. Store the relevant information
 * in the D2Client structure that is allocated on the heap.
 * Returns NULL in case of failure.
 */

   
    D2Client *client = (D2Client *)malloc(sizeof(D2Client));
    if (client==NULL){
        return NULL;
    }
     //creating d1 client
    D1Peer *d1peer = d1_create_client();
    d1_get_peer_info(d1peer, server_name, server_port);
    
    client->peer = d1peer;
    
    
    return client;
    
}

D2Client* d2_client_delete( D2Client* client )
{
    /* Delete the information and state required to communicate with the server.
 * Returns always NULL.
 */

    /* implement this */
     if (client != NULL){
        close(client->peer->socket); //closing the socket if peer is not NULL
        free(client->peer); //freeing pointer 
        free(client);
        client = NULL;
    }

    return NULL;
}

int d2_send_request( D2Client* client, uint32_t id )
{
/* Send a PacketRequest with the given id to the server identified by client.
 * The parameter id is given in host byte order.
 * Returns a value <= 0 in case of failure and positive value otherwise.
 */
    /* implement this */
    //type and id must be in network byte order
    //PacketHeader *header = (PacketHeader *) calloc(1, sizeof(PacketHeader));

    PacketRequest *request = (PacketRequest *) calloc(1, sizeof(PacketRequest));
    request->type = TYPE_REQUEST;
    request->type = htons(request->type);
    request->id = id;
    request->id = htonl(request->id);
    //int sent_bytes = sendto(client->peer->socket, request, sizeof(PacketRequest),0 ,(struct sockaddr *)&client->peer->addr, sizeof(client->peer->addr));
    int sent_bytes = d1_send_data(client->peer,request, sizeof(PacketRequest));
  
    if (sent_bytes<=0){
        return -1;
    }
    return 1;
}

int d2_recv_response_size( D2Client* client )
{

/* Wait for a PacketResponseSize packet from the server.
 *  
 * Returns the number of NetNodes that will follow in several PacketReponse
 * packet in case of success. Note that this is _not_ the number of
 * PacketResponse packets that will follow because of PacketReponse can
 * contain several NetNodes.
 *
 * Returns a value <= 0 in case of failure.
 */
    /* implement this */
    char* buffer= (char*)malloc(sizeof(PacketResponseSize));
    int recved_bytes = d1_recv_data(client->peer, buffer, 1024);
    if (recved_bytes<=0){
        return -1;
    }

    //PacketResponseSize* response = (PacketResponseSize *)calloc(1, sizeof(PacketResponseSize));
    PacketResponseSize* response = buffer;
   
    int size =ntohs(response->size);
    return size;
}


int d2_recv_response( D2Client* client, char* buffer, size_t sz )
{
    /* implement this */
    
/* Wait for a PacketResponse packet from the server.
 * The caller must provide a buffer where up to 1024 bytes can be stored.
 *
 * Returns the number of bytes received in case of success.
 * The PacketResponse header and all NetNodes included in the packet
 * are stored in the buffer. 
 *  
 * Returns a value <= 0 in case of failure.
 */

    int recved_bytes = d1_recv_data(client->peer, buffer, sz);  
    PacketHeader *header = (PacketHeader *)buffer;
    //now we can inspect which packet we got

/* PacketResponse packets are sent from the server to the client.
 * They contain the type field, which can be TYPE_RESPONSE or
 * TYPE_LAST_RESPONSE. If the type field is TYPE_RESPONSE, the
 * client can expect more PacketResponse packets, if it is
 * TYPE_LAST_RESPONSE, the server will not send more PacketResponse
 * packets.
 *
 * These packets contain the payload_size field that indicates the number of
 * bytes contained in this message.
 *
 * One or more abbreviated (!!!) NetNode structures follow the header,
 * but never more than 5. It is OK to assume that only TYPE_LAST_RESPONSE
 * packets have fewer than 5 NetNode structures following the header.
 *
 * NetNode structures are abbreviated, meaning that child_id[x] fields
 * are not transmitted over the network if x>=num_children.
 *
 * type, payload_size, and all fields of the NetNode structures are in
 * network byte order.
 *
 * PacketResponse is always PacketResponse.payload bytes long.
 * 
 * struct PacketResponse
{
    uint16_t type;
    uint16_t payload_size;
};
typedef struct PacketResponse     PacketResponse;
  
 */



}

LocalTreeStore* d2_alloc_local_tree( int num_nodes )
{   /* Allocate one more more local structures that are suitable to receive num_nodes
 * tree nodes containing all the information from a NetNode.
 * It is expected that the memory for these structures is allocated dynamically.
 *
 * Change the LocalTreeStore to suit your need.
 *
 * Returns NULL in case of failure.
 */
    NetNode *tree = (NetNode *)malloc(sizeof(struct NetNode)*num_nodes);
    if (tree==NULL){
        return NULL;
    }
    
    return tree;
}

void  d2_free_local_tree( LocalTreeStore* nodes )
{   /* Release all memory that has been allocated for the local tree structures.
 */
    /* implement this */
}

int d2_add_to_local_tree( LocalTreeStore* nodes_out, int node_idx, char* buffer, int buflen )
{
    /* Take the buffer that has been filled by d2_recv_response, and the buflen,
 * which is the number of bytes containing all the NetNodes stored in the buffer,
 * and add one node to the local tree for every NetNode in the buffer.
 *
 * One buffer can contain up to 5 NetNodes.
 *
 * node_idx is expected to be 0-based index of the tree nodes that are already
 * in the LocalTreeStore. The return value is node_idx increased by the additional
 * number of nodes that have been added to the LocalTreeStore.
 *
 * Tip: this may be helpful for array-based LocalTreeStores.
 *
 * Returns negative values in case of failure.
 */
    /* implement this */
    return 0;
}

void d2_print_tree( LocalTreeStore* nodes_out )
{

/* Print the current node in the LocalTreeStore to standard output
 * (e.g. using printf) in the following manner:
 *
id 0 value 72342 children 2
-- id 1 value 829347 children 1
---- id 2 value 298347 children 0
-- id 3 value 190238 children 2
---- id 4 value 2138 children 1
------ id 5 value 901293 children 0
---- id 6 value 128723 children 0
 *
 * In this printout, the tree's node 0 has two children, 1 and 3.
 * Node 1 has one child, 2.
 * Node 3 has two children, 4 and 6.
 * Node 4 has one child, 5.
 */
    /* implement this */
}

