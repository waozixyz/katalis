/**
 * Network - LAN Multiplayer Implementation
 */

#define _POSIX_C_SOURCE 200112L

#include "voxel/network.h"
#include "voxel/world.h"
#include "voxel/player.h"
#include "voxel/inventory.h"
#include "voxel/entity.h"
#include "voxel/block_human.h"
#include "voxel/block.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static float get_time_seconds(void) {
    return (float)GetTime();  // Raylib's GetTime()
}

static void set_socket_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void set_socket_nodelay(int fd) {
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

static void set_socket_reuseaddr(int fd) {
    int flag = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

// ============================================================================
// SERIALIZATION HELPERS
// ============================================================================

static void write_u8(uint8_t** buf, uint8_t val) {
    **buf = val;
    (*buf)++;
}

static void write_u16(uint8_t** buf, uint16_t val) {
    (*buf)[0] = val & 0xFF;
    (*buf)[1] = (val >> 8) & 0xFF;
    (*buf) += 2;
}

static void write_u32(uint8_t** buf, uint32_t val) {
    (*buf)[0] = val & 0xFF;
    (*buf)[1] = (val >> 8) & 0xFF;
    (*buf)[2] = (val >> 16) & 0xFF;
    (*buf)[3] = (val >> 24) & 0xFF;
    (*buf) += 4;
}

static void write_i32(uint8_t** buf, int32_t val) {
    write_u32(buf, (uint32_t)val);
}

static void write_f32(uint8_t** buf, float val) {
    union { float f; uint32_t u; } conv;
    conv.f = val;
    write_u32(buf, conv.u);
}

static void write_string(uint8_t** buf, const char* str, size_t max_len) {
    size_t len = strlen(str);
    if (len > max_len - 1) len = max_len - 1;
    memcpy(*buf, str, len);
    memset(*buf + len, 0, max_len - len);
    (*buf) += max_len;
}

static uint8_t read_u8(const uint8_t** buf) {
    uint8_t val = **buf;
    (*buf)++;
    return val;
}

static uint16_t read_u16(const uint8_t** buf) {
    uint16_t val = (*buf)[0] | ((*buf)[1] << 8);
    (*buf) += 2;
    return val;
}

static uint32_t read_u32(const uint8_t** buf) {
    uint32_t val = (*buf)[0] | ((*buf)[1] << 8) | ((*buf)[2] << 16) | ((*buf)[3] << 24);
    (*buf) += 4;
    return val;
}

static int32_t read_i32(const uint8_t** buf) {
    return (int32_t)read_u32(buf);
}

static float read_f32(const uint8_t** buf) {
    union { float f; uint32_t u; } conv;
    conv.u = read_u32(buf);
    return conv.f;
}

static void read_string(const uint8_t** buf, char* out, size_t max_len) {
    memcpy(out, *buf, max_len);
    out[max_len - 1] = '\0';
    (*buf) += max_len;
}

// ============================================================================
// PACKET BUILDING
// ============================================================================

static size_t build_packet_header(uint8_t* buf, NetPacketType type,
                                   uint16_t payload_size, uint32_t sequence) {
    uint8_t* p = buf;
    write_u32(&p, NET_PROTOCOL_MAGIC);
    write_u8(&p, NET_PROTOCOL_VERSION);
    write_u8(&p, (uint8_t)type);
    write_u16(&p, payload_size);
    write_u32(&p, sequence);
    return NET_HEADER_SIZE;
}

static size_t build_player_state(uint8_t* buf, const NetPlayerState* state) {
    uint8_t* p = buf;
    write_u8(&p, state->client_id);
    write_f32(&p, state->pos_x);
    write_f32(&p, state->pos_y);
    write_f32(&p, state->pos_z);
    write_f32(&p, state->vel_x);
    write_f32(&p, state->vel_y);
    write_f32(&p, state->vel_z);
    write_f32(&p, state->yaw);
    write_f32(&p, state->pitch);
    write_u8(&p, state->flags);
    write_u8(&p, state->selected_slot);
    return p - buf;
}

static size_t parse_player_state(const uint8_t* buf, NetPlayerState* state) {
    const uint8_t* p = buf;
    state->client_id = read_u8(&p);
    state->pos_x = read_f32(&p);
    state->pos_y = read_f32(&p);
    state->pos_z = read_f32(&p);
    state->vel_x = read_f32(&p);
    state->vel_y = read_f32(&p);
    state->vel_z = read_f32(&p);
    state->yaw = read_f32(&p);
    state->pitch = read_f32(&p);
    state->flags = read_u8(&p);
    state->selected_slot = read_u8(&p);
    return p - buf;
}

static size_t build_block_change(uint8_t* buf, const NetBlockChange* change) {
    uint8_t* p = buf;
    write_i32(&p, change->x);
    write_i32(&p, change->y);
    write_i32(&p, change->z);
    write_u8(&p, change->block_type);
    write_u8(&p, change->metadata);
    write_u8(&p, change->client_id);
    return p - buf;
}

static size_t parse_block_change(const uint8_t* buf, NetBlockChange* change) {
    const uint8_t* p = buf;
    change->x = read_i32(&p);
    change->y = read_i32(&p);
    change->z = read_i32(&p);
    change->block_type = read_u8(&p);
    change->metadata = read_u8(&p);
    change->client_id = read_u8(&p);
    return p - buf;
}

// ============================================================================
// SERVER IMPLEMENTATION
// ============================================================================

NetServer* net_server_create(uint16_t port, World* world, Player* host_player,
                              EntityManager* entities, float* time_of_day, float* day_speed) {
    NetServer* server = (NetServer*)calloc(1, sizeof(NetServer));
    if (!server) return NULL;

    server->port = port;
    server->world = world;
    server->host_player = host_player;
    server->entity_manager = entities;
    server->time_of_day = time_of_day;
    server->day_speed = day_speed;
    server->listen_socket = -1;
    server->running = false;

    // Initialize client slots
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        server->clients[i].socket_fd = -1;
        server->clients[i].client_id = i;
        server->clients[i].connected = false;
    }

    printf("[NET_SERVER] Created on port %d\n", port);
    return server;
}

void net_server_destroy(NetServer* server) {
    if (!server) return;

    net_server_stop(server);
    free(server);
    printf("[NET_SERVER] Destroyed\n");
}

bool net_server_start(NetServer* server) {
    if (!server || server->running) return false;

    // Create socket
    server->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_socket < 0) {
        printf("[NET_SERVER] Failed to create socket: %s\n", strerror(errno));
        return false;
    }

    set_socket_reuseaddr(server->listen_socket);
    set_socket_nonblocking(server->listen_socket);

    // Bind
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->port);

    if (bind(server->listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("[NET_SERVER] Failed to bind: %s\n", strerror(errno));
        close(server->listen_socket);
        server->listen_socket = -1;
        return false;
    }

    // Listen
    if (listen(server->listen_socket, 8) < 0) {
        printf("[NET_SERVER] Failed to listen: %s\n", strerror(errno));
        close(server->listen_socket);
        server->listen_socket = -1;
        return false;
    }

    server->running = true;
    printf("[NET_SERVER] Started listening on port %d\n", server->port);
    return true;
}

