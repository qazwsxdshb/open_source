#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/*
 * lwIP configuration for Pico SDK CYW43 + FreeRTOS.
 *
 * This project uses the socket API from a FreeRTOS task, so lwIP must run in
 * NO_SYS=0 mode.
 */

#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            1

#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
#define LWIP_DHCP                       1
#define LWIP_DNS                        1
#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_RAW                        1
#define LWIP_ICMP                       1

#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1
#define LWIP_SOCKET_SELECT              0
#define LWIP_SOCKET_POLL                0
#define LWIP_TIMEVAL_PRIVATE            0
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_SNDTIMEO                1

#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (24 * 1024)
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         4
#define MEMP_NUM_TCP_SEG                32
#define MEMP_NUM_NETCONN                8
#define MEMP_NUM_SYS_TIMEOUT            16

#define PBUF_POOL_SIZE                  24
#define PBUF_POOL_BUFSIZE               1536

#define TCP_MSS                         1460
#define TCP_WND                         (8 * TCP_MSS)
#define TCP_SND_BUF                     (8 * TCP_MSS)
#define TCP_SND_QUEUELEN                ((4 * TCP_SND_BUF + (TCP_MSS - 1)) / TCP_MSS)

#define TCPIP_MBOX_SIZE                 8
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         8

#define TCPIP_THREAD_STACKSIZE          1024
#define TCPIP_THREAD_PRIO               3
#define DEFAULT_THREAD_STACKSIZE        1024
#define DEFAULT_THREAD_PRIO             1

#define LWIP_STATS                      0
#define LWIP_DEBUG                      0

#endif
