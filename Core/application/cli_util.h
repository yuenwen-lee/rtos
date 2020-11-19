/*
 * cli_util.h
 *
 * Created on: Dec 11, 2018
 *     Author: ywlee
 */ 

#ifndef _CLI_UTIL_H_
#define _CLI_UTIL_H_

#include <stdint.h>
#include "cmd_util.h"


// ############################################################################
// ############################################################################

#define CLI_CMD_STACK_MAX     8   //

#define CLI_INFO_STATE_NODE 0x1   // state of node-walk-through
#define CLI_INFO_STATE_NEXT 0x2   // state of next-walk-through
#define CLI_INFO_STATE_ACT  0x4   // state of cli-action

#define CLI_INFO_TYPE_UNSET 0x0   // info is not set
#define CLI_INFO_TYPE_NODE  0x1   // info is NODE
#define CLI_INFO_TYPE_LEAF  0x2   // info is LEAF


#define CLI_INFO_CNFG_ERR_TYPE_UNKWN  0x01   // cli type is unknown
#define CLI_INFO_CNFG_ERR_CMND_DUP    0x02   // duplicated command in level
#define CLI_INFO_CNFG_ERR_NODE_CB     0x04   // node cli with call-back
#define CLI_INFO_CNFG_ERR_NODE_EMPTY  0x08   // node cli with EMPTY node link
#define CLI_INFO_CNFG_ERR_LEAF_NODE   0x10   // leaf cli with non-empty node link
#define CLI_INFO_CNFG_ERR_CMND_NULL   0x20   // cli command is NULL


// ############################################################################
// ############################################################################

typedef int (*cli_func_t)(cmd_info_t *cmd_info);

typedef struct cli_info_ {
	const char       *cmnd;
	const char       *help;
	uint8_t           type;       // 0 -> uknow, 1 -> node, 2 -> leaf
	uint8_t           cnfg_err;   // configuration error
	uint8_t           state;      // walk through state                    (X)
	uint8_t           level;      // command level
	cli_func_t        cli_cb;     // call-back func, if cli_cb == NULL, this is a node
	struct cli_info_ *node_head;  // the head of sub-level cli_info_t
	struct cli_info_ *next;       // next cli_info_t in this level
	struct cli_info_ *prev;       // previous cli_info_t in this level
	struct cli_info_ *chain;      // for construct one thread link list    (X)
} cli_info_t;


// ############################################################################
// ############################################################################

void cli_info_init_node(cli_info_t *cli_info, const char *name, const char *help);
void cli_info_init_leaf(cli_info_t *cli_info, const char *name, const char *help, cli_func_t cb);

void cli_info_attach_root(cli_info_t *my_cli);
void cli_info_attach_node(cli_info_t *cli_node, cli_info_t *my_cli);

void cli_info_parser(cmd_info_t *cmd_info);

void cli_mng_init(void);
uint32_t cli_task(void *arg);

void cli_info_dump_recursive(void);
void cli_info_dump_chain_walk(void);
void cli_info_dump_full_tree(void);
void cli_info_help_trace_root(void);

void cli_mem_init(void);


#endif /* _CLI_UTIL_H_ */
