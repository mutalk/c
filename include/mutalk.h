#ifndef MUTALK_LIBRARY_H
#define MUTALK_LIBRARY_H
#define MUTALK_PACKET_SIZE 508

#include <stdint.h>

#ifdef WITH_SYSNET
#include <netinet/in.h>
#endif

/**
 * default port for mutalk
 * @var mutalk_default_port
 */
static const uint16_t mutalk_default_port = 56778;


/**
 * Maximum packet size
 */
static const uint16_t mutalk_max_packet_size = MUTALK_PACKET_SIZE;

/**
 * Maximum payload size
 */
static const uint16_t mutalk_max_payload_size = MUTALK_PACKET_SIZE - 8;

/**
 * Mutalk message
 */
typedef uint8_t mutalk_message[MUTALK_PACKET_SIZE];

/**
 * create socket and binds to default port.
 * @return opened socket or -1
 */
int mutalk_open();

/**
 * calculates required address and send message
 * @param mu initialized by mutalk socket
 * @param channel channel number (unique id of message stream)
 * @param payload message payload
 * @param payload_size payload size up to #mutalk_max_packet_size
 * @return size of sent data or -1
 */
int mutalk_write(int mu, uint32_t channel, const uint8_t *payload, uint16_t payload_size);

/**
 * add socket to multicast group
 * @param mu initialized by mutalk socket
 * @param channels array of channels numbers to subscribe
 * @param channels_count count channels
 * @return number of subscribed channels or -1
 */
int mutalk_subscribe(int mu, const uint32_t *channels, uint32_t channels_count);

/**
 * remove socket from multicast group
 * @param mu initialized by mutalk socket
 * @param channels array of channels numbers to unsubscribe
 * @param channels_count count channels
 * @return number of unsubscribed channels or -1
 */
int mutalk_unsubscribe(int mu, const uint32_t *channels, uint32_t channels_count);

/**
 * Contains parse message
 */
typedef struct mutalk_packet {
    mutalk_message message; // raw message
    uint8_t *payload; // ref to payload
    uint32_t payload_size; // payload real size
    uint32_t channel; // channel number
#ifdef WITH_SYSNET
    struct in_addr source_ip; // Sender IP
    uint16_t source_port; // Sender port
#endif
} mutalk_packet;

/**
 * Read message from socket. Receive timeout can be set by usual socket recvto.
 * @param mu initialized by mutalk socket
 * @param channels array of channels numbers to filter. Must be the same as in subscribe
 * @param channels_count size of channels array in items
 * @param packet buffer for packet and parsing
 * @return size of payload (up to #mutalk_max_packet_size - 8) or -1
 */
int mutalk_read(int mu, const uint32_t *channels, uint32_t channels_count, mutalk_packet *packet);


#endif