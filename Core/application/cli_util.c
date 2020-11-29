/*
 * cli_util.c
 *
 * Created on: Dec 11, 2018
 *     Author: Y.W. Lee
 */ 

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernel/task.h"
#include "kernel/sem.h"
#include "kernel/ring_buf_event.h"
#include "sys_device/dev_uart.h"
#include "sys_device/dev_board.h"
#include "cli_util.h"
#include "cmd_util.h"


// ############################################################################
// ##
// ############################################################################

static cli_info_t cli_info_root = {
    .cmnd = "CLI_Head",
    .help = "head of CLI",
    .cli_cb = NULL,
    .node_head = NULL,
    .next = NULL,
    .prev = NULL,
    .chain = NULL,
    .type = CLI_INFO_TYPE_NODE,
    .level = 0};
static cli_info_t *cli_chain_last = &cli_info_root;
static const char *cli_cmd_stack[CLI_CMD_STACK_MAX];
static uint8_t cli_cnfg_dbg_trace = 0;
static uint8_t cli_cnfg_dbg_cmd_path = 0;


static void cli_info_chain_walk_reset(void);
static char *indentation_create (int num);
static void indentation_free(char *ind);
static void cli_info_dump(cli_info_t *cli, const char *idnt);
static void cli_info_dump_sub_tree(cli_info_t *cli);
static int cli_info_cmd_path_dump(cli_info_t *cli, const char *pad);
static void cli_info_help(cli_info_t *cli_node);
static void cli_info_help_trace(cli_info_t *cli_node);
static void cli_info_tree_scan(void);



// ############################################################################
// ##
// ############################################################################

static void cli_info_init(cli_info_t *cli_info,
                          int type, const char *command, const char *help, cli_func_t cb)
{
    cli_info->cmnd = command;
    cli_info->help = help;
    cli_info->type = type;
    cli_info->cnfg_err = 0;
    cli_info->cli_cb = cb;
    cli_info->node_head = NULL;
    cli_info->next = NULL;
    cli_info->prev = NULL;
    cli_info->chain = NULL;
}


void cli_info_init_node(cli_info_t *cli_info, const char *command, const char *help)
{
    cli_info_init(cli_info, CLI_INFO_TYPE_NODE, command, help, NULL);
}


void cli_info_init_leaf(cli_info_t *cli_info, const char *command, const char *help, cli_func_t cb)
{
    cli_info_init(cli_info, CLI_INFO_TYPE_LEAF, command, help, cb);
}


void cli_info_attach_node(cli_info_t *cli_node, cli_info_t *my_cli)
{
    cli_info_t *link, *prev;

    if (my_cli->cmnd == NULL) {
        my_cli->cnfg_err |= CLI_INFO_CNFG_ERR_CMND_NULL;
    }

    if (cli_node->node_head == NULL) {
        cli_node->node_head = my_cli;
        my_cli->prev = cli_node;
        goto EXIT;   // init condition ...
    }

    link = cli_node->node_head;
    prev = cli_node;

    while (link) {

        if (strcmp(my_cli->cmnd, link->cmnd) == 0) {
            my_cli->cnfg_err |= CLI_INFO_CNFG_ERR_CMND_DUP;
        }

        if (strcmp(my_cli->cmnd, link->cmnd) < 0) {
            break;
        } else {
            prev = link;
            link = link->next;
        }
    }

    if (prev == cli_node) {
        prev->node_head = my_cli;
    } else {
        prev->next = my_cli;
    }
    my_cli->prev = prev;
    my_cli->next = link;
    if (link) {
        link->prev = my_cli;
    }

 EXIT:
    my_cli->level = cli_node->level + 1;
    cli_chain_last->chain = my_cli;
    cli_chain_last = my_cli;
}


void cli_info_attach_root(cli_info_t *my_cli)
{
    cli_info_attach_node(&cli_info_root, my_cli);
}


// ############################################################################
// ############################################################################

