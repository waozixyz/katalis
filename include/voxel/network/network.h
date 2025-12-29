/**
 * Network - LAN Multiplayer System
 *
 * Provides server/client networking for multiplayer gameplay using raw BSD sockets.
 * Host acts as both server and client. Supports up to 8 players.
 */

#ifndef VOXEL_NETWORK_H
#define VOXEL_NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <raylib.h>

// Include actual types to avoid forward declaration conflicts
#include "voxel/world/world.h"
#include "voxel/player/player.h"
#include "voxel/entity/entity.h"

// ============================================================================
// CONSTANTS
// ============================================================================

#define NET_PROTOCOL_MAGIC      0x4B41544C  // "KATL"
#define NET_PROTOCOL_VERSION    1
#define NET_MAX_CLIENTS         8
#define NET_DEFAULT_PORT        7777
#define NET_MAX_PACKET_SIZE     65535
#define NET_RECV_BUFFER_SIZE    65536
#define NET_SEND_BUFFER_SIZE    65536
#define NET_PLAYER_NAME_MAX     32
#define NET_HEARTBEAT_INTERVAL  1.0f        // Seconds between heartbeats
#define NET_TIMEOUT_DURATION    5.0f        // Seconds before disconnect

// ============================================================================
// PACKET TYPES
// ============================================================================

typedef enum {
    // Connection management
    NET_PACKET_CONNECT_REQUEST  = 0x01,
    NET_PACKET_CONNECT_ACCEPT   = 0x02,
    NET_PACKET_CONNECT_REJECT   = 0x03,
    NET_PACKET_DISCONNECT       = 0x04,
    NET_PACKET_HEARTBEAT        = 0x05,
    NET_PACKET_HEARTBEAT_ACK    = 0x06,

    // Player synchronization
    NET_PACKET_PLAYER_JOIN      = 0x10,
    NET_PACKET_PLAYER_LEAVE     = 0x11,
    NET_PACKET_PLAYER_STATE     = 0x12,
    NET_PACKET_PLAYER_STATES    = 0x13,  // Batch of all player states

    // World synchronization
    NET_PACKET_BLOCK_CHANGE     = 0x20,
    NET_PACKET_CHUNK_REQUEST    = 0x21,
    NET_PACKET_CHUNK_DATA       = 0x22,

    // Game state
    NET_PACKET_TIME_SYNC        = 0x30,
    NET_PACKET_INVENTORY_SYNC   = 0x31,
} NetPacketType;

// ============================================================================
// PACKET HEADER
// ============================================================================

typedef struct {
    uint32_t magic;             // NET_PROTOCOL_MAGIC
    uint8_t  version;           // NET_PROTOCOL_VERSION
    uint8_t  packet_type;       // NetPacketType
    uint16_t payload_size;      // Size of payload after header
    uint32_t sequence;          // Packet sequence number
} NetPacketHeader;

#define NET_HEADER_SIZE sizeof(NetPacketHeader)

// ============================================================================
// PACKET PAYLOADS
// ============================================================================

// Connection request (client -> server)
typedef struct {
    char player_name[NET_PLAYER_NAME_MAX];
} NetConnectRequest;

// Connection accepted (server -> client)
typedef struct {
    uint8_t  client_id;         // Assigned client ID (1-7, 0 = host)
    uint32_t world_seed;        // World seed for terrain
    float    time_of_day;       // Current game time
    uint8_t  player_count;      // Current number of players
} NetConnectAccept;

// Connection rejected (server -> client)
typedef struct {
    uint8_t reason;             // 0=full, 1=banned, 2=version_mismatch
    char    message[64];
} NetConnectReject;

// Player state (sent frequently, keep compact - 36 bytes)
typedef struct {
    uint8_t  client_id;
    float    pos_x, pos_y, pos_z;
    float    vel_x, vel_y, vel_z;
    float    yaw, pitch;
    uint8_t  flags;             // Bit 0: is_flying, Bit 1: is_grounded
    uint8_t  selected_slot;
} NetPlayerState;

// Player flags
#define NET_PLAYER_FLAG_FLYING      0x01
#define NET_PLAYER_FLAG_GROUNDED    0x02

