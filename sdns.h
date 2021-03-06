#ifndef SDNS_H
#define SDNS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#define snprintf(buf, size, format, ...) _snprintf_s(buf, size, size-1, format, __VA_ARGS__)
#define strncpy(dest, src, n) strncpy_s(dest, sizeof(dest), src, n)
#define strcasecmp _stricmp
#define strdup _strdup
#define sscanf sscanf_s

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#endif

#ifndef __APPLE__
#include "openssl/evp.h"
#endif /*__APPLE__*/

#include "lruhash.h"
#include "locks.h"
#define DOMAIN_MAX_SIZE 256
#define DNS_DEFAULT_DATA_SIZE 512

struct query_info {
  char *node;
};

struct host_info {
  //host address type: AF_INET or AF_INET6
  int h_addrtype;

  /*length of address in bytes:
    sizeof(struct in_addr) or sizeof(in_addr6)
  */
  int h_length;

  //length of addr list
  int addr_list_len;
  //list of address
  char **h_addr_list;
};

struct reply_info {
  struct host_info *host;

  time_t  ttl;
  time_t  prefetch_ttl;
};

struct msgreply_entry {
  //hash table key
  struct query_info key;
  //hash table entry, data is struct reply_info
  struct lruhash_entry entry;
};

struct prefetch_stat {
  //query info
  struct query_info qinfo;
  
  //next in linked list
  struct prefetch_stat *next;
};

struct prefetch_stat_list {
  lock_basic_t lock;
  int used;
  struct prefetch_stat *head;
};

//sdns enviroment
struct sdns_env {
  //dns cache
  struct lruhash *cache;
  //max memory of dns cache
  size_t cache_maxmem;
  
  //min cache ttl
  int min_ttl;
  
  //prefetch job list;
  struct prefetch_stat_list *prefetch_list;

  //http dns server and port
  char *serv_ip;
  unsigned int port;
  
  //dev_used
  int des_used;
  int des_id;
  char *des_key;
};

/* Begin added by xie hui */

extern char *public_dnsserver;
extern unsigned int public_dnsport;

uint32_t http_response_errorcode;

void sdns_set_publicdns_server_port(char *servip, unsigned int servport);
void sdns_set_server_port(char *servip, unsigned int servport);
/*End added by xiehui*/
/**API*/

//set cache and ttl before init env
void sdns_set_cache_mem(size_t maxmem);
void sdns_set_ttl(int ttl);

//enterprise version interface
void sdns_set_des_id_key(uint32_t id, const char *key);
char *sdns_des_encrypt(const char *domain);
char *sdns_des_decrypt(const char *des_ip);

//sdns enviroment init and destroy
void sdns_env_init();
void sdns_env_destroy();

//flush host name from cache
void sdns_flush_cache(const char *node);

//sdns cache status
void sdns_cache_status();

//similar with getaddrinfo
int sdns_getaddrinfo(const char *node, const char *service,
  const struct addrinfo *hints, struct addrinfo **res);

//similar with freeaddrinfo
void sdns_freeaddrinfo(struct addrinfo *res);

/***/

/**internal functions*/

int wait_readable(int sockfd, struct timeval timeout);
int wait_writable(int sockfd, struct timeval timeout);

struct host_info *http_query(const char *node, time_t *ttl);
struct host_info *dns_query(const char *node, time_t *ttl);

//http request api
int make_connection(char *serv_ip, int port);
int make_request(int sockfd, char *hostname, char *request_path);
int fetch_response(int sockfd, char *http_data, size_t http_data_len);

//dns request api
int make_dns_query_format(const char *node, char *buf, int *query_len);
int make_dns_query(char *buf, int query_len, time_t *ttl, int *Anum);

/**for tests**/

extern struct sdns_env *sdnse;

//hash function
hashvalue_t query_info_hash(struct query_info *q);

void host_info_clear(struct host_info *host);

//prefetch job struct and function
struct prefetch_job_info {
  struct query_info qinfo;
  hashvalue_t hash;
};
int prefetch_new_query(struct query_info *qinfo, hashvalue_t hash);

/***/
#endif