void cli_info_parser(cmd_info_t *cmd_info)
{
    cli_info_t *cli = cli_info_root.node_head;
    arg_info_t *arg = cmd_info->arg_list;
    uint32_t arg_num = cmd_info->arg_num;
    int rtn, found;

    if (cli_cnfg_dbg_trace) {
        printf("cli_info_parser:\r\n");          // DBG ............
    }

    for (uint32_t n = 0; n < arg_num; ++n) {

        char *cmd = arg[n].beg;
        int   cmd_len = arg[n].len;

        if (cli_cnfg_dbg_trace)
            printf("  %lu: %s\r\n", n, cmd);      // DBG ............

        if (strncmp("help", cmd, cmd_len) == 0 || strncmp("?", cmd, cmd_len) == 0) {
            if (cli_cnfg_dbg_trace)
                printf("  ->> help cmd\r\n");    // DBG ............
            cli_info_help_trace(cli);
            goto EXIT;
        }

        if (cli_cnfg_dbg_trace)
            printf("search: ");                // DBG ............

        found = 0;
        while (cli) {
            // searche the command pattern
            if (cli_cnfg_dbg_trace)
                printf("%s ", cli->cmnd);      // DBG ............
            if (strncmp(cli->cmnd, cmd, strlen(cli->cmnd)) == 0) {
                found = 1;
                break;  // match !!!
            }
            cli = cli->next;
        }

        if (!found) {
            printf("invalid command ...\r\n");
            goto EXIT;
        }
        if (cli_cnfg_dbg_trace)
            printf("-> ");                     // DBG ............

        if (cli->cli_cb) {
            // execute the call-back
            if (cli_cnfg_dbg_trace)
                printf("call-back\n ");        // DBG ............
            cmd_info->arg_rd_indx = n + 1;
            rtn = cli->cli_cb(cmd_info);
            if (rtn < 0) {
                // what should we do if ...
            }
            goto EXIT;

        } else if (cli->node_head) {
            if (cli_cnfg_dbg_trace)
                printf("goto node\r\n");         // DBG ............
            cli = cli->node_head;

        } else {
            // call-back and node_start are both NULL ....???
            printf("dummy command\r\n");
            goto EXIT;
        }
    }
    cli_info_help(cli);

 EXIT:
    printf("\r\n");
}


// ############################################################################
// ############################################################################

static void cli_info_tree_scan(void)
{
    cli_info_t *cli;
    int count, count_err;

    printf("Scan CLI tree ...\r\n");

    count = count_err = 0;
    cli = &cli_info_root;

    do {

        if (cli->cnfg_err & CLI_INFO_CNFG_ERR_CMND_DUP) {
            count_err++;
        }
        if (cli->cnfg_err & CLI_INFO_CNFG_ERR_CMND_NULL) {
            count_err++;
        }

        if (cli->type == CLI_INFO_TYPE_LEAF) {
            if (cli->node_head) {
                // cli is a leaf, node_head is not NULL
                cli->cnfg_err |= CLI_INFO_CNFG_ERR_LEAF_NODE;
                count_err++;
            }

        } else if (cli->type == CLI_INFO_TYPE_NODE) {
            if (cli->cli_cb) {
                // cli is a node, but the call-back is not NULL
                cli->cnfg_err |= CLI_INFO_CNFG_ERR_NODE_CB;
                count_err++;
            }

            if (cli->node_head == NULL) {
                // cli is a EMPTY node ...
                cli->cnfg_err |= CLI_INFO_CNFG_ERR_NODE_EMPTY;
                count_err++;
            }

        } else {
            cli->cnfg_err |= CLI_INFO_CNFG_ERR_TYPE_UNKWN;
            count_err++;
        }

        if (cli->cnfg_err) {
            cli_info_cmd_path_dump(cli, "  ");
            printf("- %s\r\n", cli->help);
            if (cli->cnfg_err & CLI_INFO_CNFG_ERR_TYPE_UNKWN)
                printf("    cli type is unknown\r\n");
            if (cli->cnfg_err & CLI_INFO_CNFG_ERR_CMND_DUP)
                printf("    cli command is duplictaed\r\n");
            if (cli->cnfg_err & CLI_INFO_CNFG_ERR_CMND_NULL)
                printf("    cli command is NULL\r\n");
            if (cli->cnfg_err & CLI_INFO_CNFG_ERR_NODE_CB)
                printf("    cli is a node with call-back function\r\n");
            if (cli->cnfg_err & CLI_INFO_CNFG_ERR_NODE_EMPTY)
                printf("    cli is a node, but node_head is EMPTY\r\n");
            if (cli->cnfg_err & CLI_INFO_CNFG_ERR_LEAF_NODE)
                printf("    cli is a leaf, but node_head is not EMPTY\r\n");
        }

        count++;
        cli = cli->chain;
    } while (cli);

    printf("total cli: %d\r\n", count);
    printf("      err: %d\r\n", count_err);
}


