#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "common.h"

int get_int(const char str[], bool *ok)
{
    assert(str);
    assert(ok);

    int value;
    int ret = sscanf(str, "0x%x", &value);

    /* If couldn't convert from hex, try dec */
    if (0 >= ret) {
        ret = sscanf(str, "%d", &value);
    }

    *ok = (0 < ret);

    return value;
}

static int set_rtu_param(void *backend, char c, char *value)
{
    rtu_backend_t *params = (rtu_backend_t *)backend;
    bool ok = true;

    switch (c)
    {
    case 'b':
    {
        params->baud = get_int(value, &ok);
        if (!ok) {
            printf("Baudrate is invalid %s\n", value);
            ok = false;
        }
    }
    break;
    case 'd':
    {
        int db = get_int(value, &ok);
        if (!ok || (7 != db && 8 != db)) {
            printf("Data bits incorrect (%s)\n", value);
            ok = false;
        } else {
            params->data_bits = db;
        }
    }
    break;
    case 's':
    {
        int sb = get_int(value, &ok);
        if (!ok || (1 != sb && 2 != sb)) {
            printf("Stop bits incorrect (%s)\n", value);
            ok = false;
        } else {
            params->stop_bits = sb;
        }
    }
    break;
    case 'p':
    {
        if (0 == strcmp(value, "none")) {
            params->parity = 'N';
        } else if (0 == strcmp(value, "even")) {
            params->parity = 'E';
        } else if (0 == strcmp(value, "odd")) {
            params->parity = 'O';
        } else {
            printf("Unrecognized parity (%s)\n", value);
            ok = false;
        }
    }
    break;
    default:
        printf("Unknow RTU param (%c: %s)\n\n", c, value);
        ok = false;
    }
    return ok;
}

static modbus_t *create_rtu_ctx(void *backend)
{
    rtu_backend_t *rtu = (rtu_backend_t *)backend;
    modbus_t *ctx = modbus_new_rtu(rtu->dev_name, rtu->baud, rtu->parity, 
                                   rtu->data_bits, rtu->stop_bits);
    return ctx;
}

static void delete_rtu(void *backend)
{
    rtu_backend_t *rtu = (rtu_backend_t *)backend;
    free(rtu);
}

static int listen_for_rtu_conn(void *backend, modbus_t *ctx)
{
    (void)backend;
    (void)ctx;

    printf("Connecting ...\n");
    return (0 == modbus_connect(ctx));
}

static void close_rtu_conn(void *backend)
{
    (void)backend;
}

backend_params_t *create_rtu_backend(void)
{
    rtu_backend_t *rtu = (rtu_backend_t *)malloc(sizeof(rtu_backend_t));
    rtu->base.type = RTU_T;
    rtu->base.set_param = set_rtu_param;
    rtu->base.create_ctx = create_rtu_ctx;
    rtu->base.listen_for_conn = listen_for_rtu_conn;
    rtu->base.close_conn = close_rtu_conn;
    rtu->base.del = delete_rtu;

    strcpy(rtu->dev_name, "");
    rtu->baud = 9600;
    rtu->data_bits = 8;
    rtu->stop_bits = 1;
    rtu->parity = 'E';

    return (backend_params_t *)rtu;
}

static int set_tcp_param(void *backend, char c, char *value)
{
    tcp_backend_t *tcp = (tcp_backend_t *)backend;
    bool ok = true;

    switch (c)
    {
    case 'p':
    {
        tcp->port = get_int(value, &ok);
        if (!ok) {
            printf("Port parameter %s is not integer\n", value);
        }
    }
    break;
    default:
        printf("Unknown TCP param (%c: %s)\n", c, value);
        ok = false;
    }

    return ok;
}

static modbus_t *create_tcp_ctx(void *backend)
{
    tcp_backend_t *tcp = (tcp_backend_t *)backend;
    modbus_t *ctx = modbus_new_tcp(tcp->ip, tcp->port);

    return ctx;
}

static void delete_tcp(void *backend)
{
    tcp_backend_t *tcp = (tcp_backend_t *)backend;
    if (tcp)
        free(tcp);
}

static int listen_for_tcp_conn(void *backend, modbus_t *ctx)
{
    tcp_backend_t *tcp = (tcp_backend_t *)backend;
    tcp->client_socket = modbus_tcp_listen(ctx, 1);
    if (-1 == tcp->client_socket) {
        printf("Listen return %d (%s)\n", tcp->client_socket, modbus_strerror(errno));
        return 0;
    }
    modbus_tcp_accept(ctx, &(tcp->client_socket));
    return 1;
}

static void close_tcp_conn(void *backend)
{
    tcp_backend_t *tcp = (tcp_backend_t *)backend;
    if (tcp->client_socket != -1) {
        close(tcp->client_socket);
        tcp->client_socket = -1;
    }
}

backend_params_t *create_tcp_backend(void)
{
    tcp_backend_t *tcp = (tcp_backend_t *)malloc(sizeof(tcp_backend_t));
    tcp->client_socket = -1;
    tcp->base.set_param = set_tcp_param;
    tcp->base.create_ctx = create_tcp_ctx;
    tcp->base.del = delete_tcp;
    tcp->base.listen_for_conn = listen_for_tcp_conn;
    tcp->base.close_conn = close_tcp_conn;

    tcp->base.type = TCP_T;
    strcpy(tcp->ip, "0.0.0.0");
    tcp->port = 502;

    return (backend_params_t *)tcp;
}