void net_server_stop(NetServer* server) {
    if (!server) return;

    // Disconnect all clients
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].connected && server->clients[i].socket_fd >= 0) {
            close(server->clients[i].socket_fd);
            server->clients[i].socket_fd = -1;
            server->clients[i].connected = false;
        }
    }

    // Close listen socket
    if (server->listen_socket >= 0) {
        close(server->listen_socket);
        server->listen_socket = -1;
    }

    server->running = false;
    server->client_count = 0;
    printf("[NET_SERVER] Stopped\n");
}

static void server_accept_connection(NetServer* server) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(server->listen_socket, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) return;

    // Find free slot (starting from 1, 0 is host)
    int slot = -1;
    for (int i = 1; i < NET_MAX_CLIENTS; i++) {
        if (!server->clients[i].connected) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        // Server full
        printf("[NET_SERVER] Connection rejected - server full\n");
        close(client_fd);
        return;
    }

    set_socket_nonblocking(client_fd);
    set_socket_nodelay(client_fd);

    NetClientSlot* client = &server->clients[slot];
    memset(client, 0, sizeof(NetClientSlot));
    client->socket_fd = client_fd;
    client->client_id = slot;
    client->connected = true;
    client->authenticated = false;
    client->last_heartbeat = get_time_seconds();

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    printf("[NET_SERVER] Client connected from %s (slot %d)\n", ip_str, slot);
}

static void server_send_packet(NetServer* server, int client_id,
                                NetPacketType type, const void* payload, size_t payload_size) {
    if (client_id < 0 || client_id >= NET_MAX_CLIENTS) return;
    NetClientSlot* client = &server->clients[client_id];
    if (!client->connected || client->socket_fd < 0) return;

    uint8_t packet[NET_MAX_PACKET_SIZE];
    build_packet_header(packet, type, payload_size, server->next_sequence++);
    if (payload && payload_size > 0) {
        memcpy(packet + NET_HEADER_SIZE, payload, payload_size);
    }

    ssize_t sent = send(client->socket_fd, packet, NET_HEADER_SIZE + payload_size, MSG_NOSIGNAL);
    if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        printf("[NET_SERVER] Send error to client %d: %s\n", client_id, strerror(errno));
    }
}

static void server_broadcast(NetServer* server, NetPacketType type,
                              const void* payload, size_t payload_size, int exclude) {
    for (int i = 1; i < NET_MAX_CLIENTS; i++) {
        if (i == exclude) continue;
        if (server->clients[i].connected && server->clients[i].authenticated) {
            server_send_packet(server, i, type, payload, payload_size);
        }
    }
}