// ############################################################################
// ############################################################################

static void cli_info_help(cli_info_t *cli_node)
{
    cli_info_t *cli = cli_node;

    printf("help :\r\n");
    while (cli) {

        cli_info_cmd_path_dump(cli, "  ");
        switch (cli->type) {
        case CLI_INFO_TYPE_NODE:
            printf("- <N> ");
            break;
        case CLI_INFO_TYPE_LEAF:
            printf("- <L> ");
            break;
        default:
            printf("- <?> ");
        }
        printf("%s\r\n", (cli->help) ? cli->help : "(null)");

        cli = cli->next;
    }
}

static void cli_info_help_trace(cli_info_t *cli_node)
{
    cli_info_t *cli_stop;
    cli_info_t *cli;
    int flag;

    cli_info_chain_walk_reset();
    cli_stop = cli_node->prev;
    cli = cli_node;

    do {

        if (!(cli->state & CLI_INFO_STATE_ACT)) {
            cli->state |= CLI_INFO_STATE_ACT;

            if (cli_cnfg_dbg_cmd_path) {
                flag = 1;
            } else if (cli->cli_cb) {
                flag = 1;
            } else {
                flag = 0;
            }

            if (flag) {
                cli_info_cmd_path_dump(cli, "  ");
                switch (cli->type) {
                case CLI_INFO_TYPE_NODE:
                    printf("- <N> ");
                    break;
                case CLI_INFO_TYPE_LEAF:
                    printf("- <L> ");
                    break;
                default:
                    printf("- <?> ");
                }
                printf("%s\r\n", (cli->help) ? cli->help : "(null)");
            }


        } else if (!(cli->state & CLI_INFO_STATE_NODE)) {
            cli->state |= CLI_INFO_STATE_NODE;
            if (cli->node_head) {
                cli = cli->node_head;
            }

        } else if (!(cli->state & CLI_INFO_STATE_NEXT)) {
            cli->state |= CLI_INFO_STATE_NEXT;
            if (cli->next) {
                cli = cli->next;
            }

        } else {
            cli = cli->prev;
        }

    } while (cli != cli_stop);
}


void cli_info_help_trace_root(void)
{
    cli_info_help_trace(&cli_info_root);
}


// ############################################################################
// ############################################################################

static void cli_info_dump_recusive_core(cli_info_t *cli, int levl)
{
    char *ind = indentation_create (2*levl);

    cli_info_cmd_path_dump(cli, ind);
    printf("\r\n");
    cli_info_dump(cli, ind);
    printf("\r\n");

    cli_info_t *leaf = cli->node_head;
    if (leaf != NULL) {
        cli_info_dump_recusive_core(leaf, levl+1);
    }

    if (cli->next) {
        cli_info_dump_recusive_core(cli->next, levl);
    }

    indentation_free(ind);
}

void cli_info_dump_recursive(void)
{
    cli_info_dump_recusive_core(&cli_info_root, 0);
}


// ############################################################################
// ############################################################################

static void cli_info_chain_walk_reset(void)
{
    cli_info_t *cli;

    cli = &cli_info_root;
    do {
        cli->state = 0;
        cli = cli->chain;
    } while (cli);
}


void cli_info_dump_chain_walk(void)
{
    cli_info_t *cli;

    cli = &cli_info_root;
    do {
        cli_info_dump(cli, " ");
        printf("\r\n");
        cli = cli->chain;
    } while (cli != cli_chain_last);
}


// ############################################################################
// ############################################################################

static void cli_info_dump_sub_tree(cli_info_t *cli)
{
    cli_info_t *cli_node;
    uint32_t level_offset;

    cli_info_chain_walk_reset();

    cli_node = cli->prev;
    level_offset = cli->level;
    while (cli != cli_node) {

        if (!(cli->state & CLI_INFO_STATE_ACT)) {
            cli->state |= CLI_INFO_STATE_ACT;

            char *ind = indentation_create (2 * (cli->level - level_offset));

            cli_info_cmd_path_dump(cli, ind);
            printf("\r\n");
            cli_info_dump(cli, ind);
            printf("\r\n");

            indentation_free(ind);

        } else if (!(cli->state & CLI_INFO_STATE_NODE)) {
            cli->state |= CLI_INFO_STATE_NODE;
            if (cli->node_head) {
                cli = cli->node_head;
            }

        } else if (!(cli->state & CLI_INFO_STATE_NEXT)) {
            cli->state |= CLI_INFO_STATE_NEXT;
            if (cli->next) {
                cli = cli->next;
            }

        } else {
            cli = cli->prev;
        }
    }
}

