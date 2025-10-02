#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Minimal lwIP configuration compatible with pico_cyw43_arch_lwip_threadsafe_background

#define NO_SYS                          1
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0

#define LWIP_IPV4                       1
#define LWIP_ARP                        1
#define LWIP_ICMP                       1
#define LWIP_DHCP                       1
#define LWIP_DNS                        1
#define LWIP_UDP                        1
#define LWIP_TCP                        1

// Enable SNTP for NTP time synchronization
#define LWIP_SNTP                       1
#define SNTP_SERVER_DNS                 1
#define SNTP_SET_SYSTEM_TIME_US(sec, us) do { \
    time_t t = (sec); \
    if (t > 1600000000) { \
        struct timeval tv = { .tv_sec = t, .tv_usec = (us) }; \
        settimeofday(&tv, NULL); \
    } \
} while(0)

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        8000

#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                5
#define MEMP_NUM_TCP_SEG                32
#define MEMP_NUM_SYS_TIMEOUT            8

#define TCP_TTL                         255
#define TCP_QUEUE_OOSEQ                 0
#define TCP_MSS                         1460
#define TCP_SND_BUF                     (2 * TCP_MSS)
#define TCP_SND_QUEUELEN                ((4 * TCP_SND_BUF) / TCP_MSS)
#define TCP_WND                         (2 * TCP_MSS)

#define PBUF_POOL_SIZE                  16

#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1

// Disable statistics to reduce code size
#define LWIP_STATS                      0
#define LWIP_PROVIDE_ERRNO              1

#endif // _LWIPOPTS_H