static void server_handle_connect_request(NetServer* server, int client_id, const uint8_t* data) {
    NetClientSlot* client = &server->clients[client_id];

    // Parse request
    const uint8_t* p = data;
    read_string(&p, client->player_name, NET_PLAYER_NAME_MAX);

    printf("[NET_SERVER] Player '%s' joining as client %d\n", client->player_name, client_id);

    // Send accept
    uint8_t accept_buf[64];
    uint8_t* ap = accept_buf;
    write_u8(&ap, client_id);
    write_u32(&ap, 0);  // world_seed (not used currently)
    write_f32(&ap, server->time_of_day ? *server->time_of_day : 12.0f);
    write_u8(&ap, server->client_count + 1);

    server_send_packet(server, client_id, NET_PACKET_CONNECT_ACCEPT, accept_buf, ap - accept_buf);
    client->authenticated = true;
    server->client_count++;

    // Notify other clients
    uint8_t join_buf[64];
    uint8_t* jp = join_buf;
    write_u8(&jp, client_id);
    write_string(&jp, client->player_name, NET_PLAYER_NAME_MAX);
    write_f32(&jp, server->host_player->position.x);
    write_f32(&jp, server->host_player->position.y);
    write_f32(&jp, server->host_player->position.z);

    server_broadcast(server, NET_PACKET_PLAYER_JOIN, join_buf, jp - join_buf, client_id);
}

static void server_handle_player_state(NetServer* server, int client_id, const uint8_t* data) {
    NetClientSlot* client = &server->clients[client_id];
    parse_player_state(data, &client->last_state);
    client->last_state.client_id = client_id;  // Ensure correct ID
}

static void server_handle_block_change(NetServer* server, int client_id, const uint8_t* data) {
    NetBlockChange change;
    parse_block_change(data, &change);
    change.client_id = client_id;

    // Validate: check distance from player
    NetClientSlot* client = &server->clients[client_id];
    float dx = change.x - client->last_state.pos_x;
    float dy = change.y - client->last_state.pos_y;
    float dz = change.z - client->last_state.pos_z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);

    if (dist > 8.0f) {
        printf("[NET_SERVER] Block change rejected - too far (%.1f)\n", dist);
        return;
    }

    // Apply to world
    Block block = {change.block_type, 0, change.metadata};
    world_set_block(server->world, change.x, change.y, change.z, block);

    // Broadcast to all clients
    uint8_t buf[32];
    size_t len = build_block_change(buf, &change);
    server_broadcast(server, NET_PACKET_BLOCK_CHANGE, buf, len, -1);

    printf("[NET_SERVER] Block change at (%d,%d,%d) by client %d\n",
           change.x, change.y, change.z, client_id);
}

static void server_handle_packet(NetServer* server, int client_id,
                                  NetPacketType type, const uint8_t* data, size_t size) {
    (void)size;

    switch (type) {
        case NET_PACKET_CONNECT_REQUEST:
            server_handle_connect_request(server, client_id, data);
            break;
        case NET_PACKET_PLAYER_STATE:
            server_handle_player_state(server, client_id, data);
            break;
        case NET_PACKET_BLOCK_CHANGE:
            server_handle_block_change(server, client_id, data);
            break;
        case NET_PACKET_HEARTBEAT:
            server->clients[client_id].last_heartbeat = get_time_seconds();
            server_send_packet(server, client_id, NET_PACKET_HEARTBEAT_ACK, NULL, 0);
            break;
        case NET_PACKET_DISCONNECT:
            printf("[NET_SERVER] Client %d disconnecting\n", client_id);
            close(server->clients[client_id].socket_fd);
            server->clients[client_id].socket_fd = -1;
            server->clients[client_id].connected = false;
            if (server->clients[client_id].authenticated) {
                server->client_count--;
            }
            break;
        default:
            break;
    }
}

static void server_receive_client(NetServer* server, int client_id) {
    NetClientSlot* client = &server->clients[client_id];

    ssize_t received = recv(client->socket_fd,
                            client->recv_buffer + client->recv_offset,
                            NET_RECV_BUFFER_SIZE - client->recv_offset, 0);

    if (received < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("[NET_SERVER] Receive error from client %d: %s\n", client_id, strerror(errno));
            close(client->socket_fd);
            client->socket_fd = -1;
            client->connected = false;
            if (client->authenticated) server->client_count--;
        }
        return;
    }

    if (received == 0) {
        // Client disconnected
        printf("[NET_SERVER] Client %d disconnected\n", client_id);
        close(client->socket_fd);
        client->socket_fd = -1;
        client->connected = false;
        if (client->authenticated) server->client_count--;
        return;
    }

    client->recv_offset += received;

    // Process complete packets
    while (client->recv_offset >= NET_HEADER_SIZE) {
        const uint8_t* p = client->recv_buffer;

        // Validate header
        uint32_t magic = read_u32(&p);
        if (magic != NET_PROTOCOL_MAGIC) {
            printf("[NET_SERVER] Invalid magic from client %d\n", client_id);
            client->recv_offset = 0;
            break;
        }

        uint8_t version = read_u8(&p);
        (void)version;
        uint8_t type = read_u8(&p);
        uint16_t payload_size = read_u16(&p);
        read_u32(&p);  // sequence

        size_t total_size = NET_HEADER_SIZE + payload_size;
        if (client->recv_offset < total_size) {
            break;  // Wait for more data
        }

        // Handle packet
        server_handle_packet(server, client_id, (NetPacketType)type,
                             client->recv_buffer + NET_HEADER_SIZE, payload_size);

        // Remove packet from buffer
        memmove(client->recv_buffer, client->recv_buffer + total_size,
                client->recv_offset - total_size);
        client->recv_offset -= total_size;
    }
}