void cli_info_dump_full_tree(void)
{
    cli_info_dump_sub_tree(&cli_info_root);
}


// ############################################################################
// ##
// ############################################################################

static char *indentation_create(int num)
{
    char *ind = (char *) malloc((num + 1) * sizeof(char));

    for (int n = 0; n < num; ++n) {
        ind[n] = ' ';
    }
    ind[num] = 0;

    return ind;
}


static void indentation_free(char *ind)
{
    free(ind);
}


static void cli_info_dump_type(cli_info_t *cli)
{
    switch (cli->type) {
    case CLI_INFO_TYPE_NODE:
        printf(" - NODE %s\r\n", cli->cmnd);
        break;
    case CLI_INFO_TYPE_LEAF:
        printf(" - LEAF %s\r\n", cli->cmnd);
        break;
    default:
        printf(" - ???? %s\r\n", cli->cmnd);
    }
}


static void cli_info_dump(cli_info_t *cli, const char *idnt)
{
    printf("%sCLI Info (%p):\r\n", idnt, cli);
    printf("%s  level: %d\r\n", idnt, cli->level);
    if (cli->type == CLI_INFO_TYPE_NODE) {
        printf("%s  type:  NODE ...\r\n", idnt);
    } else if (cli->type == CLI_INFO_TYPE_LEAF) {
        printf("%s  type:  LEAF\r\n", idnt);
    } else {
        printf("%s  type:  UNKNOW !!!!!!\r\n", idnt);
    }
    printf("%s  cnfg:  0x%x\r\n", idnt, cli->cnfg_err);
    printf("%s  cmnd:  %s\r\n", idnt, cli->cmnd);
    printf("%s  help:  %s\r\n", idnt, cli->help);
    printf("%s  cb:    %p\r\n", idnt, cli->cli_cb);
    printf("%s  node:  %p", idnt, cli->node_head);
    if (cli->node_head) {
        cli_info_dump_type(cli->node_head);
    } else {
        printf("\r\n");
    }
    printf("%s  prev:  %p", idnt, cli->prev);
    if (cli->prev) {
        cli_info_dump_type(cli->prev);
    } else {
        printf("\r\n");
    }
    printf("%s  next:  %p", idnt, cli->next);
    if (cli->next) {
        cli_info_dump_type(cli->next);
    } else {
        printf("\r\n");
    }
    printf("%s  chain: %p\r\n", idnt, cli->chain);
}


static int cli_info_cmd_path_dump(cli_info_t *cli, const char *ind)
{
    uint32_t level;
    int m, n;

    n = 0;
    level = (uint32_t) -1;

    while  (cli) {
        if (level > cli->level) {
            level = cli->level;
            if (n < CLI_CMD_STACK_MAX) {
                cli_cmd_stack[n] = cli->cmnd;
            }
            n++;
        }
        cli = cli->prev;
    }

    printf("%s", ind);
    for (m = (n - 2); m >= 0; --m) {
        if (m < CLI_CMD_STACK_MAX) {
            printf("%s ", cli_cmd_stack[m]);
        }
    }

    return n;
}


// ############################################################################
// ##
// ############################################################################

extern ring_buf_event_t uart_buf_event;
static cmd_info_t cli_cmd_info;


uint32_t cli_task(void *arg)
{
    char     *buf_p;
    uint32_t  buf_len;
    int rt;

    uart_handler_init();  // Setup the UART_Handler() ISR for UART

    cli_mng_init();
    cli_cmd_info.arg_list_size = ARG_LEN;

    cli_mem_init();   // cli for mem checking

    while (1) {

        ring_buf_event_wait(&uart_buf_event, &buf_p, &buf_len);
        if (buf_len == 0) {
            continue;
        }
        board_blink_led_cli();

        cli_cmd_info.cmd_buf = buf_p;
        cli_cmd_info.cmd_len = buf_len;

        rt = cmd_segment(&cli_cmd_info);
        if (rt < 0) {
            printf("Error, too many arguments ...\r\n");
            continue;
        }
        cli_info_parser(&cli_cmd_info);

        ring_buf_event_seg_free(&uart_buf_event);
    }

    return 0;
}


