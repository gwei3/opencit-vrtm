#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>



int main ( int argc, char** argv) {
	char buf[1024] = {0};
	int err = -1;
	int i = 100;
	int fd = -1;

	printf("Hello World\n");

	fd= open("/dev/rpmmio0", O_RDWR);
	if(fd<0) {
        	fprintf(stdout, "Can't open rpmmio driver %s\n", "/dev/rpmmio0");
        	fprintf(stdout, "Open error: %s\n", strerror(errno));
        	return -1;
        }
	
	printf("Hello World driver opened successfully\n");

		err = read(fd, buf, sizeof(buf));
		if (err < 0){
			fprintf(stdout, "read error:%d  %s\n", errno, strerror(errno));
			goto fail;
		}
		
		fprintf(stdout,"%d ", err);
		memset(buf, 0, sizeof(buf));
	

	err = write(fd, buf, i );
	if (err < 0){
		fprintf(stdout, "write error: %s\n", strerror(errno));
		goto fail;
	}

	fprintf(stdout, "Hey hey success\n");
fail:
	close (fd);

return 0;
}

