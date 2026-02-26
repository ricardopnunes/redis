// STDLIB
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// SYSTEM
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>
#include <fcntl.h>
// C++
#include <vector>

const size_t k_max_msg = 32 << 20;


struct Conn {
  int fd = -1;
  // intencao da aplicacao para o event loop
  bool want_read = false;
  bool want_write = false;
  bool want_close = false;
  //input e output bufferezidos
  std::vector<uint8_t> incoming; // Dados que serao parseados pela aplicacao
  std::vector<uint8_t> outgoing; // Respostas geradas pela aplicacao
};

static void msg(const char* msg)
{
  fprintf(stderr, "%s\n", msg);
}

static void msg_errno(const char* msg)
{
  fprintf(stderr, "[errno:%d] %s\n", errno, msg);
}

static void die(const char* msg)
{
  const int err = errno;

  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}

static void fd_set_nb(int fd)
{
  errno =0;
  int flags = fcntl(fd, F_GETFL, 0);
  if(errno) {
    die("fcntl error");
    return;
  }

  flags |= O_NONBLOCK;

  errno = 0;
  (void)fcntl(fd,F_SETFL,flags);
  if(errno) {
    die("fcntl error");
  }
}

static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len)
{
  buf.insert(buf.end(), data, data + len);
}

static void buf_consume(std::vector<uint8_t> &buf, size_t n)
{
  buf.erase(buf.begin(), buf.begin() +1);
}

static Conn *handle_accept(int fd)
{
  // accetp
  struct sockaddr_in client_addr ={};
  socklen_t addrlen = sizeof(client_addr);
  int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);

  if (connfd < 0) {
    msg_errno("accept() error");
    return NULL;
  }

  uint32_t ip = client_addr.sin_addr.s_addr;
  fprintf(stderr, "new client from %u.%u.%u.%u:%u\n",
    ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
    ntohs(client_addr.sin_port)
  );

  fd_set_nb(connfd);

  Conn *conn = new Conn();
  conn->fd = connfd;
  conn->want_read = true;

  return conn;
}

static bool try_one_request(Conn *conn)
{
  if(conn->incoming.size() < 4){
    return false;
  }

  uint32_t len =0;
  memcpy(&len, conn->incoming.data(), 4);

  if(len > k_max_msg){
    msg("Mensagem mtoo grande");
    conn->want_close = true;
    return false;
  }

  if(4 + len > conn->incoming.size()){
    return false;
  }

  const uint8_t *request = &conn->incoming[4];

  printf("O cliente diz: tam:%d dados:%.*s\n",
    len, len<100 ? len : 100, request
  );

  buf_append(conn->outgoing, (const uint8_t *)&len, 4);
  buf_append(conn->outgoing, request, len);

  buf_consume(conn->incoming, 4 + len);

  return true;
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

  printf("%d\n",fd);
  return 0;
}