// ############################################################################
// ##
// ############################################################################

static int cli_cb_cnfg(cmd_info_t *cmd_info)
{
    const char *help = "usuage: [debug on|off] [cmd_path on|off]";
    arg_info_t *arg;

    if (!cmd_arg_avail(cmd_info) || cmd_arg_1st_is_help(cmd_info)) {
        printf("%s\r\n", help);
        goto EXIT;
    }

    while (cmd_arg_avail(cmd_info)) {

        macro_cmd_arg_pop(cmd_info, arg);

        if (!strcmp(arg->beg, "debug")) {
            macro_cmd_arg_pop(cmd_info, arg);
            if (!strcmp(arg->beg, "on")) {
                cli_cnfg_dbg_trace = 1;
            } else {
                cli_cnfg_dbg_trace = 0;
            }

        } else if (!strcmp(arg->beg, "cmd_path")) {
            macro_cmd_arg_pop(cmd_info, arg);
            if (!strcmp(arg->beg, "on")) {
                cli_cnfg_dbg_cmd_path = 1;
            } else {
                cli_cnfg_dbg_cmd_path = 0;
            }

        } else {
            printf("invalid commmand, %s\r\n", arg->beg);
        }
    }

 EXIT:
    printf("debug: %s\r\n", (cli_cnfg_dbg_trace == 1) ? "on" : "off");
    printf("cmd_path %s\r\n", (cli_cnfg_dbg_cmd_path == 1) ? "on" :" off");
    return 0;
}


static int cli_cb_scan(cmd_info_t *cmd_info)
{
    const char *help = "usuage:";

    if (cmd_arg_1st_is_help(cmd_info)) {
        printf("%s\r\n", help);
        return 0;
    }

    cli_info_tree_scan();
    return 0;
}


static int cli_cb_dump(cmd_info_t *cmd_info)
{
    const char *help = "usuage: tree|single cmd_path ...";
    arg_info_t *arg;
    int mode, found, stop_at_leaf;

    if (!cmd_arg_avail(cmd_info) || cmd_arg_1st_is_help(cmd_info)) {
        printf("%s\r\n", help);
        return 0;
    }

    arg = cmd_arg_pop(cmd_info);
    if (!strcmp("tree", arg->beg)) {
        mode = 0;
    } else if (!strcmp("single", arg->beg)) {
        mode = 1;
    } else {
        printf("invalid command %s\r\n", arg->beg);
        return -1;
    }

    stop_at_leaf = 0;
    cli_info_t *cli = cli_info_root.node_head;
    while (cmd_arg_avail(cmd_info)) {

        arg = cmd_arg_pop(cmd_info);
        printf("%s ", arg->beg);

        found = 0;
        while (cli) {
            if (!strcmp(cli->cmnd, arg->beg)) {
                found = 1;
                break;  // match !!!
            }
            cli = cli->next;
        }

        if (!found) {
            printf("-> invalid cli path\r\n");
            return -1;
        }

        if (cli->node_head) {
            cli = cli->node_head;
        } else {
            stop_at_leaf = 1;
            break;
        }
    }
    printf("\n\r\n");

    if (mode == 0) {
        cli_info_dump_sub_tree(cli);
    } else {
        if (stop_at_leaf == 0) {
            cli = cli->prev;
        }
        cli_info_cmd_path_dump(cli, "");
        printf("\r\n");
        cli_info_dump(cli, "");
    }

    return 0;
}


// ############################################################################
// ############################################################################

static cli_info_t cli_mng_root;
static cli_info_t cli_mng_root_cnfg;
static cli_info_t cli_mng_root_scan;
static cli_info_t cli_mng_root_dump;

void cli_mng_init(void)
{
    cli_info_init_node(&cli_mng_root, "cli", "cli control and dump");
    cli_info_attach_root(&cli_mng_root);

    cli_info_init_leaf(&cli_mng_root_cnfg, "cnfg", "control cli operation", cli_cb_cnfg);
    cli_info_init_leaf(&cli_mng_root_scan, "scan", "scan and check the cli tree", cli_cb_scan);
    cli_info_init_leaf(&cli_mng_root_dump, "dump", "dump cli_info (single or sub-tree)", cli_cb_dump);

    cli_info_attach_node(&cli_mng_root, &cli_mng_root_cnfg);
    cli_info_attach_node(&cli_mng_root, &cli_mng_root_scan);
    cli_info_attach_node(&cli_mng_root, &cli_mng_root_dump);
}


