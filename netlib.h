/*
+------------------------------------------------------+
|                                                      |
|    Adam Barborík's networking header-only library    |
|                                                      |
|    currently supporting:                             |
|    * POSIX                                           |
|    * WINDOWS - link with -lws2_32                    |
+------------------------------------------------------+
*/

#ifndef __NETLIB_
#define __NETLIB_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#endif

typedef struct
{
    int socket;
    char port[8];
    char ipaddr[16];
} sock_t;

enum
{
    NLIB_TCP,
    NLIB_UDP,
    NLIB_SERVER,
    NLIB_CLIENT,
};

/*
    last netlib error
    unique for each compilation unit since this is a header file
*/
static char nlib_msg[512];

/*
    call this first!
*/
static int nlib_init();

/*
    make socket non-blocking

    [IN] sock_t *sock | socket to be made non-blocking
*/
static int nlib_noblock(sock_t *sock);

/*
    sock_t constructor

    [OUT] sock_t *sock | socket you want to initialize
    [IN]  char* port   | port
    [IN]  int type     | NLIB_CLIENT or NLIB_SERVER
    [IN]  int proto    | NLIB_TCP r NLIB_UDP

    returns 1 on success and 0 on error
*/
static int nlib_mksock(sock_t *sock, char *port, int type, int proto);

/*
    free a socket

    [IN] sock_t *sock | socket to free

    returns 1 on success and 0 on error
*/
static int nlib_free(sock_t *sock);

/*
    connect to a socket

    [IN]  sock_t *sock  | unconnected socket
    [IN]  char* target  | either ip address or domain name
    [IN]  char* port    | port

    returns 1 on success and 0 on error
*/
static int nlib_connect(sock_t *sock, char *host, char *port);

/*
    accept a connection

    [IN] sock_t *server | listening socket
    [IN] sock_t *client | connecting socket

    returns 1 on success and 0 on error
*/
static int nlib_accept(sock_t *server, sock_t *client);

/*
    receive data from socket

    [IN]  sock_t *sock | socket from which to receive data
    [OUT] char* buf    | receive buffer
    [IN]  int len      | length of receive buffer

    returns number of bytes received or 0 on error
    in non-blocking mode 0 is returned when the socket would block
    -1 bytes received means the socket disconnected
*/
static int nlib_recv(sock_t *sock, char *buf, int len);

/*
    send data to socket

    [IN]  sock_t *sock | socket where to send data
    [IN]  char* buf    | send buffer
    [IN]  int len      | length of send buffer

    returns number of bytes sent or 0 on error
*/
static int nlib_send(sock_t *sock, char *buf, int len);

/*
    receive an UDP datagram from IPADDR:PORT

    [IN]  sock_t *sock | bound socket
    [OUT] char* ipaadr | ip address of the sender
    [OUT] char* port   | port of the sender
    [OUT] char* buf    | receive buffer
    [IN]  int len      | length of the buffer

    returns number of bytes received or 0 on error
    in non-blocking mode 0 is returned when the socket would block
*/
static int nlib_recvfrom(sock_t *sock, char *ipaddr, char *port, char *buf, int len);

/*
    send a datagram directly to IPADDR:PORT destination on an UDP socket

    [IN]  sock_t *sock  | socket from which to send
    [IN]  char* ipaadr  | ip address where to send the datagram
    [IN]  char* port    | port where to send the datagram
    [IN]  char* buf     | send buffer
    [IN]  int len       | length of the buffer

    returns number of bytes sent or 0 on error
*/
static int nlib_sendto(sock_t *sock, char *ipaddr, char *port, char *buf, int len);

/*
    set a timeout on a socket

    [IN]  sock_t *sock | socket to set the timeout on
    [IN]  int ms       | number of milliseconds before timeout
*/
static int nlib_timeo(sock_t *sock, int ms);

// helper function used by this library
static void __func_fail(char *func)
{
#if defined(_WIN32)
    int err = WSAGetLastError();
#else
    int err = errno;
#endif

    sprintf(nlib_msg, "ERROR: %s failed with code : %d", func, err);
}

#if defined(_WIN32)

static int nlib_init()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa))
    {
        __func_fail("WSAStartup()");
        return 0;
    }

    nlib_msg[0] = 0;
    return 1;
}

