# Network
Moss Network slightly modified version of ENet6 v6.1.2. ENet6 is a reliable networking library built on top of Enet, enabling both connection-oriented and connectionless communication with low overhead supporting both IPv4 and IPv6. [Enet6 v6.1.2](https://github.com/SirLynix/enet6)

Network supports ```Windows```, ```Linux```, ```MacOS```, ```FreeBSD```, ```IOS``` and ```Android```.

- [Overview](/docs/Network.md#overview) 
    - [Client/Server](/docs/#), [RPC](/docs/#)
    - [HTTP/Rest](/docs/#)

## Macros
```cpp
#define ENET_SOCKETSET_EMPTY(sockset)
#define ENET_SOCKETSET_ADD(sockset, socket)
#define ENET_SOCKETSET_REMOVE(sockset, socket)
#define ENET_SOCKETSET_CHECK(sockset, socket)
#define ENET_HOST_TO_NET_16(value)
#define ENET_HOST_TO_NET_32(value)
```
## Enums
```cpp
enum ENetAddressType { 
    ENET_ADDRESS_TYPE_ANY = 0, 
    ENET_ADDRESS_TYPE_IPV4 = 1, 
    ENET_ADDRESS_TYPE_IPV6 = 2 
};
enum ENetSocketType { 
    ENET_SOCKET_TYPE_STREAM = 1, 
    ENET_SOCKET_TYPE_DATAGRAM = 2
};
enum ENetSocketShutdown { 
    ENET_SOCKET_SHUTDOWN_READ = 0, 
    ENET_SOCKET_SHUTDOWN_WRITE = 1, 
    ENET_SOCKET_SHUTDOWN_READ_WRITE = 2 
};
enum ENetSocketOption {
    ENET_SOCKOPT_NONBLOCK  = 1,
    ENET_SOCKOPT_BROADCAST = 2,
    ENET_SOCKOPT_RCVBUF    = 3,
    ENET_SOCKOPT_SNDBUF    = 4,
    ENET_SOCKOPT_REUSEADDR = 5,
    ENET_SOCKOPT_RCVTIMEO  = 6,
    ENET_SOCKOPT_SNDTIMEO  = 7,
    ENET_SOCKOPT_ERROR     = 8,
    ENET_SOCKOPT_NODELAY   = 9,
    ENET_SOCKOPT_TTL       = 10,
    ENET_SOCKOPT_IPV6ONLY  = 11
};
enum ENetSocketWait {
    ENET_SOCKET_WAIT_NONE = 0, 
    ENET_SOCKET_WAIT_SEND = (1 << 0), 
    ENET_SOCKET_WAIT_RECEIVE = (1 << 1), 
    ENET_SOCKET_WAIT_INTERRUPT = (1 << 2)
};
enum ENetProtocolCommand {
    ENET_PROTOCOL_COMMAND_NONE               = 0,
    ENET_PROTOCOL_COMMAND_ACKNOWLEDGE        = 1,
    ENET_PROTOCOL_COMMAND_CONNECT            = 2,
    ENET_PROTOCOL_COMMAND_VERIFY_CONNECT     = 3,
    ENET_PROTOCOL_COMMAND_DISCONNECT         = 4,
    ENET_PROTOCOL_COMMAND_PING               = 5,
    ENET_PROTOCOL_COMMAND_SEND_RELIABLE      = 6,
    ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE    = 7,
    ENET_PROTOCOL_COMMAND_SEND_FRAGMENT      = 8,
    ENET_PROTOCOL_COMMAND_SEND_UNSEQUENCED   = 9,
    ENET_PROTOCOL_COMMAND_BANDWIDTH_LIMIT    = 10,
    ENET_PROTOCOL_COMMAND_THROTTLE_CONFIGURE = 11,
    ENET_PROTOCOL_COMMAND_SEND_UNRELIABLE_FRAGMENT = 12,
    ENET_PROTOCOL_COMMAND_COUNT              = 13,
    ENET_PROTOCOL_COMMAND_MASK               = 0x0F
};
enum ENetProtocolFlag {
    ENET_PROTOCOL_COMMAND_FLAG_ACKNOWLEDGE = (1 << 7),
    ENET_PROTOCOL_COMMAND_FLAG_UNSEQUENCED = (1 << 6),
    ENET_PROTOCOL_HEADER_FLAG_COMPRESSED = (1 << 14),
    ENET_PROTOCOL_HEADER_FLAG_SENT_TIME  = (1 << 15),
    ENET_PROTOCOL_HEADER_FLAG_MASK       = ENET_PROTOCOL_HEADER_FLAG_COMPRESSED | ENET_PROTOCOL_HEADER_FLAG_SENT_TIME,
    ENET_PROTOCOL_HEADER_SESSION_MASK    = (3 << 12),
    ENET_PROTOCOL_HEADER_SESSION_SHIFT   = 12
};
enum ENetEventType {
    ENET_EVENT_TYPE_NONE       = 0,  
    ENET_EVENT_TYPE_CONNECT    = 1,  
    ENET_EVENT_TYPE_DISCONNECT = 2,  
    ENET_EVENT_TYPE_RECEIVE    = 3
};

enum ENetPacketFlag {
    ENET_PACKET_FLAG_RELIABLE    = (1 << 0),         // Packet must be received by the target peer and resend attempts should be made until the packet is delivered */
    ENET_PACKET_FLAG_UNSEQUENCED = (1 << 1),         // Packet will not be sequenced with other packets
    ENET_PACKET_FLAG_NO_ALLOCATE = (1 << 2),         // Packet will not allocate data, and user must supply it instead */
    ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT = (1 << 3), // Packet will be fragmented using unreliable (instead of reliable) sends if it exceeds the MTU.
    ENET_PACKET_FLAG_SENT = (1<<8)                   // whether the packet has been sent from all queues it has been entered into.
};

enum ENetPeerState {
    ENET_PEER_STATE_DISCONNECTED                = 0,
    ENET_PEER_STATE_CONNECTING                  = 1,
    ENET_PEER_STATE_ACKNOWLEDGING_CONNECT       = 2,
    ENET_PEER_STATE_CONNECTION_PENDING          = 3,
    ENET_PEER_STATE_CONNECTION_SUCCEEDED        = 4,
    ENET_PEER_STATE_CONNECTED                   = 5,
    ENET_PEER_STATE_DISCONNECT_LATER            = 6,
    ENET_PEER_STATE_DISCONNECTING               = 7,
    ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT    = 8,
    ENET_PEER_STATE_ZOMBIE                      = 9 
};
enum ENetPeerFlag {
    ENET_PEER_FLAG_NEEDS_DISPATCH   = (1 << 0),
    ENET_PEER_FLAG_CONTINUE_SENDING = (1 << 1)
};
```

## Structs
```cpp
struct ENetSocket;
struct ENetHost;
struct ENetEvent;
struct ENetAddress;
struct ENetPeer;
struct ENetCallbacks;
struct ENetBuffer;
struct ENetListNode;
struct ENetList;
struct ENetProtocolHeader;
struct ENetProtocolCommandHeader;
struct ENetProtocolAcknowledge;
struct ENetProtocolConnect;
struct ENetProtocolVerifyConnect;
struct ENetProtocolBandwidthLimit;
struct ENetProtocolThrottleConfigure;
struct ENetProtocolDisconnect;
struct ENetProtocolPing;
struct ENetProtocolSendReliable;
struct ENetProtocolSendUnreliable;
struct ENetProtocolSendUnsequenced;
struct ENetProtocolSendFragment;
struct ENetProtocol;
```

## Functions
```cpp
int Moss_Init_Network(void);
void Moss_TerminateNetwork(void);

void enet_list_clear(ENetList*);
ENetListIterator enet_list_insert(ENetListIterator, void*);
void* enet_list_remove(ENetListIterator);
ENetListIterator enet_list_move(ENetListIterator, void*, void*);
size_t     enet_list_size(ENetList*);
uint32_t     enet_time_get(void);
void       enet_time_set(uint32_t);
ENetSocket enet_socket_create(ENetSocketType);
int        enet_socket_bind(ENetSocket, const ENetAddress *);
int        enet_socket_get_address(ENetSocket, ENetAddress *);
int        enet_socket_listen(ENetSocket, int);
ENetSocket enet_socket_accept(ENetSocket, ENetAddress *);
int        enet_socket_connect(ENetSocket, const ENetAddress *);
int        enet_socket_send(ENetSocket, const ENetAddress *, const ENetBuffer *, size_t);
int        enet_socket_receive(ENetSocket, ENetAddress *, ENetBuffer *, size_t);
int        enet_socket_wait(ENetSocket, uint32_t*, uint32_t);
int        enet_socket_set_option(ENetSocket, ENetSocketOption, int);
int        enet_socket_get_option(ENetSocket, ENetSocketOption, int *);
int        enet_socket_shutdown(ENetSocket, ENetSocketShutdown);
void       enet_socket_destroy(ENetSocket);
int        enet_socketset_select(ENetSocket, ENetSocketSet *, ENetSocketSet *, uint32_t);
int enet_address_set_host_ip(ENetAddress* address, const char* hostName);
int enet_address_set_host(ENetAddress* address, const char* hostName);
int enet_address_get_host_ip(const ENetAddress* address, char* hostName, size_t nameLength);
int enet_address_get_host(const ENetAddress* address, char* hostName, size_t nameLength);
ENetPacket* enet_packet_create(const void*, size_t, uint32_t);
void        enet_packet_destroy(ENetPacket*);
int         enet_packet_resize(ENetPacket*, size_t);
uint32_t      enet_crc32(const ENetBuffer*, size_t);            
ENetHost*   enet_host_create(const ENetAddress*, size_t, size_t, uint32_t, uint32_t);
void        enet_host_destroy(ENetHost*);
ENetPeer*   enet_host_connect(ENetHost*, const ENetAddress*, size_t, uint32_t);
int         enet_host_check_events(ENetHost*, ENetEvent*);
int         enet_host_service(ENetHost*, ENetEvent*, uint32_t);
void        enet_host_flush(ENetHost*);
void        enet_host_broadcast(ENetHost*, uint8_t, ENetPacket *);
void        enet_host_compress(ENetHost*, const ENetCompressor *);
int         enet_host_compress_with_range_coder(ENetHost * host);
void        enet_host_channel_limit(ENetHost*, size_t);
void        enet_host_bandwidth_limit(ENetHost*, uint32_t, uint32_t);
void        enet_host_bandwidth_throttle(ENetHost*);
uint32_t      enet_host_random_seed(void);
uint32_t      enet_host_random(ENetHost*);
int                  enet_peer_send(ENetPeer*, uint8_t, ENetPacket*);
ENetPacket*          enet_peer_receive(ENetPeer*, uint8_t * channelID);
void                 enet_peer_ping(ENetPeer*);
void                 enet_peer_ping_interval(ENetPeer*, uint32_t);
void                 enet_peer_timeout(ENetPeer*, uint32_t, uint32_t, uint32_t);
void                 enet_peer_reset(ENetPeer*);
void                 enet_peer_disconnect(ENetPeer*, uint32_t);
void                 enet_peer_disconnect_now(ENetPeer*, uint32_t);
void                 enet_peer_disconnect_later(ENetPeer*, uint32_t);
void                 enet_peer_throttle_configure(ENetPeer*, uint32_t, uint32_t, uint32_t);
int                  enet_peer_throttle(ENetPeer *, uint32_t);
void                 enet_peer_reset_queues(ENetPeer *);
int                  enet_peer_has_outgoing_commands(ENetPeer*);
void                 enet_peer_setup_outgoing_command(ENetPeer*, ENetOutgoingCommand*);
ENetOutgoingCommand* enet_peer_queue_outgoing_command(ENetPeer*, const ENetProtocol*, ENetPacket*, uint32_t, uint16);
ENetIncomingCommand* enet_peer_queue_incoming_command(ENetPeer*, const ENetProtocol*, const void*, size_t, uint32_t, uint32_t);
ENetAcknowledgement* enet_peer_queue_acknowledgement(ENetPeer*, const ENetProtocol*, uint16);
void                 enet_peer_dispatch_incoming_unreliable_commands(ENetPeer*, ENetChannel*, ENetIncomingCommand*);
void                 enet_peer_dispatch_incoming_reliable_commands(ENetPeer*, ENetChannel*, ENetIncomingCommand*);
void                 enet_peer_on_connect(ENetPeer*);
void                 enet_peer_on_disconnect(ENetPeer*);

void*  enet_range_coder_create(void);
void   enet_range_coder_destroy(void*);
size_t enet_range_coder_compress(void*, const ENetBuffer*, size_t, size_t, uint8_t*, size_t);
size_t enet_range_coder_decompress(void*, const uint8_t*, size_t, uint8_t*, size_t);
size_t enet_protocol_command_size(uint8_t);
```

## Examples
### Getting started
```cpp
...

if (Moss_Init_Network() != 0) { return 0; }

...
...
...

Moss_TerminateNetwork();
```

### Creating Host/Server
```cpp
ENetAddress address;
ENetHost* server;

enet_address_build_any(ENET_ADDRESS_TYPE_IPV6, &address);
address.port = 1234;
server = enet_host_create (ENET_ADDRESS_TYPE_ANY, &address, 32, 2, 0, 0);

MOSS_ASSERT(client != NULL);

...
...
...

enet_host_destroy(client);

```

### Connecting to a client using either IPV4 or IPV6.
```cpp
ENetAddress address;
ENetEvent event;
ENetPeer* peer;
ENetHost* client;

enet_address_set_host (& address, ENET_ADDRESS_TYPE_ANY, "some.server.net");
address.port = 1234;

client = enet_host_create (address.type, NULL, 1 , 2 ,0 , 0);

MOSS_ASSERT(client != NULL);

peer = enet_host_connect (client, & address, 2, 0);

MOSS_ASSERT(client != NULL);

if (enet_host_service (client, & event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {}
else {

    enet_peer_reset(peer);
}

...
...
...

enet_host_destroy(client);
```

### Events
```cpp
ENetEvent event;
    
    /* Wait up to 1000 milliseconds for an event. */
    while (enet_host_service (client, & event, 1000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            printf ("A new client connected from %x:%u.\n", 
                    event.peer -> address.host,
                    event.peer -> address.port);

            /* Store any relevant client information here. */
            event.peer -> data = "Client information";

            break;

        case ENET_EVENT_TYPE_RECEIVE:
            printf ("A packet of length %u containing %s was received from %s on channel %u.\n",
                    event.packet -> dataLength,
                    event.packet -> data,
                    event.peer -> data,
                    event.channelID);

            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy (event.packet);
            
            break;
           
        case ENET_EVENT_TYPE_DISCONNECT:
        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            printf ("%s disconnected.\n", event.peer -> data);

            /* Reset the peer's client information. */

            event.peer -> data = NULL;
        }
    }
    ...
    ...
    ...
```

### Creating Packets
```cpp
ENetPacket * packet = enet_packet_create ("packet", 
                                              strlen ("packet") + 1, 
                                              ENET_PACKET_FLAG_RELIABLE);

    /* Extend the packet so and append the string "foo", so it now */
    /* contains "packetfoo\0"                                      */
    enet_packet_resize (packet, strlen ("packetfoo") + 1);
    strcpy (& packet -> data [strlen ("packet")], "foo");
    
    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    enet_peer_send (peer, 0, packet);
    ...
    ...
    ...
    /* One could just use enet_host_service() instead. */
    enet_host_flush (host);
```

### Connecting to host
```cpp
ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;

    /* Connect to some.server.net:1234. */
    enet_address_set_host (& address, "some.server.net");
    address.port = 1234;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect (client, & address, 2, 0);    
    
    if (peer == NULL)
    {
       fprintf (stderr, 
                "No available peers for initiating an ENet connection.\n");
       exit (EXIT_FAILURE);
    }
    
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service (client, & event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts ("Connection to some.server.net:1234 succeeded.");
        ...
        ...
        ...
    }
    else
    {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset (peer);

        puts ("Connection to some.server.net:1234 failed.");
    }
    ...
    ...
    ...
```

### Disconnecting a Peer
```cpp
ENetEvent event;
    
    enet_peer_disconnect (peer, 0);

    /* Allow up to 3 seconds for the disconnect to succeed
     * and drop any packets received packets.
     */
    while (enet_host_service (client, & event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy (event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            puts ("Disconnection succeeded.");
            return;
        ...
        ...
        ...
        }
    }
    
    /* We've arrived here, so the disconnect attempt didn't */
    /* succeed yet.  Force the connection down.             */
    enet_peer_reset (peer);
    ...
    ...
    ...
```