// ############################################################################
// ##
// ## Memory Utility
// ##
// ############################################################################

// ############################################################################
// ############################################################################

#define LINE_BYTE_COUNT    20


typedef struct cli_mem_reg_info_ {
    uint32_t    beg;   // address should be in te following range
    uint32_t    end;   //     beg <= address < end
    const char *name;  // name of the region
} cli_mem_reg_info;


// STM32F412ZG
const cli_mem_reg_info cli_mem_reg_array[] = {
    {0x40000000, 0x40002400, "APB1.1 - TIM 2~7, 12~14"},
    {0x40002800, 0x40004C00, "    .2 - RTC, WDG, SPI, I2S, USART"},
    {0x40005400, 0x40006C00, "    .3"},
    {0x40007000, 0x40007400, "    .4"},
    {0x40010000, 0x40010800, "APB2.1"},
    {0x40011000, 0x40011800, "    .2"},
    {0x40012000, 0x40012400, "    .3"},
    {0x40012C00, 0x40015400, "    .4"},
    {0x40016000, 0x40016400, "    .5"},
    {0x40020000, 0x40022000, "AHB1.1"},
    {0x40023000, 0x40023400, "    .2"},
    {0x40023800, 0x40024000, "    .3"},
    {0x40026000, 0x40026800, "    .4"},
    {0x50000000, 0x50040000, "AHB2.1"},
    {0x50060800, 0x50060C00, "    .2"},
    {0x60000000, 0x70000000, "AHB3.1"},
    {0x90000000, 0xA0002000, "    .2"},
    {0xE0000000, 0xE0100000, "Cortex-M4"},
    {0x20000000, 0x20040000, "RAM 512KB"}
};

const cli_mem_reg_info cli_mem_reg_flash = {
    0x08000000, 0x08100000, "FLASH, 1MB"
};


static uint32_t str2int(char *v_string)
{
    uint32_t val;
    
    if (strlen(v_string) <= 2) {
        return atoi(v_string);
    }
    
    if (v_string[0] == '0' && v_string[1] == 'x') {
        sscanf(v_string, "%lx", &val);
        return val;
    } else {
        return atoi(v_string);
    }
}


static bool cli_mem_reg_rd_valid(uint32_t addr, uint32_t size)
{
    uint32_t beg = addr;
    uint32_t end = addr + size;

    if (beg >= cli_mem_reg_flash.beg && end <= cli_mem_reg_flash.end) {
        return true;
    }

    for (int n = 0; n < sizeof(cli_mem_reg_array)/sizeof(cli_mem_reg_info); ++n) {
        if (beg >= cli_mem_reg_array[n].beg && end <= cli_mem_reg_array[n].end) {
            return true;
        }
    }
    return false;
}


static void cli_mem_dump_char(uint32_t beg, uint32_t size)
{
    uint32_t addr;
    int count = 0;

    while (count < size) {

        addr = beg + count;
        if ((count % LINE_BYTE_COUNT) == 0) {
            printf("0x%08lx - %02x", addr, *((uint8_t *) addr));
        } else {
            printf(" %02x", *((uint8_t *) addr));
        }

        count++;
        if ((count % LINE_BYTE_COUNT) == 0) {
            printf("\r\n");
        }
    }
    if ((count % LINE_BYTE_COUNT)) {
        printf("\r\n");
    }

    printf("\r\n");
}


static void cli_mem_dump_short(uint32_t beg, uint32_t size)
{
    uint32_t addr;
    int count = 0;

    while (count < size) {

        addr = beg + count;
        if ((count % LINE_BYTE_COUNT) == 0) {
            printf("0x%08lx - %04x", addr, *((uint16_t *) addr));
        } else {
            printf(" %04x", *((uint16_t *) addr));
        }

        count += 2;
        if ((count % LINE_BYTE_COUNT) == 0) {
            printf("\r\n");
        }
    }
    if ((count % LINE_BYTE_COUNT)) {
        printf("\r\n");
    }

    printf("\r\n");
}


