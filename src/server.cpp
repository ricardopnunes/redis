#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <format>
#include <iostream>

const size_t k_max_msg = 4096;

static void die(const char* msg)
{
  const int err = errno;

  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}

static auto read_full(int fd, char* buf, size_t n)
{
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);

    if (rv <= 0) {
      return -1;
    }

    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }

  return 0;
}

static auto write_all(int fd, const char* buf, size_t n)
{
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);

    if (rv <= 0) {
      return -1;
    }

    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static void msg(const char* msg)
{
  fprintf(stderr, "%s\n", msg);
}

static void do_something(int connfd)
{
  char    rbuf[64] = {};
  ssize_t n        = read(connfd, rbuf, sizeof(rbuf) - 1);

  if (n < 0) {
    msg("read() error");
    return;
  }

  printf("O cliente diz: %s\n", rbuf);

  char wbuf[] = "world";
  write(connfd, wbuf, strlen(wbuf));
}

static int32_t one_request(int connfd)
{
  char rbuf[4 + k_max_msg];
  errno       = 0;
  int32_t err = read_full(connfd, rbuf, 4);

  if (err) {
    msg(errno == 0 ? "EOF" : "read() error");
    return err;
  }

  uint32_t len = 0;
  memcpy(&len, rbuf, 4);

  if (len > k_max_msg) {
    msg("Muitoooo Grande essa mensagem");
    return -1;
  }

  // Corpo da requisicao
  err = read_full(connfd, &rbuf[4], len);
  if (err) {
    msg("read) error");
    return err;
  }

  printf("O Cliente Diz: %.*s\n", len, &rbuf[4]);

  const char reply[] = "World";
  char       wbuf[4 + sizeof(reply)];

  len = (uint32_t)strlen(reply);
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], reply, len);
  return write_all(connfd, wbuf, 4 + len);
}

int main()
{
  // Criando o socket
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  // Setando valores defaults
  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // Criando o bind para o socket
  struct sockaddr_in addr = {};
  addr.sin_family         = AF_INET;
  addr.sin_port           = htons(1234);
  addr.sin_addr.s_addr    = htonl(0);

  int rv = bind(fd, (const struct sockaddr*)&addr, sizeof(addr));
  if (rv) {
    die("bind()");
  }

  // Criando o listener para o socket
  rv = listen(fd, SOMAXCONN);
  if (rv) {
    die("listen()");
  }

  while (true) {
    struct sockaddr_in client_addr = {};
    socklen_t          addrlen     = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*)&client_addr, &addrlen);

    if (connfd < 0) {
      continue;  // error
    }

    while (true) {
      int32_t err = one_request(connfd);

      if (err) {
        break;
      }
    }
    do_something(connfd);
    close(connfd);
  }

  std::cout << fd;
  return 0;
}