int net_server_poll(NetServer* server, int timeout_ms) {
    if (!server || !server->running) return 0;

    struct pollfd fds[NET_MAX_CLIENTS + 1];
    int nfds = 0;

    // Add listen socket
    fds[nfds].fd = server->listen_socket;
    fds[nfds].events = POLLIN;
    nfds++;

    // Add client sockets
    int client_fds[NET_MAX_CLIENTS];
    int client_count = 0;
    for (int i = 1; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].connected && server->clients[i].socket_fd >= 0) {
            fds[nfds].fd = server->clients[i].socket_fd;
            fds[nfds].events = POLLIN;
            client_fds[client_count++] = i;
            nfds++;
        }
    }

    int ready = poll(fds, nfds, timeout_ms);
    if (ready <= 0) return 0;

    int events = 0;

    // Check listen socket
    if (fds[0].revents & POLLIN) {
        server_accept_connection(server);
        events++;
    }

    // Check client sockets
    for (int i = 0; i < client_count; i++) {
        if (fds[i + 1].revents & POLLIN) {
            server_receive_client(server, client_fds[i]);
            events++;
        }
        if (fds[i + 1].revents & (POLLERR | POLLHUP)) {
            int client_id = client_fds[i];
            printf("[NET_SERVER] Client %d connection error\n", client_id);
            close(server->clients[client_id].socket_fd);
            server->clients[client_id].socket_fd = -1;
            server->clients[client_id].connected = false;
            if (server->clients[client_id].authenticated) server->client_count--;
            events++;
        }
    }

    return events;
}

void net_server_broadcast_states(NetServer* server) {
    if (!server || !server->running) return;

    // Build host state
    server->host_state.client_id = 0;
    server->host_state.pos_x = server->host_player->position.x;
    server->host_state.pos_y = server->host_player->position.y;
    server->host_state.pos_z = server->host_player->position.z;
    server->host_state.vel_x = server->host_player->velocity.x;
    server->host_state.vel_y = server->host_player->velocity.y;
    server->host_state.vel_z = server->host_player->velocity.z;
    server->host_state.yaw = server->host_player->yaw;
    server->host_state.pitch = server->host_player->pitch;
    server->host_state.flags = 0;
    if (server->host_player->is_flying) server->host_state.flags |= NET_PLAYER_FLAG_FLYING;
    if (server->host_player->is_grounded) server->host_state.flags |= NET_PLAYER_FLAG_GROUNDED;
    server->host_state.selected_slot = server->host_player->inventory->selected_hotbar_slot;

    // Build packet with all player states
    uint8_t buf[1024];
    uint8_t* p = buf;

    // Count active players
    int player_count = 1;  // Host
    for (int i = 1; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].connected && server->clients[i].authenticated) {
            player_count++;
        }
    }
    write_u8(&p, player_count);

    // Write host state
    p += build_player_state(p, &server->host_state);

    // Write client states
    for (int i = 1; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].connected && server->clients[i].authenticated) {
            p += build_player_state(p, &server->clients[i].last_state);
        }
    }

    server_broadcast(server, NET_PACKET_PLAYER_STATES, buf, p - buf, -1);
}

void net_server_broadcast_block_change(NetServer* server, int x, int y, int z,
                                        uint8_t block_type, uint8_t metadata) {
    if (!server || !server->running) return;

    NetBlockChange change = {x, y, z, block_type, metadata, 0};
    uint8_t buf[32];
    size_t len = build_block_change(buf, &change);
    server_broadcast(server, NET_PACKET_BLOCK_CHANGE, buf, len, -1);
}

void net_server_broadcast_time(NetServer* server) {
    if (!server || !server->running) return;

    uint8_t buf[16];
    uint8_t* p = buf;
    write_f32(&p, server->time_of_day ? *server->time_of_day : 12.0f);
    write_f32(&p, server->day_speed ? *server->day_speed : 0.5f);
    write_u8(&p, 0);  // not paused

    server_broadcast(server, NET_PACKET_TIME_SYNC, buf, p - buf, -1);
}

int net_server_get_client_count(NetServer* server) {
    return server ? server->client_count : 0;
}

// ============================================================================
// CLIENT IMPLEMENTATION
// ============================================================================

NetClient* net_client_create(const char* player_name, World* world, Player* player,
                              EntityManager* entities, float* time_of_day) {
    NetClient* client = (NetClient*)calloc(1, sizeof(NetClient));
    if (!client) return NULL;

    strncpy(client->player_name, player_name, NET_PLAYER_NAME_MAX - 1);
    client->world = world;
    client->local_player = player;
    client->entity_manager = entities;
    client->time_of_day = time_of_day;
    client->socket_fd = -1;
    client->state = NET_STATE_DISCONNECTED;

    printf("[NET_CLIENT] Created as '%s'\n", player_name);
    return client;
}

void net_client_destroy(NetClient* client) {
    if (!client) return;

    net_client_disconnect(client);
    free(client);
    printf("[NET_CLIENT] Destroyed\n");
}