static void cli_mem_dump_long(uint32_t beg, uint32_t size)
{
    uint32_t addr;
    int count = 0;

    while (count < size) {
        
        addr = beg + count;
        if ((count % LINE_BYTE_COUNT) == 0) {
            printf("0x%08lx - %08lx", addr, *((uint32_t *) addr));
        } else {
            printf(" %08lx", *((uint32_t *) addr));
        }

        count += 4;
        if ((count % LINE_BYTE_COUNT) == 0) {
            printf("\r\n");
        }
    }
    if ((count % LINE_BYTE_COUNT)) {
        printf("\r\n");
    }

    printf("\r\n");
}


static int cli_cb_mem_root_dump(cmd_info_t *cmd_info)
{
    const char *help = "usuage: <mode> <addr> <size_aft> [size_bfr]\r\n"
                       "        <mode> - c: char, s: short, l: long\r\n";
    arg_info_t *arg;
    uint32_t addr, size_a, size_b, size;
    char mode;

    if (!cmd_arg_avail(cmd_info) || cmd_arg_1st_is_help(cmd_info)) {
        printf("%s\r\n", help);
        goto EXIT;
    }

    arg = cmd_arg_pop(cmd_info);
    if (arg == NULL) {
        printf("   !!! missing <mode> !!!\r\n");
        goto EXIT;
    }
    switch (*arg->beg) {
    case 'c':
        mode = 'c';
        break;
    case 's':
        mode = 's';
        break;
    case 'l':
        mode = 'l';
        break;
    default:
        printf("   !!! invalid mode %c !!!\r\n", *arg->beg);
        goto EXIT;
    }

    arg = cmd_arg_pop(cmd_info);
    if (arg == NULL) {
        printf("   !!! missing <addr> !!!\r\n");
        goto EXIT;
    }
    addr = str2int(arg->beg);

    arg = cmd_arg_pop(cmd_info);
    if (arg == NULL) {
        printf("   !!! missing <size_aft> !!!\r\n");
        goto EXIT;
    }
    size_a = str2int(arg->beg);

    arg = cmd_arg_pop(cmd_info);
    if (arg != NULL) {
        size_b = str2int(arg->beg);
    } else {
        size_b = 0;
    }

    addr = addr - size_b;
    size = size_a + size_b;    
    switch (mode) {
    case 's':
        addr &= 0xFFFFFFFE;   // aligned to 2 byte
        size &= 0xFFFFFFFE;   // aligned to 2 byte
        break;
    case 'l':
        addr &= 0xFFFFFFFC;   // aligned to 4 byte
        size &= 0xFFFFFFFC;   // aligned to 4 byte
        break;
    default:
        break;
    }

    printf("addr: 0x%08lx, size: %lu byte, mode '%c'\r\n", addr, size, mode);
    if (!cli_mem_reg_rd_valid(addr, size)) {
        printf("    !!! invalid memory region for read\r\n");
        goto EXIT;
    }
    printf("\n");

    switch (mode) {
    case 'c':
        cli_mem_dump_char(addr, size);
        break;
    case 's':
        cli_mem_dump_short(addr, size);
        break;
    default:
        cli_mem_dump_long(addr, size);
        break;
    }

 EXIT:
    return 0;
}


static int cli_cb_mem_root_map(cmd_info_t *cmd_info)
{
    int n;
    for (n = 0; n < sizeof(cli_mem_reg_array)/sizeof(cli_mem_reg_info); ++n) {
        printf("   %2d: 0x%08lx ~ 0x%08lx   %s\r\n", n,
               cli_mem_reg_array[n].beg, cli_mem_reg_array[n].end, cli_mem_reg_array[n].name);
    }
    printf("   %2d: 0x%08lx ~ 0x%08lx   %s\r\n", n,
           cli_mem_reg_flash.beg, cli_mem_reg_flash.end, cli_mem_reg_flash.name);
    return 0;
}


// ############################################################################
// ############################################################################

static cli_info_t cli_mem_root;
static cli_info_t cli_mem_root_map;
static cli_info_t cli_mem_root_dump;


void cli_mem_init(void)
{
    cli_info_init_node(&cli_mem_root, "mem", "mem dump/modify");
    cli_info_attach_root(&cli_mem_root);

    cli_info_init_leaf(&cli_mem_root_map, "map", "show the memory map",
                       cli_cb_mem_root_map);
    cli_info_attach_node(&cli_mem_root, &cli_mem_root_map);

    cli_info_init_leaf(&cli_mem_root_dump, "dump", "dump memory region",
                       cli_cb_mem_root_dump);
    cli_info_attach_node(&cli_mem_root, &cli_mem_root_dump);
}
