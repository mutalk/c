#include "mutalk.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

#ifdef DEBUG

#include "stdio.h"

#define LOG(format, ...) do { fprintf(stderr,"(debug) [%s:%i] " format "\n", __func__,__LINE__, __VA_ARGS__); fflush(stderr); } while(0)
#else
#define LOG(format, ...)
#endif


static const uint32_t mutalk_begin_addr = 4009754625;
static const uint32_t mutalk_address_limit = 16777215;
static const uint8_t mutalk_magic[] = {0x4d, 0x54, 0x4c, 0x4b};
static const uint32_t mutalk_magic_size = sizeof(mutalk_magic) / sizeof(mutalk_magic[0]);

/**
 * returns real address in network-byte order
 */
uint32_t mutalk_addr(uint32_t channel) {
    return htonl(mutalk_begin_addr + (channel % mutalk_address_limit));
}


in_addr_t mutalk_get_first_interface_addr(const char *interface) {
    struct ifaddrs *ifaddr, *ifa;
    int rc;
    in_addr_t res = 0;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        return NULL;
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        rc = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if ((strcmp(ifa->ifa_name, interface) == 0) && (ifa->ifa_addr->sa_family == AF_INET)) {
            if (rc != 0) {
                break;
            }
            res = ((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return res;
}

int mutalk_open() {
    const int optval = 1;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(mutalk_default_port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    int rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (rc == -1) {
        close(fd);
        return -1;
    }
    if (bind(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) < 0) {
        close(fd);
        return -1;
    }
    LOG("socket opened at %s:%i", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
    return fd;
}

int mutalk_write(int mu, uint32_t channel, const uint8_t *payload, uint16_t payload_size) {
    mutalk_message message = {0};
    if (payload_size > mutalk_max_payload_size) {
        payload_size = mutalk_max_payload_size;
    }
    uint32_t net_channel = htonl(channel);
    const uint8_t *channel_data = ((const uint8_t *) &net_channel);
    memcpy(message, mutalk_magic, mutalk_magic_size);
    memcpy(message + mutalk_magic_size, channel_data, sizeof(channel));
    memcpy(message + mutalk_magic_size + sizeof(channel), payload, payload_size);

    uint32_t packet_size = mutalk_magic_size + sizeof(channel) + payload_size;

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(mutalk_default_port);
    sin.sin_addr.s_addr = mutalk_addr(channel);
    LOG("write to channel %i (%s) %i bytes, payload %i bytes", channel, inet_ntoa(sin.sin_addr), packet_size,
        payload_size);
    return (int) sendto(mu, (const void *) message, packet_size, 0, (const struct sockaddr *) &sin, sizeof(sin));
}

int mutalk_subscribe(int mu, const uint32_t *channels, uint32_t channels_count) {
    struct ip_mreq mreq;
    struct in_addr addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    LOG("interface address %s", inet_ntoa(mreq.imr_interface));
    for (uint32_t i = 0; i < channels_count; ++i) {
        uint32_t channel = channels[i];
        addr.s_addr = mutalk_addr(channel);
        mreq.imr_multiaddr = addr;

        LOG("subscribe channel %i - %s", channel, inet_ntoa(addr));
        if (setsockopt(mu, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            return -1;
        }
    }
    return channels_count;
}

int mutalk_unsubscribe(int mu, const uint32_t *channels, uint32_t channels_count) {
    struct ip_mreq mreq;
    struct in_addr addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    for (uint32_t i = 0; i < channels_count; ++i) {
        uint32_t channel = channels[i];
        addr.s_addr = mutalk_addr(channel);
        mreq.imr_multiaddr = addr;
        LOG("unsubscribe channel %i - %s", channel, inet_ntoa(addr));
        if (setsockopt(mu, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            return -1;
        }
    }
    return channels_count;
}

int mutalk_read(int mu, const uint32_t *channels, uint32_t channels_count, mutalk_packet *packet) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    while (1) {
        ssize_t message = recvfrom(mu, packet->message, mutalk_max_packet_size, 0, (struct sockaddr *) &addr, &len);
        if (message == -1) {
            return -1;
        }
        if (message < mutalk_max_packet_size - mutalk_max_payload_size) {
            // no headers
            LOG("skip message: no headers. got %li", message);
            continue;
        }
        if (memcmp(packet->message, mutalk_magic, mutalk_magic_size) != 0) {
            // wrong magic
            LOG("skip message: wrong magic. got %li", message);
            continue;
        }

        packet->channel = ntohl(*((uint32_t *) (packet->message + mutalk_magic_size)));
        packet->payload = packet->message + mutalk_magic_size + sizeof(packet->channel);
        LOG("read from channel %u %li bytes", packet->channel, message);
        // check channel
        char matched = 0;
        for (uint32_t i = 0; i < channels_count; ++i) {
            if (channels[i] == packet->channel) {
                matched = 1;
                break;
            }
        }
        if (!matched) {
            // spam
            LOG("skip message: unknown channel %ui", packet->channel);
            continue;
        }
        packet->payload_size = message - mutalk_magic_size - sizeof(packet->channel);
        LOG("payload: %u bytes, channel: %ui", packet->payload_size, packet->channel);
#ifdef WITH_SYSNET
        packet->source_port = ntohs(addr.sin_port);
        packet->source_ip = addr.sin_addr;
#endif
        return (int) message;
    }
}