// Player join notification (server -> all)
typedef struct {
    uint8_t client_id;
    char    player_name[NET_PLAYER_NAME_MAX];
    float   pos_x, pos_y, pos_z;
} NetPlayerJoin;

// Player leave notification (server -> all)
typedef struct {
    uint8_t client_id;
    uint8_t reason;             // 0=disconnect, 1=timeout, 2=kicked
} NetPlayerLeave;

// Block change (either -> server -> all)
typedef struct {
    int32_t x, y, z;
    uint8_t block_type;
    uint8_t metadata;
    uint8_t client_id;          // Who made the change
} NetBlockChange;

// Time synchronization (server -> all)
typedef struct {
    float time_of_day;
    float day_speed;
    uint8_t is_paused;
} NetTimeSync;

// ============================================================================
// NETWORK STATE
// ============================================================================

typedef enum {
    NET_MODE_NONE,              // Single player (no networking)
    NET_MODE_HOST,              // Hosting and playing
    NET_MODE_CLIENT,            // Connected to remote host
} NetworkMode;

typedef enum {
    NET_STATE_DISCONNECTED,
    NET_STATE_CONNECTING,
    NET_STATE_CONNECTED,
    NET_STATE_ERROR,
} NetClientState;

// ============================================================================
// CLIENT SLOT (for server)
// ============================================================================

typedef struct {
    int                socket_fd;
    uint8_t            client_id;
    char               player_name[NET_PLAYER_NAME_MAX];
    NetPlayerState     last_state;
    float              last_heartbeat;
    bool               connected;
    bool               authenticated;

    // Receive buffer for partial packets
    uint8_t            recv_buffer[NET_RECV_BUFFER_SIZE];
    size_t             recv_offset;
} NetClientSlot;

// ============================================================================
// SERVER
// ============================================================================

typedef struct {
    int                listen_socket;
    uint16_t           port;
    bool               running;

    // Client management
    NetClientSlot      clients[NET_MAX_CLIENTS];
    uint8_t            client_count;

    // Host info
    char               host_name[NET_PLAYER_NAME_MAX];

    // Game state references
    World*             world;
    Player*            host_player;
    EntityManager*     entity_manager;
    float*             time_of_day;
    float*             day_speed;

    // Packet sequencing
    uint32_t           next_sequence;

    // Host's own state (client_id = 0)
    NetPlayerState     host_state;
} NetServer;

// ============================================================================
// CLIENT
// ============================================================================

typedef struct {
    int                socket_fd;
    NetClientState     state;
    uint8_t            my_client_id;
    char               player_name[NET_PLAYER_NAME_MAX];

    // Receive buffer
    uint8_t            recv_buffer[NET_RECV_BUFFER_SIZE];
    size_t             recv_offset;

    // Remote player tracking
    NetPlayerState     remote_players[NET_MAX_CLIENTS];
    bool               player_active[NET_MAX_CLIENTS];
    char               player_names[NET_MAX_CLIENTS][NET_PLAYER_NAME_MAX];

    // Game state references
    World*             world;
    Player*            local_player;
    EntityManager*     entity_manager;
    float*             time_of_day;

    // Connection info
    char               server_ip[64];
    uint16_t           server_port;

    // Timing
    float              last_heartbeat_sent;
    float              last_heartbeat_received;
    uint32_t           send_sequence;

    // Error handling
    char               error_message[128];
} NetClient;

// ============================================================================
// NETWORK CONTEXT (unified for game integration)
// ============================================================================

typedef struct NetworkContext {
    NetworkMode        mode;
    NetServer*         server;          // Non-NULL when hosting
    NetClient*         client;          // Non-NULL when connected as client

    // Remote player entities (for rendering)
    Entity*            remote_entities[NET_MAX_CLIENTS];
    char               remote_names[NET_MAX_CLIENTS][NET_PLAYER_NAME_MAX];

    // Send rate limiting
    float              send_timer;
    float              send_interval;   // Default 0.05 (20 Hz)
} NetworkContext;