bool net_client_connect(NetClient* client, const char* ip, uint16_t port) {
    if (!client) return false;

    printf("[NET_CLIENT] Attempting to connect to %s:%d...\n", ip, port);

    strncpy(client->server_ip, ip, sizeof(client->server_ip) - 1);
    client->server_port = port;

    // Use getaddrinfo to resolve hostname (supports "localhost", IPs, etc.)
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int gai_result = getaddrinfo(ip, port_str, &hints, &result);
    if (gai_result != 0) {
        snprintf(client->error_message, sizeof(client->error_message),
                 "Failed to resolve '%s': %s", ip, gai_strerror(gai_result));
        printf("[NET_CLIENT] DNS resolution failed: %s\n", gai_strerror(gai_result));
        client->state = NET_STATE_ERROR;
        return false;
    }

    client->socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (client->socket_fd < 0) {
        snprintf(client->error_message, sizeof(client->error_message),
                 "Failed to create socket");
        printf("[NET_CLIENT] Socket creation failed\n");
        freeaddrinfo(result);
        client->state = NET_STATE_ERROR;
        return false;
    }

    set_socket_nonblocking(client->socket_fd);
    set_socket_nodelay(client->socket_fd);

    int conn_result = connect(client->socket_fd, result->ai_addr, result->ai_addrlen);
    int saved_errno = errno;  // Save errno before freeaddrinfo might change it
    freeaddrinfo(result);

    printf("[NET_CLIENT] connect() returned %d, errno=%d (%s)\n",
           conn_result, saved_errno, strerror(saved_errno));

    if (conn_result < 0 && saved_errno != EINPROGRESS) {
        snprintf(client->error_message, sizeof(client->error_message),
                 "Connection failed: %s", strerror(saved_errno));
        printf("[NET_CLIENT] Connection failed: %s\n", strerror(saved_errno));
        close(client->socket_fd);
        client->socket_fd = -1;
        client->state = NET_STATE_ERROR;
        return false;
    }

    client->state = NET_STATE_CONNECTING;
    printf("[NET_CLIENT] Connecting to %s:%d (state=CONNECTING)\n", ip, port);
    return true;
}

void net_client_disconnect(NetClient* client) {
    if (!client) return;

    if (client->socket_fd >= 0) {
        // Send disconnect packet
        uint8_t packet[NET_HEADER_SIZE];
        build_packet_header(packet, NET_PACKET_DISCONNECT, 0, client->send_sequence++);
        send(client->socket_fd, packet, NET_HEADER_SIZE, MSG_NOSIGNAL);

        close(client->socket_fd);
        client->socket_fd = -1;
    }

    client->state = NET_STATE_DISCONNECTED;
    client->recv_offset = 0;

    // Clear remote players
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        client->player_active[i] = false;
    }

    printf("[NET_CLIENT] Disconnected\n");
}

static void client_send_packet(NetClient* client, NetPacketType type,
                                const void* payload, size_t payload_size) {
    if (client->socket_fd < 0) return;

    uint8_t packet[NET_MAX_PACKET_SIZE];
    build_packet_header(packet, type, payload_size, client->send_sequence++);
    if (payload && payload_size > 0) {
        memcpy(packet + NET_HEADER_SIZE, payload, payload_size);
    }

    ssize_t sent = send(client->socket_fd, packet, NET_HEADER_SIZE + payload_size, MSG_NOSIGNAL);
    if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        printf("[NET_CLIENT] Send error: %s\n", strerror(errno));
    }
}

static void client_handle_connect_accept(NetClient* client, const uint8_t* data) {
    const uint8_t* p = data;
    client->my_client_id = read_u8(&p);
    read_u32(&p);  // world_seed
    float time_of_day = read_f32(&p);
    read_u8(&p);  // player_count

    if (client->time_of_day) {
        *client->time_of_day = time_of_day;
    }

    client->state = NET_STATE_CONNECTED;
    printf("[NET_CLIENT] Connected as client %d\n", client->my_client_id);
}

static void client_handle_player_states(NetClient* client, const uint8_t* data) {
    const uint8_t* p = data;
    uint8_t player_count = read_u8(&p);

    for (int i = 0; i < player_count; i++) {
        NetPlayerState state;
        p += parse_player_state(p, &state);

        // Skip our own state
        if (state.client_id == client->my_client_id) continue;

        client->remote_players[state.client_id] = state;
        client->player_active[state.client_id] = true;
    }
}

static void client_handle_player_join(NetClient* client, const uint8_t* data) {
    const uint8_t* p = data;
    uint8_t client_id = read_u8(&p);
    read_string(&p, client->player_names[client_id], NET_PLAYER_NAME_MAX);
    float pos_x = read_f32(&p);
    float pos_y = read_f32(&p);
    float pos_z = read_f32(&p);

    client->remote_players[client_id].pos_x = pos_x;
    client->remote_players[client_id].pos_y = pos_y;
    client->remote_players[client_id].pos_z = pos_z;
    client->player_active[client_id] = true;

    printf("[NET_CLIENT] Player '%s' joined (id=%d)\n", client->player_names[client_id], client_id);
}

static void client_handle_player_leave(NetClient* client, const uint8_t* data) {
    const uint8_t* p = data;
    uint8_t client_id = read_u8(&p);
    read_u8(&p);  // reason

    client->player_active[client_id] = false;
    printf("[NET_CLIENT] Player %d left\n", client_id);
}

