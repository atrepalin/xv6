#include "types.h"
#include "user.h"

#define printf(...) printf(1, __VA_ARGS__)

struct eth_hdr {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
};

struct arp_hdr {
    uint16_t htype;      
    uint16_t ptype;      
    uint8_t hlen;        
    uint8_t plen;        
    uint16_t op;         

    uint8_t sha[6];      
    uint8_t spa[4];      
    uint8_t tha[6];      
    uint8_t tpa[4];      
};

struct ip_hdr {
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint8_t saddr[4];
    uint8_t daddr[4];
};

struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
};

static inline uint16_t htons(uint16_t x) {
    return (uint16_t)((x << 8) | (x >> 8));
}
static inline uint16_t ntohs(uint16_t x) { return htons(x); }

static inline uint32_t htonl(uint32_t x) {
    return ((x & 0x000000FFU) << 24) |
           ((x & 0x0000FF00U) <<  8) |
           ((x & 0x00FF0000U) >>  8) |
           ((x & 0xFF000000U) >> 24);
}

static inline uint32_t ntohl(uint32_t x) { return htonl(x); }

static uint16_t checksum(void *vdata, size_t length) {
    uint8_t *data = vdata;
    uint64_t acc = 0;
    for (size_t i = 0; i + 1 < length; i += 2)
        acc += (data[i] << 8) + data[i+1];
    if (length & 1)
        acc += data[length-1] << 8;
    while (acc >> 16)
        acc = (acc & 0xffff) + (acc >> 16);
    return ~acc;
}

char* parse_payload(uint8_t *payload, int payload_len) {
    static char tmp[65];
    int i;

    for(i = 0; i < payload_len && i < 64; i++) {
        if(payload[i] >= 32 && payload[i] <= 126)
            tmp[i] = payload[i];
        else
            tmp[i] = '.';
    }
    
    tmp[i] = 0;
    return tmp;
}

uint8_t mac[6];
uint8_t ipv4[4];

void net_poll(void) {
    static uint8_t buf[2048];
    static uint8_t reply_buf[256];

    memset(buf, 0, sizeof(buf));
    memset(reply_buf, 0, sizeof(reply_buf));

    if (receive(buf, sizeof(buf)) < sizeof(struct eth_hdr))
        return;

    struct eth_hdr *eth = (struct eth_hdr*)buf;
    int offset = 0;

    switch (ntohs(eth->type)) {
        case 0x0800:
            struct ip_hdr *ip = (struct ip_hdr*)(buf + sizeof(*eth));
            if (ip->proto != 1) 
                return;

            struct icmp_hdr *icmp = (struct icmp_hdr*)(buf + sizeof(*eth) + sizeof(*ip));
            uint8_t *payload = (uint8_t*)(icmp + 1);
            int payload_len = ntohs(ip->tot_len) - sizeof(*ip) - sizeof(*icmp);

            if (icmp->type == 8 && icmp->code == 0) {
                printf("ICMP Echo request from %d.%d.%d.%d (id=%d seq=%d), payload='%s'\n",
                    ip->saddr[0], ip->saddr[1], ip->saddr[2], ip->saddr[3],
                    ntohs(icmp->id), ntohs(icmp->seq), parse_payload(payload, payload_len));

                struct eth_hdr reply_eth = *eth;
                for (int i = 0; i < 6; i++) {
                    reply_eth.dst[i] = eth->src[i];
                    reply_eth.src[i] = eth->dst[i];
                }

                struct ip_hdr reply_ip = *ip;
                for (int i = 0; i < 4; i++) {
                    reply_ip.daddr[i] = ip->saddr[i];
                    reply_ip.saddr[i] = ip->daddr[i];
                }

                reply_ip.checksum = 0;
                reply_ip.checksum = checksum((uint16_t*)&reply_ip, sizeof(*ip));

                struct icmp_hdr reply_icmp = *icmp;
                reply_icmp.type = 0;
                reply_icmp.checksum = 0;
                reply_icmp.checksum = checksum((uint16_t*)&reply_icmp, sizeof(*icmp));

                memmove(reply_buf, &reply_eth, sizeof(reply_eth));
                offset += sizeof(reply_eth);

                memmove(reply_buf + offset, &reply_ip, sizeof(reply_ip));
                offset += sizeof(reply_ip);

                memmove(reply_buf + offset, &reply_icmp, sizeof(reply_icmp));
                offset += sizeof(reply_icmp);

                memmove(reply_buf + offset, payload, payload_len);

                transmit(reply_buf, offset + payload_len);
            }
        break;

        case 0x0806:
            struct arp_hdr *arp = (struct arp_hdr*)(buf + sizeof(*eth));

            if (ntohs(arp->op) == 1) {
                printf("ARP request for IP %d.%d.%d.%d\n",
                    arp->tpa[0], arp->tpa[1], arp->tpa[2], arp->tpa[3]);

                for (int i = 0; i < 4; i++) {
                    if (ipv4[i] != arp->tpa[i]) {
                        return;
                    }
                }

                struct eth_hdr reply_eth;
                for (int i = 0; i < 6; i++) {
                    reply_eth.dst[i] = eth->src[i];
                    reply_eth.src[i] = mac[i];
                }
                reply_eth.type = htons(0x0806);

                struct arp_hdr reply_arp = *arp;
                reply_arp.op = htons(2);

                for (int i = 0; i < 6; i++) {
                    reply_arp.tha[i] = arp->sha[i];
                    reply_arp.sha[i] = mac[i];
                }

                for (int i = 0; i < 4; i++) {
                    reply_arp.tpa[i] = arp->spa[i];
                    reply_arp.spa[i] = arp->tpa[i];
                }

                memmove(reply_buf, &reply_eth, sizeof(reply_eth));
                offset += sizeof(reply_eth);

                memmove(reply_buf + offset, &reply_arp, sizeof(reply_arp));
                offset += sizeof(reply_arp);

                transmit(reply_buf, offset);
            }
        break;

        default:
            break;
    }
}

int main() {
    printf("Starting pingpong...\n");

    get_mac(mac);
    get_ip(ipv4);

    printf("Listening on %d.%d.%d.%d\n", ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
    printf("MAC: %x:%x:%x:%x:%x:%x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    while (1) {
        net_poll();
    }

    exit();
}