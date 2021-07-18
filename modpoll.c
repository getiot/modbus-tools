#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include "common.h"

static struct option long_options[] = {
    {DebugOpt, no_argument, 0, 0}, 
    {0, 0, 0, 0}
};

void show_usage(const char *progname)
{
    printf("usage: %s [options]... <serialport|host> [write-data]\n", progname);
    printf("  options:\n");
    printf("    --debug  debug mode.\n");
    printf("    -m{rtu|tcp}\n");
    printf("    -a<slave-addr=1>\n");
    printf("    -c<read-no>=1\n");
    printf("    -r<start-addr>=100\n");
    printf("    -t<f-type>              f-type:\n");
    printf("      (0x01) Read Coils\n");
    printf("      (0x02) Read Discrete Inputs\n");
    printf("      (0x03) Read Holding Registers\n");
    printf("      (0x04) Read Input Registers\n");
    printf("      (0x05) Write Single Coil\n");
    printf("      (0x06) Write Single Register\n");
    printf("      (0x0F) Write Multiple Coils\n");
    printf("      (0x10) Write Multiple Registers\n");
    printf("    -o<timeout-ms>=1000\n");
    printf("    rtu-params\n");
    printf("      b<baud-rate>=9600\n");
    printf("      d{7|8}<data-bits>=8\n");
    printf("      s{1|2}<stop-bits>=1\n");
    printf("      p{none|even|odd}=none\n");
    printf("    tcp-params\n");
    printf("      p<port>=502\n");
    printf("    -h --help               display the help screen and exit.\n");
    printf("    -v --version            display the version number and exit.\n\n");
    printf("Examples (run with default modbus server on port 1502):\n");
    printf("  Write data: %s --debug -mtcp -t0x10 -r0 -p1502 127.0.0.1 0x01 0x02\n", progname);
    printf("   Read data: %s --debug -mtcp -t0x03 -r0 -p1502 127.0.0.1 -c3\n", progname);
}

int main(int argc, char *argv[])
{
    int c;
    int ret = EXIT_SUCCESS;
    bool ok;
    int debug = 0;
    backend_params_t *backend = 0;
    int slave_addr = 1;
    int start_addr = 100;
    int start_reference_at0 = 0;
    int read_write_no = 1;
    int f_type = FuncNone;
    int timeout_ms = 1000;
    int has_device = 0;
    int is_write_function = 0;

    enum WriteDataType
    {
        DataInt,
        Data8Array,
        Data16Array
    } wDataType = DataInt;

    union Data
    {
        int dataInt;
        uint8_t *data8;
        uint16_t *data16;
    } data;

    while (1)
    {
        int option_index = 0;

        c = getopt_long(argc, argv, "hva:b:d:c:m:r:s:t:p:o:0", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c)
        {
        case 0:
        {
            if (0 == strcmp(long_options[option_index].name, DebugOpt)) {
                debug = 1;
            }
        }
        break;
        case 'a':
        {
            slave_addr = get_int(optarg, &ok);
            if (!ok) {
                printf("Slave address (%s) is not integer!\n", optarg);
                ret = EXIT_FAILURE;
                goto __exit;
            }
        }
        break;
        case 'c':
        {
            read_write_no = get_int(optarg, &ok);
            if (!ok) {
                printf("Quantity to read/write (%s) is not integer!\n", optarg);
                ret = EXIT_FAILURE;
                goto __exit;
            }
        }
        break;
        case 'm':
        {
            if (0 == strcmp(optarg, TcpOptVal)) {
                backend = create_tcp_backend();
            } else if (0 == strcmp(optarg, RtuOptVal)) {
                backend = create_rtu_backend();
            } else {
                printf("Unrecognized connection type %s\n", optarg);
                ret = EXIT_FAILURE;
                goto __exit;
            }
        }
        break;
        case 'r':
        {
            start_addr = get_int(optarg, &ok);
            if (!ok) {
                printf("Start address (%s) is not integer!\n", optarg);
                ret = EXIT_FAILURE;
                goto __exit;
            }
        }
        break;
        case 't':
        {
            f_type = get_int(optarg, &ok);
            if (!ok) {
                printf("Function code (%s) is not integer!\n", optarg);
                ret = EXIT_FAILURE;
                goto __exit;
            }
        }
        break;
        case 'o':
        {
            timeout_ms = get_int(optarg, &ok);
            if (!ok) {
                printf("Timeout (%s) is not integer!\n", optarg);
                ret = EXIT_FAILURE;
                goto __exit;
            }
        }
        break;
        case '0':
        {
            start_reference_at0 = 1;
        }
        break;
        case 'p':
        case 'b':
        case 'd':
        case 's':
        {
            if (!backend) {
                printf("Connect type (-m switch) has to be set!\n");
                ret = EXIT_FAILURE;
                goto __exit;
            } else {
                if (0 == backend->set_param(backend, c, optarg)) {
                    ret = EXIT_FAILURE;
                }
            }
        }
        break;
        case '?': break;
        case 'h':
        {
            show_usage("modpoll");
        }
        break;
        default: printf("?? getopt return char code 0%o ??\n", c);
            break;
        }
    }

    if (!backend) {
        printf("You have not specify modbus backend.\n");
        exit(EXIT_FAILURE);
    }

    if (1 == start_reference_at0) {
        start_addr--;
    }

    /* Choose write data type */
    if (f_type == ReadCoils || f_type == WriteMultipleCoils) {
        wDataType = Data8Array;
    } else if (f_type == ReadDiscreteInput || f_type == WriteSingleCoil || f_type == WriteSingleRegister) {
        wDataType = DataInt;
    } else if (f_type == ReadHoldingRegisters || f_type == ReadInputRegisters || f_type == WriteMultipleRegisters) {
        wDataType = Data16Array;
    } else {
        printf("No correct function type chose\n");
        ret = EXIT_FAILURE;
        goto __exit;
    }

    if (is_write_function) {
        int data_num = argc - optind -1;
        read_write_no = data_num;
    }
    
__exit:
    if (backend) {
        free(backend);
    }

    return ret;
}