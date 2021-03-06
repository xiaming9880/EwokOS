#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/global.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	int i=0;
	proto_t in;
	PF->init(&in, NULL, 0)->adds(&in, "hello!");
	proto_t r;
	PF->init(&r, NULL, 0);
	while(1) {
		set_global("test.test", &in);	
		usleep(1000);
		if(get_global("test.test", &r) == 0) {	
			printf("ret:'%s'\n", proto_read_str(&r));
		}
		PF->clear(&r);
		i++;
	}
	PF->clear(&in);
  return 0;
}