// ============================================================================
// SERVER API
// ============================================================================

/**
 * Create a new server instance
 */
NetServer* net_server_create(uint16_t port, World* world, Player* host_player,
                              EntityManager* entities, float* time_of_day, float* day_speed);

/**
 * Destroy server and free resources
 */
void net_server_destroy(NetServer* server);

/**
 * Start listening for connections
 */
bool net_server_start(NetServer* server);

/**
 * Stop server and disconnect all clients
 */
void net_server_stop(NetServer* server);

/**
 * Poll for network events (non-blocking)
 * Call every frame with timeout_ms = 0
 */
int net_server_poll(NetServer* server, int timeout_ms);

/**
 * Broadcast all player states to all clients
 */
void net_server_broadcast_states(NetServer* server);

/**
 * Broadcast a block change to all clients
 */
void net_server_broadcast_block_change(NetServer* server, int x, int y, int z,
                                        uint8_t block_type, uint8_t metadata);

/**
 * Broadcast time synchronization
 */
void net_server_broadcast_time(NetServer* server);

/**
 * Get number of connected clients (not including host)
 */
int net_server_get_client_count(NetServer* server);

// ============================================================================
// CLIENT API
// ============================================================================

/**
 * Create a new client instance
 */
NetClient* net_client_create(const char* player_name, World* world, Player* player,
                              EntityManager* entities, float* time_of_day);

/**
 * Destroy client and free resources
 */
void net_client_destroy(NetClient* client);

/**
 * Connect to a server (non-blocking, check state for result)
 */
bool net_client_connect(NetClient* client, const char* ip, uint16_t port);

/**
 * Disconnect from server
 */
void net_client_disconnect(NetClient* client);

/**
 * Poll for network events (non-blocking)
 */
int net_client_poll(NetClient* client, int timeout_ms);

/**
 * Send local player state to server
 */
void net_client_send_player_state(NetClient* client);

/**
 * Request a block change (server validates and broadcasts)
 */
void net_client_send_block_change(NetClient* client, int x, int y, int z,
                                   uint8_t block_type, uint8_t metadata);

/**
 * Get client connection state
 */
NetClientState net_client_get_state(NetClient* client);

/**
 * Get error message (if state == NET_STATE_ERROR)
 */
const char* net_client_get_error(NetClient* client);

// ============================================================================
// NETWORK CONTEXT API (high-level game integration)
// ============================================================================

/**
 * Create network context
 */
NetworkContext* network_create(void);

/**
 * Destroy network context
 */
void network_destroy(NetworkContext* ctx);

/**
 * Start hosting a game
 */
bool network_host(NetworkContext* ctx, uint16_t port, const char* host_name,
                  World* world, Player* player, EntityManager* entities,
                  float* time_of_day, float* day_speed);

/**
 * Join a game
 */
bool network_join(NetworkContext* ctx, const char* ip, uint16_t port,
                  const char* player_name, World* world, Player* player,
                  EntityManager* entities, float* time_of_day);

/**
 * Disconnect/stop
 */
void network_disconnect(NetworkContext* ctx);

/**
 * Update network (call every frame)
 */
void network_update(NetworkContext* ctx, float dt);

/**
 * Broadcast a block change
 */
void network_broadcast_block_change(NetworkContext* ctx, int x, int y, int z,
                                     uint8_t block_type, uint8_t metadata);

/**
 * Update remote player entities for rendering
 */
void network_update_remote_players(NetworkContext* ctx, float dt);

/**
 * Get network mode
 */
NetworkMode network_get_mode(NetworkContext* ctx);

/**
 * Get player count (including local player)
 */
int network_get_player_count(NetworkContext* ctx);

/**
 * Check if a remote player slot is active
 */
bool network_is_player_active(NetworkContext* ctx, uint8_t client_id);

/**
 * Get remote player name
 */
const char* network_get_player_name(NetworkContext* ctx, uint8_t client_id);

/**
 * Draw nametags above all remote players
 * Should be called during 3D rendering phase
 */
void network_draw_nametags(NetworkContext* ctx, Camera3D camera);

#endif // VOXEL_NETWORK_H
