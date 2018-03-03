//
// Created by reddec on 02.03.18.
//
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>
#include <sys/socket.h>
#include "mutalk.h"

static void assert(bool condition, const char *text) {
    if (!condition) {
        perror(text);
        fflush(stderr);
        exit(1);
    }
}

static uint32_t channels[] = {1, 2, 123456};
static char payload[] = "Hell in world";

void *thread_read(void *reader) {
    int mu = *((int *) reader);
    mutalk_packet packet;

    int rc = mutalk_read(mu, channels, sizeof(channels), &packet);
    assert(rc != -1, "read message");
    assert(rc == strlen(payload) + 8, "payload size");
    assert(memcmp(payload, packet.payload, strlen(payload)) == 0, "compare incoming and sent message");
    assert(packet.channel == 123456, "channel number");

    return NULL;
}

int main(int argc, char **argv) {


    int writer = mutalk_open();
    assert(writer != -1, "open mu socket (writer)");

    int reader = mutalk_open();
    assert(reader != -1, "open mu socket (reader)");

    int rc = mutalk_subscribe(reader, channels, sizeof(channels) / sizeof(channels[0]));
    assert(rc != -1, "subscribe");
    assert(rc == sizeof(channels) / sizeof(channels[0]), "subscribe channels number");

    struct timeval tv;
    tv.tv_sec = 4;
    tv.tv_usec = 0;
    rc = setsockopt(reader, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
    assert(rc == 0, "set recv timeout");

    pthread_t read_thread;
    rc = pthread_create(&read_thread, NULL, thread_read, &reader);
    assert(rc != -1, "create reader thread");

    sleep(1);

    rc = mutalk_write(writer, 123456, (const uint8_t *) payload, (uint16_t) strlen(payload));
    assert(rc != -1, "write");
    assert(rc == strlen(payload) + 8, "write size");

    rc = pthread_join(read_thread, NULL);
    assert(rc == 0, "join thread");

    rc = mutalk_unsubscribe(reader, channels, sizeof(channels) / sizeof(channels[0]));
    assert(rc != -1, "unsubscribe");
    assert(rc == sizeof(channels) / sizeof(channels[0]), "unsubscribe channels number");

    close(writer);
    close(reader);
}