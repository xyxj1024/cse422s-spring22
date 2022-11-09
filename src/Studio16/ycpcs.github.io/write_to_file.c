#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

int main(void) {

	// open the file
	int fd;

	fd = open("hello.txt", O_CREAT | O_WRONLY, 0600);
	if (fd < 0) {
		return 1;
	}

	// write some data to the file
	char buf[100] = "Hey there! Don't fall down.\n";
	ssize_t n = write(fd, buf, strlen(buf));
	if (n < 0) {
		return 1;
	}
	if (n < strlen(buf)) {
		return 2;
	}

	// close the file
	close(fd);

	return 0;

}