static void client_handle_block_change(NetClient* client, const uint8_t* data) {
    NetBlockChange change;
    parse_block_change(data, &change);

    Block block = {change.block_type, 0, change.metadata};
    world_set_block(client->world, change.x, change.y, change.z, block);
}

static void client_handle_time_sync(NetClient* client, const uint8_t* data) {
    const uint8_t* p = data;
    float time_of_day = read_f32(&p);

    if (client->time_of_day) {
        *client->time_of_day = time_of_day;
    }
}

static void client_handle_packet(NetClient* client, NetPacketType type,
                                  const uint8_t* data, size_t size) {
    (void)size;

    switch (type) {
        case NET_PACKET_CONNECT_ACCEPT:
            client_handle_connect_accept(client, data);
            break;
        case NET_PACKET_CONNECT_REJECT:
            snprintf(client->error_message, sizeof(client->error_message),
                     "Connection rejected by server");
            client->state = NET_STATE_ERROR;
            break;
        case NET_PACKET_PLAYER_STATES:
            client_handle_player_states(client, data);
            break;
        case NET_PACKET_PLAYER_JOIN:
            client_handle_player_join(client, data);
            break;
        case NET_PACKET_PLAYER_LEAVE:
            client_handle_player_leave(client, data);
            break;
        case NET_PACKET_BLOCK_CHANGE:
            client_handle_block_change(client, data);
            break;
        case NET_PACKET_TIME_SYNC:
            client_handle_time_sync(client, data);
            break;
        case NET_PACKET_HEARTBEAT_ACK:
            client->last_heartbeat_received = get_time_seconds();
            break;
        default:
            break;
    }
}

int net_client_poll(NetClient* client, int timeout_ms) {
    if (!client || client->socket_fd < 0) return 0;

    // Check for connection completion
    if (client->state == NET_STATE_CONNECTING) {
        struct pollfd pfd = {client->socket_fd, POLLOUT, 0};
        if (poll(&pfd, 1, 0) > 0) {
            int error = 0;
            socklen_t len = sizeof(error);
            getsockopt(client->socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);

            if (error != 0) {
                snprintf(client->error_message, sizeof(client->error_message),
                         "Connection failed: %s", strerror(error));
                client->state = NET_STATE_ERROR;
                close(client->socket_fd);
                client->socket_fd = -1;
                return 0;
            }

            // Connected - send handshake
            uint8_t buf[64];
            uint8_t* p = buf;
            write_string(&p, client->player_name, NET_PLAYER_NAME_MAX);
            client_send_packet(client, NET_PACKET_CONNECT_REQUEST, buf, p - buf);
        }
    }

    // Poll for incoming data
    struct pollfd pfd = {client->socket_fd, POLLIN, 0};
    int ready = poll(&pfd, 1, timeout_ms);
    if (ready <= 0) return 0;

    if (pfd.revents & (POLLERR | POLLHUP)) {
        snprintf(client->error_message, sizeof(client->error_message),
                 "Connection lost");
        client->state = NET_STATE_ERROR;
        close(client->socket_fd);
        client->socket_fd = -1;
        return 0;
    }

    if (pfd.revents & POLLIN) {
        ssize_t received = recv(client->socket_fd,
                                client->recv_buffer + client->recv_offset,
                                NET_RECV_BUFFER_SIZE - client->recv_offset, 0);

        if (received <= 0) {
            if (received == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                snprintf(client->error_message, sizeof(client->error_message),
                         "Connection closed");
                client->state = NET_STATE_DISCONNECTED;
                close(client->socket_fd);
                client->socket_fd = -1;
                return 0;
            }
            return 0;
        }

        client->recv_offset += received;

        // Process complete packets
        int packets = 0;
        while (client->recv_offset >= NET_HEADER_SIZE) {
            const uint8_t* p = client->recv_buffer;

            uint32_t magic = read_u32(&p);
            if (magic != NET_PROTOCOL_MAGIC) {
                client->recv_offset = 0;
                break;
            }

            read_u8(&p);  // version
            uint8_t type = read_u8(&p);
            uint16_t payload_size = read_u16(&p);
            read_u32(&p);  // sequence

            size_t total_size = NET_HEADER_SIZE + payload_size;
            if (client->recv_offset < total_size) break;

            client_handle_packet(client, (NetPacketType)type,
                                 client->recv_buffer + NET_HEADER_SIZE, payload_size);
            packets++;

            memmove(client->recv_buffer, client->recv_buffer + total_size,
                    client->recv_offset - total_size);
            client->recv_offset -= total_size;
        }

        return packets;
    }

    return 0;
}

void net_client_send_player_state(NetClient* client) {
    if (!client || client->state != NET_STATE_CONNECTED) return;

    NetPlayerState state;
    state.client_id = client->my_client_id;
    state.pos_x = client->local_player->position.x;
    state.pos_y = client->local_player->position.y;
    state.pos_z = client->local_player->position.z;
    state.vel_x = client->local_player->velocity.x;
    state.vel_y = client->local_player->velocity.y;
    state.vel_z = client->local_player->velocity.z;
    state.yaw = client->local_player->yaw;
    state.pitch = client->local_player->pitch;
    state.flags = 0;
    if (client->local_player->is_flying) state.flags |= NET_PLAYER_FLAG_FLYING;
    if (client->local_player->is_grounded) state.flags |= NET_PLAYER_FLAG_GROUNDED;
    state.selected_slot = client->local_player->inventory->selected_hotbar_slot;

    uint8_t buf[64];
    size_t len = build_player_state(buf, &state);
    client_send_packet(client, NET_PACKET_PLAYER_STATE, buf, len);
}

