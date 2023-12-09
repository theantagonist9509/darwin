#include <sys/socket.h>
#include <stdint.h>

ssize_t recvAll(int fd, void *buf, size_t buf_size)
{
	size_t bytes_read = 0;
	ssize_t last_bytes_read = 0;
	uint8_t *buf_cursor = buf;
	while (bytes_read < buf_size) {
		last_bytes_read = recv(fd, buf_cursor, buf_size - bytes_read, 0);
		if (last_bytes_read <= 0) { // 0 or -1
			return last_bytes_read;
		}
		bytes_read += last_bytes_read;
		buf_cursor += last_bytes_read;
	}
	return bytes_read;
}

ssize_t sendAll(int fd, void const *buf, size_t buf_size)
{
	size_t bytes_written = 0;
	ssize_t last_bytes_written = 0;
	uint8_t const *buf_cursor = buf;
	while (bytes_written < buf_size) {
		last_bytes_written = send(fd, buf_cursor, buf_size - bytes_written, MSG_NOSIGNAL);
		if (last_bytes_written < 0) { // -1
			return last_bytes_written;
		}
		bytes_written += last_bytes_written;
		buf_cursor += last_bytes_written;
	}
	return bytes_written;
}
