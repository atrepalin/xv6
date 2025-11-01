#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS                          1
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_IPV4                       1
#define LWIP_ICMP                       1
#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_DHCP                       0
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0
#define LWIP_STATS                      0
#define LWIP_TIMEVAL_PRIVATE            0

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        16000
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_TCP_PCB                5
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_SYS_TIMEOUT            8

#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               512

#define LWIP_DEBUG                      0

#endif /* LWIPOPTS_H */