void net_client_send_block_change(NetClient* client, int x, int y, int z,
                                   uint8_t block_type, uint8_t metadata) {
    if (!client || client->state != NET_STATE_CONNECTED) return;

    NetBlockChange change = {x, y, z, block_type, metadata, client->my_client_id};
    uint8_t buf[32];
    size_t len = build_block_change(buf, &change);
    client_send_packet(client, NET_PACKET_BLOCK_CHANGE, buf, len);
}

NetClientState net_client_get_state(NetClient* client) {
    return client ? client->state : NET_STATE_DISCONNECTED;
}

const char* net_client_get_error(NetClient* client) {
    return client ? client->error_message : "";
}

// ============================================================================
// NETWORK CONTEXT (HIGH-LEVEL API)
// ============================================================================

NetworkContext* network_create(void) {
    NetworkContext* ctx = (NetworkContext*)calloc(1, sizeof(NetworkContext));
    if (!ctx) return NULL;

    ctx->mode = NET_MODE_NONE;
    ctx->send_interval = 0.05f;  // 20 Hz

    printf("[NETWORK] Context created\n");
    return ctx;
}

void network_destroy(NetworkContext* ctx) {
    if (!ctx) return;

    network_disconnect(ctx);
    free(ctx);
    printf("[NETWORK] Context destroyed\n");
}

bool network_host(NetworkContext* ctx, uint16_t port, World* world, Player* player,
                  EntityManager* entities, float* time_of_day, float* day_speed) {
    if (!ctx) return false;

    network_disconnect(ctx);

    ctx->server = net_server_create(port, world, player, entities, time_of_day, day_speed);
    if (!ctx->server) return false;

    if (!net_server_start(ctx->server)) {
        net_server_destroy(ctx->server);
        ctx->server = NULL;
        return false;
    }

    ctx->mode = NET_MODE_HOST;
    printf("[NETWORK] Now hosting on port %d\n", port);
    return true;
}

bool network_join(NetworkContext* ctx, const char* ip, uint16_t port,
                  const char* player_name, World* world, Player* player,
                  EntityManager* entities, float* time_of_day) {
    if (!ctx) {
        printf("[NETWORK] network_join: ctx is NULL\n");
        return false;
    }

    printf("[NETWORK] network_join: starting join to %s:%d\n", ip, port);
    network_disconnect(ctx);

    ctx->client = net_client_create(player_name, world, player, entities, time_of_day);
    if (!ctx->client) {
        printf("[NETWORK] network_join: failed to create client\n");
        return false;
    }

    if (!net_client_connect(ctx->client, ip, port)) {
        printf("[NETWORK] network_join: net_client_connect failed, cleaning up\n");
        net_client_destroy(ctx->client);
        ctx->client = NULL;
        return false;
    }

    ctx->mode = NET_MODE_CLIENT;
    printf("[NETWORK] Joining %s:%d - SUCCESS\n", ip, port);
    return true;
}

void network_disconnect(NetworkContext* ctx) {
    if (!ctx) return;

    printf("[NETWORK] network_disconnect called (mode=%d, server=%p, client=%p)\n",
           ctx->mode, (void*)ctx->server, (void*)ctx->client);

    // Remove remote player entities
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        if (ctx->remote_entities[i]) {
            // Entity cleanup handled by entity manager
            ctx->remote_entities[i] = NULL;
        }
    }

    if (ctx->server) {
        net_server_destroy(ctx->server);
        ctx->server = NULL;
    }

    if (ctx->client) {
        net_client_destroy(ctx->client);
        ctx->client = NULL;
    }

    ctx->mode = NET_MODE_NONE;
    printf("[NETWORK] network_disconnect complete\n");
}

void network_update(NetworkContext* ctx, float dt) {
    if (!ctx) return;

    if (ctx->mode == NET_MODE_HOST && ctx->server) {
        net_server_poll(ctx->server, 0);

        ctx->send_timer += dt;
        if (ctx->send_timer >= ctx->send_interval) {
            ctx->send_timer = 0;
            net_server_broadcast_states(ctx->server);
        }
    }
    else if (ctx->mode == NET_MODE_CLIENT && ctx->client) {
        net_client_poll(ctx->client, 0);

        if (ctx->client->state == NET_STATE_CONNECTED) {
            ctx->send_timer += dt;
            if (ctx->send_timer >= ctx->send_interval) {
                ctx->send_timer = 0;
                net_client_send_player_state(ctx->client);
            }
        }
    }
}

void network_broadcast_block_change(NetworkContext* ctx, int x, int y, int z,
                                     uint8_t block_type, uint8_t metadata) {
    if (!ctx) return;

    if (ctx->mode == NET_MODE_HOST && ctx->server) {
        net_server_broadcast_block_change(ctx->server, x, y, z, block_type, metadata);
    }
    else if (ctx->mode == NET_MODE_CLIENT && ctx->client) {
        net_client_send_block_change(ctx->client, x, y, z, block_type, metadata);
    }
}

