#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/ip_addr.h"

#include "e1000.h"
#include "mmu.h"
#include "defs.h"

static struct netif netif;

static char init_done = 0;

static err_t low_level_output(struct netif *netif, struct pbuf *p) {
    uint8_t *buf = (uint8_t*)kalloc(); 
    int buf_len = 0;

    struct pbuf *q = p;
    int q_offset = 0;

    while (q != NULL) {
        int copy_len = q->len - q_offset;
        if (buf_len + copy_len > PGSIZE) {
            copy_len = PGSIZE - buf_len;
        }

        memmove(buf + buf_len, (uint8_t*)q->payload + q_offset, copy_len);
        buf_len += copy_len;
        q_offset += copy_len;

        if (buf_len == PGSIZE) {
            e1000_transmit(buf, buf_len);
            buf_len = 0;
        }

        if (q_offset == q->len) {
            q = q->next;
            q_offset = 0;
        }
    }

    if (buf_len > 0) {
        e1000_transmit(buf, buf_len);
    }

    kfree((char *)buf);
    return ERR_OK;
}

static err_t netif_init_cb(struct netif *netif) {
    uint8_t mac[6];
    e1000_get_mac(mac);

    netif->hwaddr_len = 6;
    memmove(netif->hwaddr, mac, 6);
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    return ERR_OK;
}

void lwip_stack_init(void) {
    lwip_init();
    dns_init();

    struct ip4_addr ipaddr, netmask, gw, dns;
    uint8_t ip[4];
    e1000_get_ip(ip);

    IP4_ADDR(&ipaddr, ip[0], ip[1], ip[2], ip[3]);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, ip[0], ip[1], ip[2], 1);
    IP4_ADDR(&dns, 8, 8, 8, 8);

    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, netif_init_cb, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

    dns_setserver(0, &dns);

    cprintf("lwIP initialized: IP=%d.%d.%d.%d GW=%d.%d.%d.%d DNS=%d.%d.%d.%d\n",
        ip[0], ip[1], ip[2], ip[3],
        ip[0], ip[1], ip[2], 1,
        8, 8, 8, 8
    );

    init_done = 1;
}

extern void http_check_timeouts(void);

void lwip_poll(void) {
    if (!init_done) return;

    uint8_t *buf = (uint8_t*)kalloc();

    int len = e1000_receive(buf, 4096);
    if (len > 0) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (p != NULL) {
            pbuf_take(p, buf, len);
            if (netif.input(p, &netif) != ERR_OK)
                pbuf_free(p);
        }
    }

    kfree((char *)buf);

    sys_check_timeouts();
    http_check_timeouts();
}

int
sys_lwip_poll(void)
{
    lwip_poll();
    return 0;
}