static int nlib_noblock(sock_t *sock)
{
    u_long arg = 1;
    if (ioctlsocket(sock->socket, FIONBIO, &arg))
    {
        __func_fail("ioctlsocket()");
        return 0;
    }
}

#else

static int nlib_init()
{
    nlib_msg[0] = 0;
    return 1;
}

static int nlib_noblock(sock_t *sock)
{
    if (fcntl(sock->socket, F_SETFL, O_NONBLOCK) < 0)
    {
        __func_fail("fcntl()");
        return 0;
    }

    return 1;
}

#endif

static int nlib_mksock(sock_t *sock, char *port, int type, int proto)
{
    strcpy(sock->port, port);

    // creating socket
    if (proto == NLIB_TCP) sock->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proto == NLIB_UDP) sock->socket = socket(AF_INET, SOCK_DGRAM,  IPPROTO_UDP);

    if (sock->socket < 0)
    {
        __func_fail("socket()");
        return 0;
    }

    if (type == NLIB_SERVER)
    {
        // binding socket
        struct sockaddr_in sin = {0};

        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(atoi(port));

        if (bind(sock->socket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            __func_fail("bind()");
            return 0;
        }

        // listen on socket
        if (proto == NLIB_TCP && listen(sock->socket, 1) < 0)
        {
            __func_fail("listen()");
            return 0;
        }
    }

    return 1;
}

static int nlib_free(sock_t *sock)
{
#if defined(_WIN32)
    if (closesocket(sock->socket) < 0)
    {
        __func_fail("closesocket()");
        return 0;
    }
#else
    if (close(sock->socket) < 0)
    {
        __func_fail("close()");
        return 0;
    }
#endif
    return 1;
}

static int nlib_connect(sock_t *sock, char *host, char *port)
{
    struct in_addr addr;
    struct sockaddr_in sin = {0};
    struct hostent *hostent = gethostbyname(host);

    addr.s_addr = *(uint64_t *)hostent->h_addr_list[0];
    strcpy(sock->ipaddr, inet_ntoa(addr));
    strcpy(sock->port, port);

    sin.sin_addr = addr;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(atoi(port));

    if (connect(sock->socket, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        __func_fail("connect()");
        return 0;
    }

    return 1;
}

static int nlib_accept(sock_t *server, sock_t *client)
{
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);

    client->socket = accept(server->socket, (struct sockaddr *)&sin, &len);

    if (client->socket < 0)
    {
        __func_fail("accept()");
        return 0;
    }

    strcpy(client->ipaddr, inet_ntoa(sin.sin_addr));
    sprintf(client->port, "%d", ntohs(sin.sin_port));
    return 1;
}

static int nlib_recv(sock_t *sock, char *buf, int len)
{
    int recvd = recv(sock->socket, buf, len, 0);

#if defined(_WIN32)
    if (WSAGetLastError() == WSAECONNRESET)
    {
        return -1;
    }
#else
    if (!recvd)
    {
        return -1;
    }
#endif

    if (recvd < 0)
    {
        __func_fail("recv()");
        return 0;
    }

    return recvd;
}

static int nlib_send(sock_t *sock, char *buf, int len)
{
    int sent = send(sock->socket, buf, len, 0);

    if (sent < 0)
    {
        __func_fail("send()");
        return 0;
    }

    return sent;
}

static int nlib_recvfrom(sock_t *sock, char *ipaddr, char *port, char *buf, int len)
{
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);

    int recvd = recvfrom(sock->socket, buf, len, 0, (struct sockaddr *)&sin, &slen);

    if (recvd < 0)
    {
        __func_fail("recvfrom()");
        return 0;
    }

    strcpy(ipaddr, inet_ntoa(sin.sin_addr));
    sprintf(port, "%d", ntohs(sin.sin_port));

    return recvd;
}

static int nlib_sendto(sock_t *sock, char *ipaddr, char *port, char *buf, int len)
{
    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(atoi(port));
    inet_pton(AF_INET, ipaddr, &(sin.sin_addr));

    int sent = sendto(sock->socket, buf, len, 0, (struct sockaddr *)&sin, sizeof(sin));

    if (sent < 0)
    {
        __func_fail("sendto()");
        return 0;
    }

    return sent;
}

static int nlib_timeo(sock_t *sock, int ms)
{
#if defined(_WIN32)
    DWORD timeout = ms;
    setsockopt(sock->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#else
    #include <sys/time.h>
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ms * 1000;
    setsockopt(sock->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

#endif
