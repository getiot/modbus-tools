#ifndef __MODBUS_TOOLS_COMMON_H__
#define __MODBUS_TOOLS_COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include "modbus.h"

#define DebugOpt     "debug"
#define TcpOptVal    "tcp"
#define RtuOptVal    "rtu"

typedef enum
{
    FuncNone               = -1,
    ReadCoils              = 0x01,
    ReadDiscreteInput      = 0x02,
    ReadHoldingRegisters   = 0x03,
    ReadInputRegisters     = 0x04,
    WriteSingleCoil        = 0x05,
    WriteSingleRegister    = 0x06,
    WriteMultipleCoils     = 0x0F,
    WriteMultipleRegisters = 0x10
} function_code_t;

typedef enum
{
    NONE,
    RTU_T,
    TCP_T
} conn_type_t;

typedef struct
{
    conn_type_t type;
    void (*del)(void *backend);

    /* common client/server functions */
    int (*set_param)(void *backend, char c, char *value);
    modbus_t *(*create_ctx)(void *backend);

    /* server functions */
    int (*listen_for_conn)(void *backend, modbus_t *ctx);
    void (*close_conn)(void *backend);

} backend_params_t;

typedef struct
{
    backend_params_t base;
    char dev_name[32];
    uint32_t baud;
    uint8_t  data_bits;
    uint8_t  stop_bits;
    char     parity;
} rtu_backend_t;

typedef struct
{
    backend_params_t base;
    char ip[32];
    int port;

    int client_socket;
} tcp_backend_t;

int get_int(const char str[], bool *ok);

backend_params_t *create_rtu_backend(void);
backend_params_t *create_tcp_backend(void);



#endif /* __MODBUS_TOOLS_COMMON_H__ */