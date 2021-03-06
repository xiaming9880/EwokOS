#include <sys/vdevice.h>
#include <sys/vfs.h>
#include <sys/ipc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/kserv.h>
#include <sys/syscall.h>

static void do_open(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	fsinfo_t info;
	int oflag;
	int fd = proto_read_int(in);
	int ufid = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	oflag = proto_read_int(in);
	
	int res = 0;
	if(fd >= 0 && dev != NULL && dev->open != NULL) {
		if(dev->open(fd, ufid, from_pid, &info, oflag, p) != 0) {
			res = -1;
		}
	}
	PF->addi(out, res);
}

static void do_info(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	fsinfo_t info;
	const char* fname = proto_read_str(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	
	proto_t ret;
	PF->init(&ret, NULL, 0);
	PF->addi(out, -1);
	if(dev != NULL && dev->info != NULL) {
		if(dev->info(from_pid, fname, &info, &ret, p) == 0) {
			PF->clear(out)->addi(out, 0)->add(out, ret.data, ret.size);
		}
	}
}

static void do_close(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)out;
	fsinfo_t info;
	int fd = proto_read_int(in);
	int ufid = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));

	if(dev != NULL && dev->close != NULL) {
		dev->close(fd, ufid, from_pid, &info, p);
	}
}

static void do_read(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, offset, shm_id;
	fsinfo_t info;
	int fd = proto_read_int(in);
	int ufid = proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	size = proto_read_int(in);
	offset = proto_read_int(in);
	shm_id = proto_read_int(in);

	if(dev != NULL && dev->read != NULL) {
		void* buf;
		if(shm_id < 0)
			buf = malloc(size);
		else
			buf = shm_map(shm_id);

		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->read(fd, ufid, from_pid, &info, buf, size, offset, p);
			PF->addi(out, size);
			if(size > 0) {
				if(shm_id < 0) {
					PF->add(out, buf, size);
				}
			}

			if(shm_id >= 0)
				shm_unmap(shm_id);
			else
				free(buf);
		}
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_write(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int32_t size, offset, shm_id;
	fsinfo_t info;
	int fd = proto_read_int(in);
	int ufid = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));
	offset = proto_read_int(in);
	shm_id = proto_read_int(in);
	
	if(dev != NULL && dev->write != NULL) {
		void* data;
		if(shm_id < 0)
			data = proto_read(in, &size);
		else {
			size = proto_read_int(in);
			data = shm_map(shm_id);
		}

		if(data == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->write(fd, ufid, from_pid, &info, data, size, offset, p);
			PF->addi(out, size);
		}
		if(shm_id >= 0)
			shm_unmap(shm_id);
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_read_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int size, index, shm_id;
	size = proto_read_int(in);
	index = proto_read_int(in);
	shm_id = proto_read_int(in);

	if(dev != NULL && dev->read_block != NULL) {
		void* buf;
		if(shm_id < 0)
			buf = malloc(size);
		else
			buf = shm_map(shm_id);
		if(buf == NULL) {
			PF->addi(out, -1);
		}
		else {
			size = dev->read_block(from_pid, buf, size, index, p);
			PF->addi(out, size);
			if(size > 0) {
				if(shm_id < 0) {
					PF->add(out, buf, size);
				}
			}

			if(shm_id >= 0)
				shm_unmap(shm_id);
			else
				free(buf);
		}
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_write_block(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	int32_t size, index;
	void* data = proto_read(in, &size);
	index = proto_read_int(in);

	if(dev != NULL && dev->write_block != NULL) {
		size = dev->write_block(from_pid, data, size, index, p);
		PF->addi(out, size);
	}
	else {
		PF->addi(out, -1);
	}
}

static void do_dma(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	fsinfo_t info;
	int fd = proto_read_int(in);
	int ufid = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	int id = -1;	
	int size = 0;
	if(dev != NULL && dev->dma != NULL) {
		id = dev->dma(fd, ufid, from_pid, &info, &size, p);
	}
	PF->addi(out, id)->addi(out, size);
}

static void do_fcntl(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	fsinfo_t info;
	int fd = proto_read_int(in);
	uint32_t ufid = (uint32_t)proto_read_int(in);
	proto_read_to(in, &info, sizeof(fsinfo_t));
	int32_t cmd = proto_read_int(in);

	proto_t arg_in, arg_out;
	PF->init(&arg_out, NULL, 0);

	int32_t arg_size;
	void* arg_data = proto_read(in, &arg_size);
	PF->init(&arg_in, arg_data, arg_size);

	int res = -1;
	if(dev != NULL && dev->fcntl != NULL) {
		res = dev->fcntl(fd, ufid, from_pid, &info, cmd, &arg_in, &arg_out, p);
	}
	PF->clear(&arg_in);

	PF->addi(out, res)->add(out, arg_out.data, arg_out.size);
	PF->clear(&arg_out);
}

static void do_flush(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info;
	int fd = proto_read_int(in);
	memcpy(&info, proto_read(in, NULL), sizeof(fsinfo_t));

	if(dev != NULL && dev->flush != NULL) {
		dev->flush(fd, from_pid, &info, p);
	}
	PF->addi(out, 0);
}

static void do_create(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info_to, info;
	proto_read_to(in, &info_to, sizeof(fsinfo_t));
	proto_read_to(in, &info, sizeof(fsinfo_t));

	int res = 0;
	if(dev != NULL && dev->create != NULL) {
		res = dev->create(&info_to, &info, p);
	}

	PF->addi(out, res)->add(out, &info, sizeof(fsinfo_t));
}

static void do_unlink(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info_to, info;
	proto_read_to(in, &info_to, sizeof(fsinfo_t));
	const char* fname = proto_read_str(in);

	int res = 0;
	if(dev != NULL && dev->unlink != NULL) {
		res = dev->unlink(&info, fname, p);
	}
	PF->addi(out, res);
}

static void do_clear_buffer(vdevice_t* dev, int from_pid, proto_t *in, proto_t* out, void* p) {
	(void)from_pid;
	fsinfo_t info;
	proto_read_to(in, &info, sizeof(fsinfo_t));

	int res = -1;
	if(dev != NULL && dev->clear_buffer != NULL) {
		res = dev->clear_buffer(&info, p);
	}

	PF->addi(out, res);
}

static void do_safe_cmd(vdevice_t* dev, int cmd, int from_pid, proto_t *in, proto_t* out, void* p) {
	int res = -1;
	if(dev != NULL && dev->safe_cmd != NULL) {
		res = dev->safe_cmd(cmd, from_pid, in, p);
	}
	PF->addi(out, res);
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	vdevice_t* dev = (vdevice_t*)p;
	if(dev == NULL)
		return;
	p = dev->extra_data;

	switch(cmd) {
	case FS_CMD_OPEN:
		do_open(dev, from_pid, in, out, p);
		break;
	case FS_CMD_INFO:
		do_info(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CLOSE:
		do_close(dev, from_pid, in, out, p);
		break;
	case FS_CMD_READ:
		do_read(dev, from_pid, in, out, p);
		break;
	case FS_CMD_WRITE:
		do_write(dev, from_pid, in, out, p);
		break;
	case FS_CMD_READ_BLOCK:
		do_read_block(dev, from_pid, in, out, p);
		break;
	case FS_CMD_WRITE_BLOCK:
		do_write_block(dev, from_pid, in, out, p);
		break;
	case FS_CMD_DMA:
		do_dma(dev, from_pid, in, out, p);
		break;
	case FS_CMD_FLUSH:
		do_flush(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CNTL:
		do_fcntl(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CREATE:
		do_create(dev, from_pid, in, out, p);
		break;
	case FS_CMD_UNLINK:
		do_unlink(dev, from_pid, in, out, p);
		break;
	case FS_CMD_CLEAR_BUFFER:
		do_clear_buffer(dev, from_pid, in, out, p);
		break;
	default:
		if(cmd >= IPC_SAFE_CMD_BASE)
			do_safe_cmd(dev, cmd, from_pid, in, out, p);
		break;
	}
}

static int do_mount(vdevice_t* dev, fsinfo_t* mnt_point, int type) {
	fsinfo_t info;
	memset(&info, 0, sizeof(fsinfo_t));
	strcpy(info.name, mnt_point->name);
	info.type = type;
	vfs_new_node(&info);

	if(dev->mount != NULL) {
		if(dev->mount(&info, dev->extra_data) != 0) {
			vfs_del(&info);
			return -1;
		}
	}

	if(vfs_mount(mnt_point, &info) != 0) {
		vfs_del(&info);
		return -1;
	}
	memcpy(mnt_point, &info, sizeof(fsinfo_t));
	return 0;
}

int device_run(vdevice_t* dev, const char* mnt_point, int mnt_type) {
	if(dev == NULL)
		return -1;

	fsinfo_t mnt_point_info;
	if(mnt_point != NULL) {
		if(strcmp(mnt_point, "/") != 0)
			vfs_create(mnt_point, &mnt_point_info, mnt_type);
		else
			vfs_get(mnt_point, &mnt_point_info);

		if(do_mount(dev, &mnt_point_info, mnt_type) != 0)
			return -1;
	}

	if(dev->loop_step != NULL) 
		kserv_run(handle, dev, true);
	else
		kserv_run(handle, dev, false);

	while(1) {
		if(dev->loop_step != NULL) {
			dev->loop_step(dev->extra_data);
			usleep(1000);
		}
		else {
			sleep(1);
		}
	}

	if(mnt_point != NULL && dev->umount != NULL) {
		return dev->umount(&mnt_point_info, dev->extra_data);
	}
	vfs_umount(&mnt_point_info);
	return 0;
}
