/*
 * bpf_util.h	BPF common code
 *
 *		This program is free software; you can distribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Daniel Borkmann <daniel@iogearbox.net>
 *		Jiri Pirko <jiri@resnulli.us>
 */

#ifndef __BPF_UTIL__
#define __BPF_UTIL__

#include <linux/bpf.h>
#include <linux/filter.h>
#include <linux/magic.h>
#include <linux/elf-em.h>
#include <linux/if_alg.h>

#include "utils.h"
#include "bpf_scm.h"

#define BPF_ENV_UDS	"TC_BPF_UDS"
#define BPF_ENV_MNT	"TC_BPF_MNT"

#ifndef BPF_MAX_LOG
# define BPF_MAX_LOG	4096
#endif

#define BPF_DIR_GLOBALS	"globals"

#ifndef BPF_FS_MAGIC
# define BPF_FS_MAGIC	0xcafe4a11
#endif

#define BPF_DIR_MNT	"/sys/fs/bpf"

#ifndef TRACEFS_MAGIC
# define TRACEFS_MAGIC	0x74726163
#endif

#define TRACE_DIR_MNT	"/sys/kernel/tracing"

#ifndef AF_ALG
# define AF_ALG		38
#endif

#ifndef EM_BPF
# define EM_BPF		247
#endif

struct bpf_cfg_ops {
	void (*cbpf_cb)(void *nl, const struct sock_filter *ops, int ops_len);
	void (*ebpf_cb)(void *nl, int fd, const char *annotation);
};

struct bpf_cfg_in {
	const char *object;
	const char *section;
	const char *uds;
	int argc;
	char **argv;
	struct sock_filter *ops;
};

struct bpf_map_info {
	__u32 map_type;
	__u32 key_size;
	__u32 value_size;
	__u32 max_entries;
	__u32 map_flags;
};

enum bpf_type {
	BPF_TYPE_UNSPEC = 0,
	BPF_TYPE_PROG,
	BPF_TYPE_MAP,
};

/* ALU ops on registers, bpf_add|sub|...: dst_reg += src_reg */

#define BPF_ALU64_REG(OP, DST, SRC)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_OP(OP) | BPF_X,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

#define BPF_ALU32_REG(OP, DST, SRC)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_OP(OP) | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

/* ALU ops on immediates, bpf_add|sub|...: dst_reg += imm32 */

#define BPF_ALU64_IMM(OP, DST, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_OP(OP) | BPF_K,	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

#define BPF_ALU32_IMM(OP, DST, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_OP(OP) | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

/* Short form of mov, dst_reg = src_reg */

#define BPF_MOV64_REG(DST, SRC)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_MOV | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

#define BPF_MOV32_REG(DST, SRC)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_MOV | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

/* Short form of mov, dst_reg = imm32 */

#define BPF_MOV64_IMM(DST, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_MOV | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

#define BPF_MOV32_IMM(DST, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_MOV | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

/* BPF_LD_IMM64 macro encodes single 'load 64-bit immediate' insn */
#define BPF_LD_IMM64(DST, IMM)					\
	BPF_LD_IMM64_RAW(DST, 0, IMM)

#define BPF_LD_IMM64_RAW(DST, SRC, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_LD | BPF_DW | BPF_IMM,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = (__u32) (IMM) }),			\
	((struct bpf_insn) {					\
		.code  = 0, /* zero is reserved opcode */	\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = ((__u64) (IMM)) >> 32 })

#ifndef BPF_PSEUDO_MAP_FD
# define BPF_PSEUDO_MAP_FD	1
#endif

/* pseudo BPF_LD_IMM64 insn used to refer to process-local map_fd */
#define BPF_LD_MAP_FD(DST, MAP_FD)				\
	BPF_LD_IMM64_RAW(DST, BPF_PSEUDO_MAP_FD, MAP_FD)


/* Direct packet access, R0 = *(uint *) (skb->data + imm32) */

#define BPF_LD_ABS(SIZE, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_LD | BPF_SIZE(SIZE) | BPF_ABS,	\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

/* Memory load, dst_reg = *(uint *) (src_reg + off16) */

#define BPF_LDX_MEM(SIZE, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_LDX | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Memory store, *(uint *) (dst_reg + off16) = src_reg */

#define BPF_STX_MEM(SIZE, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_STX | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Memory store, *(uint *) (dst_reg + off16) = imm32 */

#define BPF_ST_MEM(SIZE, DST, OFF, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_ST | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Conditional jumps against registers, if (dst_reg 'op' src_reg) goto pc + off16 */

#define BPF_JMP_REG(OP, DST, SRC, OFF)				\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_OP(OP) | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Conditional jumps against immediates, if (dst_reg 'op' imm32) goto pc + off16 */

#define BPF_JMP_IMM(OP, DST, IMM, OFF)				\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_OP(OP) | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Raw code statement block */

#define BPF_RAW_INSN(CODE, DST, SRC, OFF, IMM)			\
	((struct bpf_insn) {					\
		.code  = CODE,					\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Program exit */

#define BPF_EXIT_INSN()						\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_EXIT,			\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = 0 })

int bpf_parse_common(enum bpf_prog_type type, struct bpf_cfg_in *cfg,
		     const struct bpf_cfg_ops *ops, void *nl);

const char *bpf_prog_to_default_section(enum bpf_prog_type type);

int bpf_graft_map(const char *map_path, uint32_t *key, int argc, char **argv);
int bpf_trace_pipe(void);

void bpf_print_ops(FILE *f, struct rtattr *bpf_ops, __u16 len);

int bpf_prog_load(enum bpf_prog_type type, const struct bpf_insn *insns,
		  size_t size_insns, const char *license, char *log,
		  size_t size_log);

int bpf_prog_attach_fd(int prog_fd, int target_fd, enum bpf_attach_type type);
int bpf_prog_detach_fd(int target_fd, enum bpf_attach_type type);
int bpf_prog_get_attach(enum bpf_prog_type ptype, __u32 arg1, __u32 arg2,
			struct bpf_insn *insns, size_t size_insns);

int bpf_get_pid(__u32 fd, __u32 pid, enum bpf_type *type);
int bpf_get_cgroup(__u32 fd, __u32 attach_type);
int bpf_prog_get(int fd, __u32 *prog_type);
int bpf_map_get(int fd, struct bpf_map_info *map);

int bpf_map_lookup(int fd, const void *key, void *value, __u64 flags);
int bpf_map_get_next_key(int fd, const void *key, void *next_key, __u64 flags);
const char *bpf_map_type2str(__u32 map_type);

#ifdef HAVE_ELF
int bpf_send_map_fds(const char *path, const char *obj);
int bpf_recv_map_fds(const char *path, int *fds, struct bpf_map_aux *aux,
		     unsigned int entries);
#else
static inline int bpf_send_map_fds(const char *path, const char *obj)
{
	return 0;
}

static inline int bpf_recv_map_fds(const char *path, int *fds,
				   struct bpf_map_aux *aux,
				   unsigned int entries)
{
	return -1;
}
#endif /* HAVE_ELF */
#endif /* __BPF_UTIL__ */