void network_update_remote_players(NetworkContext* ctx, float dt) {
    if (!ctx) return;

    if (ctx->mode == NET_MODE_HOST && ctx->server) {
        // Update entities for connected clients
        for (int i = 1; i < NET_MAX_CLIENTS; i++) {
            NetClientSlot* slot = &ctx->server->clients[i];

            if (slot->connected && slot->authenticated) {
                // Create entity if needed
                if (!ctx->remote_entities[i]) {
                    Vector3 pos = {slot->last_state.pos_x, slot->last_state.pos_y, slot->last_state.pos_z};
                    ctx->remote_entities[i] = block_human_spawn_colored(
                        ctx->server->entity_manager, pos,
                        (Color){255, 200, 150, 255},
                        (Color){100, 200, 100, 255},  // Green for clients
                        (Color){60, 60, 60, 255}
                    );
                    printf("[NETWORK] Spawned entity for client %d\n", i);
                }

                // Update entity position
                if (ctx->remote_entities[i]) {
                    Entity* e = ctx->remote_entities[i];
                    NetPlayerState* s = &slot->last_state;

                    // Interpolate
                    e->position.x += (s->pos_x - e->position.x) * dt * 15.0f;
                    e->position.y += (s->pos_y - e->position.y) * dt * 15.0f;
                    e->position.z += (s->pos_z - e->position.z) * dt * 15.0f;
                    e->rotation.y = s->yaw;
                }
            } else {
                // Remove entity if disconnected
                if (ctx->remote_entities[i]) {
                    ctx->remote_entities[i]->active = false;
                    ctx->remote_entities[i] = NULL;
                }
            }
        }
    }
    else if (ctx->mode == NET_MODE_CLIENT && ctx->client) {
        // Update entities for remote players
        for (int i = 0; i < NET_MAX_CLIENTS; i++) {
            if (i == ctx->client->my_client_id) continue;

            if (ctx->client->player_active[i]) {
                // Create entity if needed
                if (!ctx->remote_entities[i]) {
                    NetPlayerState* s = &ctx->client->remote_players[i];
                    Vector3 pos = {s->pos_x, s->pos_y, s->pos_z};

                    Color shirt = (i == 0) ?
                        (Color){200, 100, 100, 255} :  // Red for host
                        (Color){100, 200, 100, 255};   // Green for others

                    ctx->remote_entities[i] = block_human_spawn_colored(
                        ctx->client->entity_manager, pos,
                        (Color){255, 200, 150, 255},
                        shirt,
                        (Color){60, 60, 60, 255}
                    );
                    printf("[NETWORK] Spawned entity for player %d\n", i);
                }

                // Update entity position
                if (ctx->remote_entities[i]) {
                    Entity* e = ctx->remote_entities[i];
                    NetPlayerState* s = &ctx->client->remote_players[i];

                    // Interpolate
                    e->position.x += (s->pos_x - e->position.x) * dt * 15.0f;
                    e->position.y += (s->pos_y - e->position.y) * dt * 15.0f;
                    e->position.z += (s->pos_z - e->position.z) * dt * 15.0f;
                    e->rotation.y = s->yaw;
                }
            } else {
                // Remove entity
                if (ctx->remote_entities[i]) {
                    ctx->remote_entities[i]->active = false;
                    ctx->remote_entities[i] = NULL;
                }
            }
        }
    }
}

NetworkMode network_get_mode(NetworkContext* ctx) {
    return ctx ? ctx->mode : NET_MODE_NONE;
}

int network_get_player_count(NetworkContext* ctx) {
    if (!ctx) return 1;

    if (ctx->mode == NET_MODE_HOST && ctx->server) {
        return 1 + ctx->server->client_count;
    }
    else if (ctx->mode == NET_MODE_CLIENT && ctx->client) {
        int count = 1;
        for (int i = 0; i < NET_MAX_CLIENTS; i++) {
            if (ctx->client->player_active[i]) count++;
        }
        return count;
    }

    return 1;
}

bool network_is_player_active(NetworkContext* ctx, uint8_t client_id) {
    if (!ctx || client_id >= NET_MAX_CLIENTS) return false;

    if (ctx->mode == NET_MODE_HOST && ctx->server) {
        if (client_id == 0) return true;  // Host
        return ctx->server->clients[client_id].connected &&
               ctx->server->clients[client_id].authenticated;
    }
    else if (ctx->mode == NET_MODE_CLIENT && ctx->client) {
        if (client_id == ctx->client->my_client_id) return true;
        return ctx->client->player_active[client_id];
    }

    return false;
}

const char* network_get_player_name(NetworkContext* ctx, uint8_t client_id) {
    if (!ctx || client_id >= NET_MAX_CLIENTS) return "";

    if (ctx->mode == NET_MODE_HOST && ctx->server) {
        if (client_id == 0) return "Host";
        return ctx->server->clients[client_id].player_name;
    }
    else if (ctx->mode == NET_MODE_CLIENT && ctx->client) {
        if (client_id == ctx->client->my_client_id) return ctx->client->player_name;
        return ctx->client->player_names[client_id];
    }

    return "";
}
