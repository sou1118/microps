#include "icmp.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ip.h"
#include "util.h"

#define ICMP_BUFSIZ IP_PAYLOAD_SIZE_MAX

struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t sum;
    uint32_t values;
};

struct icmp_echo {
    uint8_t type;
    uint8_t code;
    uint16_t sum;
    uint16_t id;
    uint16_t seq;
};

static char *icmp_type_ntoa(uint8_t type) {
    switch (type) {
        case ICMP_TYPE_ECHOREPLY:
            return "EchoReply";
        case ICMP_TYPE_DEST_UNREACH:
            return "DestinationUnreachable";
        case ICMP_TYPE_SOURCE_QUENCH:
            return "SourceQuench";
        case ICMP_TYPE_REDIRECT:
            return "Redirect";
        case ICMP_TYPE_ECHO:
            return "Echo";
        case ICMP_TYPE_TIME_EXCEEDED:
            return "TimeExceeded";
        case ICMP_TYPE_PARAM_PROBLEM:
            return "ParameterProblem";
        case ICMP_TYPE_TIMESTAMP:
            return "Timestamp";
        case ICMP_TYPE_TIMESTAMPREPLY:
            return "TimestampReply";
        case ICMP_TYPE_INFO_REQUEST:
            return "InformationRequest";
        case ICMP_TYPE_INFO_REPLY:
            return "InformationReply";
    }
    return "Unknown";
}

static void icmp_dump(const uint8_t *data, size_t len) {
    struct icmp_hdr *hdr;
    struct icmp_echo *echo;

    flockfile(stderr);
    hdr = (struct icmp_hdr *)data;
    fprintf(stderr, "       type: %u (%s)\n", hdr->type,
            icmp_type_ntoa(hdr->type));
    fprintf(stderr, "       code: %u\n", hdr->code);
    fprintf(stderr, "        sum: 0x%04x\n", ntoh16(hdr->sum));
    switch (hdr->type) {
        case ICMP_TYPE_ECHOREPLY:
        case ICMP_TYPE_ECHO:
            echo = (struct icmp_echo *)hdr;
            fprintf(stderr, "         id: %u\n", ntoh16(echo->id));
            fprintf(stderr, "        seq: %u\n", ntoh16(echo->seq));
            break;
        default:
            fprintf(stderr, "     values: 0x%08x\n", ntoh32(hdr->values));
            break;
    }
#ifdef HEXDUMP
    hexdump(stderr, data, len);
#endif
    funlockfile(stderr);
}

void icmp_input(const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst,
                struct ip_iface *iface) {
    struct icmp_hdr *hdr;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[IP_ADDR_STR_LEN];

    if (len < sizeof(*hdr)) {
        errorf("too short, len=%d", len);
        return;
    }

    hdr = (struct icmp_hdr *)data;

    if (cksum16((uint16_t *)data, len, 0) != 0) {
        uint16_t sum = ntoh16(hdr->sum);
        uint16_t verify = ntoh16(cksum16((uint16_t *)data, len, -hdr->sum));

        errorf("checksum error, sum=0x%04x, verify=0x%04x", sum, verify);
        return;
    }

    debugf("%s => %s, len=%zu", ip_addr_ntop(src, addr1, sizeof(addr1)),
           ip_addr_ntop(dst, addr2, sizeof(addr2)), len);
    icmp_dump(data, len);
    switch (hdr->type) {
        case ICMP_TYPE_ECHO:
            /* Responds with the address of the received interface. */
            // The ICMP header is at the start of the packet.

            // The ICMP payload is after the header.
            uint8_t *payload = (uint8_t *)(hdr + 1);

            // The length of the ICMP payload is the length of the
            // packet minus the length of the header.
            uint32_t payload_len = len - sizeof(*hdr);

            // Send the ICMP packet.
            icmp_output(ICMP_TYPE_ECHOREPLY, hdr->code,
                        hdr->values, payload, payload_len,
                        iface->unicast, src);

            break;
        default:
            /* ignore */
            break;
    }
}

int icmp_output(uint8_t type, uint8_t code, uint32_t values,
                const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst) {
    uint8_t buf[ICMP_BUFSIZ];
    struct icmp_hdr *hdr;
    size_t msg_len;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[IP_ADDR_STR_LEN];

    hdr = (struct icmp_hdr *)buf;
    /* 1. Initialize the ICMP header. */
    hdr->type = type;
    hdr->code = code;
    hdr->sum = 0;
    hdr->values = values;

    /* 2. Copy the ICMP message to the packet. */
    memcpy(hdr + 1, data, len);

    /* 3. Compute the ICMP checksum. */
    msg_len = sizeof(*hdr) + len;
    hdr->sum = cksum16((uint16_t *)hdr, msg_len, 0);

    /* 4. Send the packet. */
    debugf("%s => %s, len=%zu", ip_addr_ntop(src, addr1, sizeof(addr1)),
           ip_addr_ntop(dst, addr2, sizeof(addr2)), msg_len);
    icmp_dump(buf, msg_len);
    return ip_output(IP_PROTOCOL_ICMP, buf, msg_len, src, dst);
}

int icmp_init(void) {
    if (ip_protocol_register(IP_PROTOCOL_ICMP, icmp_input) == -1) {
        errorf("ip_protocol_register() failure");
        return -1;
    }
    return 0;
}
