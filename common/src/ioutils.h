#ifndef IOUTILS_H
#define IOUTILS_H

ssize_t recvAll(int fd, void *buf, size_t buf_size);
ssize_t sendAll(int fd, void const *buf, size_t buf_size);

#endif /* IOUTILS_H */
