#ifndef SYSCALLS_H
#define SYSCALLS_H

enum {
	SYS_NONE = 0,
	SYS_KPRINT,

	//proccess memory manage
	SYS_MALLOC,
	SYS_REALLOC,
	SYS_FREE,

	SYS_EXEC_ELF,
	SYS_FORK,
	SYS_THREAD,
	SYS_YIELD,
	SYS_WAIT_PID,
	SYS_USLEEP,
	SYS_EXIT,
	SYS_DETACH,
	SYS_BLOCK,
	SYS_WAKEUP,

	SYS_GET_PID,
	SYS_GET_THREAD_ID,

	SYS_PROC_PING,
	SYS_PROC_READY_PING,

	SYS_PROC_GET_CMD,
	SYS_PROC_SET_CMD,
	SYS_PROC_GET_UID,
	SYS_PROC_SET_UID,

	//share memory syscalls
	SYS_PROC_SHM_ALLOC,
	SYS_PROC_SHM_MAP,
	SYS_PROC_SHM_UNMAP,
	SYS_PROC_SHM_REF,

	SYS_GET_SYSINFO,
	SYS_GET_PROCS,

	SYS_MMIO_MAP,
	SYS_FRAMEBUFFER_MAP,

	SYS_PROC_CRITICAL_ENTER,
	SYS_PROC_CRITICAL_QUIT,

	//internal proccess communication
	SYS_IPC_SETUP,
	SYS_IPC_CALL,
	SYS_IPC_GET_ARG,
	SYS_IPC_SET_RETURN,
	SYS_IPC_GET_RETURN,
	SYS_IPC_END,

	SYS_CORE_READY,
	SYS_GET_KEVENT
};

#endif
