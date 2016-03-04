/**
 * Powder Toy - HTTP Library
 *
 * Copyright (c) 2008 - 2010 Stanislaw Skowronek.
 * Copyright (c) 2010 Simon Robertshaw
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */


#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN
#include <sys/param.h>
#endif
#if !defined(MACOSX) && !defined(BSD)
#include <malloc.h>
#endif
#include <time.h>
#ifdef WIN
#define _WIN32_WINNT 0x0501
//#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include "Config.h"
#include "Misc.h"
#include "HTTP.h"
#include "MD5.h"
#include "Platform.h"
#include "Format.h"
#include "Lang.h"

#ifdef WIN
#define PERROR SOCKET_ERROR
#define PERRNO WSAGetLastError()
#define PEAGAIN WSAEWOULDBLOCK
#define PEINTR WSAEINTR
#define PEINPROGRESS WSAEINPROGRESS
#define PEALREADY WSAEALREADY
#define PCLOSE closesocket
#else
#define PERROR -1
#define PERRNO errno
#define PEAGAIN EAGAIN
#define PEINTR EINTR
#define PEINPROGRESS EINPROGRESS
#define PEALREADY EALREADY
#define PCLOSE close
#endif

#ifdef _MSC_VER
#include <BaseTsd.h> //for SSIZE_T
typedef SSIZE_T ssize_t;
#endif

char * userAgent;
static int http_up = 0;
static long http_timeout = 15;
static int http_use_proxy = 0;
static struct sockaddr_in http_proxy;

static char * eatwhitespace(char * s)
{
	while(*s)
	{
		if(!(*s == ' ' || *s == '\t'))
			break;
		s++;
	}
	return s;
}

static int splituri(const char *uri, char **host, char **path)
{
	const char *q;
	char *x,*y;
	if (!strncmp(uri, "http://", 7))
		uri += 7;
	q = strchr(uri, '/');
	if (!q)
		q = uri + strlen(uri);
	x = (char *)malloc(q-uri+1);
	if (*q)
		y = mystrdup(q);
	else
	{
		y = mystrdup("/");
	}
	strncpy(x, uri, q-uri);
	x[q-uri] = 0;
	if (q==uri || x[q-uri-1]==':')
	{
		free(x);
		free(y);
		return 1;
	}
	*host = x;
	*path = y;
	return 0;
}

static char *getserv(char *host)
{
	char *q, *x = mystrdup(host);
	q = strchr(x, ':');
	if (q)
		*q = 0;
	return x;
}

static char *getport(char *host)
{
	char *p, *q;
	q = strchr(host, ':');
	if (q)
		p = mystrdup(q+1);
	else
		p = mystrdup("80");
	return p;
}

static int resolve(char *dns, char *srv, struct sockaddr_in *addr)
{
	struct addrinfo hnt, *res = 0;
	if (http_use_proxy)
	{
		memcpy(addr, &http_proxy, sizeof(struct sockaddr_in));
		return 0;
	}
	memset(&hnt, 0, sizeof(hnt));
	hnt.ai_family = AF_INET;
	hnt.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(dns, srv, &hnt, &res))
		return 1;
	if (res)
	{
		if (res->ai_family != AF_INET)
		{
			freeaddrinfo(res);
			return 1;
		}
		memcpy(addr, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);
		return 0;
	}
	return 1;
}

void http_init(char *proxy)
{
	char *host, *port;
#ifdef WIN
	WSADATA wsadata;
	if (!WSAStartup(MAKEWORD(2,2), &wsadata))
		http_up = 1;
#else
	signal(SIGPIPE, SIG_IGN);
	http_up = 1;
#endif
	if (proxy)
	{
		host = getserv(proxy);
		port = getport(proxy);
		if (resolve(host, port, &http_proxy))
			http_up = 0;
		else
			http_use_proxy = 1;
		free(host);
		free(port);
	}
	std::stringstream userAgentBuilder;
	userAgentBuilder << "PowderToy/" << SAVE_VERSION << "." << MINOR_VERSION << " ";
	userAgentBuilder << "(" << IDENT_PLATFORM << "; " << IDENT_BUILD << "; M0) ";
	userAgentBuilder << "TPTPP/" << SAVE_VERSION << "." << MINOR_VERSION << "." << BUILD_NUM << IDENT_RELTYPE << "." << SNAPSHOT_ID;
	std::string newUserAgent = userAgentBuilder.str();
	userAgent = new char[newUserAgent.length()+1];
	std::copy(newUserAgent.begin(), newUserAgent.end(), userAgent);
	userAgent[newUserAgent.length()] = 0;
	//"User-Agent: PowderToy/%d.%d (%s; %s; M%d) TPTPP/%d.%d.%d%s.%d\n", SAVE_VERSION, MINOR_VERSION, IDENT_PLATFORM, IDENT_BUILD, 0, SAVE_VERSION, MINOR_VERSION, BUILD_NUM, IDENT_RELTYPE, SNAPSHOT_ID
}

void http_done(void)
{
#ifdef WIN
	WSACleanup();
#endif
	http_up = 0;
}

#define CHUNK 4096

#define HTS_STRT 0
#define HTS_RSLV 1
#define HTS_CONN 2
#define HTS_IDLE 3
#define HTS_XMIT 4
#define HTS_RECV 5
#define HTS_DONE 6
struct http_ctx
{
	int state;
	time_t last;
	int keep;
	int ret;
	char *host, *path;
	char *thdr;
	int thlen;
	char *txd;
	int txdl;
	struct sockaddr_in addr;
	char *tbuf;
	int tlen, tptr;
	char *hbuf;
	int hlen, hptr;
	char *rbuf;
	int rlen, rptr;
	int chunked, chunkhdr, rxtogo, contlen, cclose;
	int fd;
	char *fdhost;
};
void *http_async_req_start(void *ctx, const char *uri, const char *data, int dlen, int keep)
{
	struct http_ctx *cx = (http_ctx *)ctx;
	if (!ctx)
	{
		ctx = calloc(1, sizeof(struct http_ctx));
		cx = (http_ctx *)ctx;
		cx->fd = PERROR;
	}

	if (!cx->hbuf)
	{
		cx->hbuf = (char *)malloc(256);
		cx->hlen = 256;
	}

	if (!http_up)
	{
		cx->ret = 604;
		cx->state = HTS_DONE;
		return ctx;
	}

	if (cx->state!=HTS_STRT && cx->state!=HTS_IDLE)
	{
		fprintf(stderr, "HTTP: unclean request restart state.\n");
		exit(1);
	}

	cx->keep = keep;
	cx->ret = 600;
	if (splituri(uri, &cx->host, &cx->path))
	{
		cx->ret = 601;
		cx->state = HTS_DONE;
		return ctx;
	}
	if (http_use_proxy)
	{
		free(cx->path);
		cx->path = mystrdup(uri);
	}
	if (cx->fdhost && strcmp(cx->host, cx->fdhost))
	{
		free(cx->fdhost);
		cx->fdhost = NULL;
		PCLOSE(cx->fd);
		cx->fd = PERROR;
		cx->state = HTS_STRT;
	}
	if (data)
	{
		if (!dlen)
			dlen = strlen(data);
		cx->txd = (char *)malloc(dlen);
		memcpy(cx->txd, data, dlen);
		cx->txdl = dlen;
	}
	else
		cx->txdl = 0;

	cx->contlen = 0;
	cx->chunked = 0;
	cx->chunkhdr = 0;
	cx->rxtogo = 0;
	cx->cclose = 0;

	cx->tptr = 0;
	cx->tlen = 0;

	cx->last = time(NULL);

	return ctx;
}

void http_async_add_header(void *ctx, const char *name, const char *data)
{
	struct http_ctx *cx = (http_ctx *)ctx;
	cx->thdr = (char *)realloc(cx->thdr, cx->thlen + strlen(name) + strlen(data) + 5);
	cx->thlen += sprintf(cx->thdr+cx->thlen, "%s: %s\r\n", name, data);
}

static void process_header(struct http_ctx *cx, char *str)
{
	char *p;
	if (cx->chunkhdr)
	{
		p = strchr(str, ';');
		if (p)
			*p = 0;
		cx->rxtogo = strtoul(str, NULL, 16);
		cx->chunkhdr = 0;
		if (!cx->rxtogo)
			cx->chunked = 0;
	}
	if (!str[0])
	{
		cx->rxtogo = cx->contlen;
		cx->chunkhdr = cx->chunked;
		if (!cx->contlen && !cx->chunked && cx->ret!=100)
			cx->state = HTS_DONE;
		return;
	}
	if (!strncmp(str, "HTTP/", 5))
	{
		p = strchr(str, ' ');
		if (!p)
		{
			cx->ret = 603;
			cx->state = HTS_DONE;
			return;
		}
		p++;
		cx->ret = atoi(p);
		return;
	}
	if (!strncmp(str, "Content-Length: ", 16))
	{
		str = eatwhitespace(str+16);
		cx->contlen = atoi(str);
		return;
	}
	if (!strncmp(str, "Transfer-Encoding: ", 19))
	{
		str = eatwhitespace(str+19);
		if(!strncmp(str, "chunked", 8))
		{
			cx->chunked = 1;
		}
		return;
	}
	if (!strncmp(str, "Connection: ", 12))
	{
		str = eatwhitespace(str+12);
		if(!strncmp(str, "close", 6))
		{
			cx->cclose = 1;
		}
		return;
	}
}

static void process_byte(struct http_ctx *cx, char ch)
{
	if (cx->rxtogo)
	{
		cx->rxtogo--;

		if (!cx->rbuf)
		{
			cx->rbuf = (char *)malloc(256);
			cx->rlen = 256;
		}
		if (cx->rptr >= cx->rlen-1)
		{
			cx->rlen *= 2;
			cx->rbuf = (char *)realloc(cx->rbuf, cx->rlen);
		}
		cx->rbuf[cx->rptr++] = ch;

		if (!cx->rxtogo && !cx->chunked)
			cx->state = HTS_DONE;
	}
	else
	{
		if (ch == '\n')
		{
			cx->hbuf[cx->hptr] = 0;
			process_header(cx, cx->hbuf);
			cx->hptr = 0;
		}
		else if (ch != '\r')
		{
			if (cx->hptr >= cx->hlen-1)
			{
				cx->hlen *= 2;
				cx->hbuf = (char *)realloc(cx->hbuf, cx->hlen);
			}
			cx->hbuf[cx->hptr++] = ch;
		}
	}
}

int http_async_req_status(void *ctx)
{
	struct http_ctx *cx = (http_ctx *)ctx;
	char *dns,*srv,buf[CHUNK];
	int tmp, i;
	time_t now = time(NULL);
#ifdef WIN
	unsigned long tmp2;
#endif

	switch (cx->state)
	{
	case HTS_STRT:
		dns = getserv(cx->host);
		srv = getport(cx->host);
		if (resolve(dns, srv, &cx->addr))
		{
			free(dns);
			free(srv);
			cx->state = HTS_DONE;
			cx->ret = 602;
			return 1;
		}
		free(dns);
		free(srv);
		cx->state = HTS_RSLV;
		return 0;
	case HTS_RSLV:
		cx->state = HTS_CONN;
		cx->last = now;
		return 0;
	case HTS_CONN:
		if (cx->fd == PERROR)
		{
			cx->fd = socket(AF_INET, SOCK_STREAM, 0);
			if (cx->fd == PERROR)
				goto fail;
			cx->fdhost = mystrdup(cx->host);
#ifdef WIN
			tmp2 = 1;
			if (ioctlsocket(cx->fd, FIONBIO, &tmp2) == SOCKET_ERROR)
				goto fail;
#else
			tmp = fcntl(cx->fd, F_GETFL);
			if (tmp < 0)
				goto fail;
			if (fcntl(cx->fd, F_SETFL, tmp|O_NONBLOCK) < 0)
				goto fail;
#endif
		}
		if (!connect(cx->fd, (struct sockaddr *)&cx->addr, sizeof(cx->addr)))
			cx->state = HTS_IDLE;
#ifdef WIN
		else if (PERRNO==WSAEISCONN)
			cx->state = HTS_IDLE;
#endif
#if defined(MACOSX) || defined(BSD)
		else if (PERRNO==EISCONN)
			cx->state = HTS_IDLE;
#endif
		else if (PERRNO!=PEINPROGRESS && PERRNO!=PEALREADY
#ifdef WIN
		         && PERRNO!=PEAGAIN && PERRNO!=WSAEINVAL
#endif
		        )
			goto fail;
		if (now-cx->last>http_timeout)
			goto timeout;
		return 0;
	case HTS_IDLE:
		if (cx->txdl)
		{
			// generate POST
			cx->tbuf = (char *)malloc(strlen(cx->host) + strlen(cx->path) + 132 + strlen(userAgent) + cx->txdl + cx->thlen);
			cx->tptr = 0;
			cx->tlen = 0;
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "POST %s HTTP/1.1\r\n", cx->path);
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "Host: %s\r\n", cx->host);
			if (!cx->keep)
				cx->tlen += sprintf(cx->tbuf+cx->tlen, "Connection: close\r\n");
			if (cx->thdr)
			{
				memcpy(cx->tbuf+cx->tlen, cx->thdr, cx->thlen);
				cx->tlen += cx->thlen;
				free(cx->thdr);
				cx->thdr = NULL;
				cx->thlen = 0;
			}
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "Content-Length: %d\r\n", cx->txdl);
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "User-Agent: %s\r\n", userAgent);
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "\r\n");
			memcpy(cx->tbuf+cx->tlen, cx->txd, cx->txdl);
			cx->tlen += cx->txdl;
			free(cx->txd);
			cx->txd = NULL;
			cx->txdl = 0;
		}
		else
		{
			// generate GET
			cx->tbuf = (char *)malloc(strlen(cx->host) + strlen(cx->path) + 98 + strlen(userAgent) + cx->thlen);
			cx->tptr = 0;
			cx->tlen = 0;
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "GET %s HTTP/1.1\r\n", cx->path);
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "Host: %s\r\n", cx->host);
			if (cx->thdr)
			{
				memcpy(cx->tbuf+cx->tlen, cx->thdr, cx->thlen);
				cx->tlen += cx->thlen;
				free(cx->thdr);
				cx->thdr = NULL;
				cx->thlen = 0;
			}
			if (!cx->keep)
				cx->tlen += sprintf(cx->tbuf+cx->tlen, "Connection: close\r\n");
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "User-Agent: %s\r\n", userAgent);
			cx->tlen += sprintf(cx->tbuf+cx->tlen, "\r\n");
		}
		cx->state = HTS_XMIT;
		cx->last = now;
		return 0;
	case HTS_XMIT:
		tmp = send(cx->fd, cx->tbuf+cx->tptr, cx->tlen-cx->tptr, 0);
		if (tmp==PERROR && PERRNO!=PEAGAIN && PERRNO!=PEINTR)
			goto fail;
		if (tmp!=PERROR)
		{
			cx->tptr += tmp;
			if (cx->tptr == cx->tlen)
			{
				cx->tptr = 0;
				cx->tlen = 0;
				free(cx->tbuf);
				cx->state = HTS_RECV;
			}
			cx->last = now;
		}
		if (now-cx->last>http_timeout)
			goto timeout;
		return 0;
	case HTS_RECV:
		tmp = recv(cx->fd, buf, CHUNK, 0);
		if (tmp==PERROR && PERRNO!=PEAGAIN && PERRNO!=PEINTR)
			goto fail;
		if (tmp!=PERROR)
		{
			for (i=0; i<tmp; i++)
			{
				process_byte(cx, buf[i]);
				if (cx->state == HTS_DONE)
					return 1;
			}
			cx->last = now;
		}
		if (now-cx->last>http_timeout)
			goto timeout;
		return 0;
	case HTS_DONE:
		return 1;
	}
	return 0;

fail:
	cx->ret = 600;
	cx->state = HTS_DONE;
	return 1;

timeout:
	cx->ret = 605;
	cx->state = HTS_DONE;
	return 1;
}

char *http_async_req_stop(void *ctx, int *ret, int *len)
{
	struct http_ctx *cx = (http_ctx *)ctx;
	char *rxd;

	if (cx->state != HTS_DONE)
		while (!http_async_req_status(ctx))
			Platform::Millisleep(1);

	if (cx->host)
	{
		free(cx->host);
		cx->host = NULL;
	}
	if (cx->path)
	{
		free(cx->path);
		cx->path = NULL;
	}
	if (cx->txd)
	{
		free(cx->txd);
		cx->txd = NULL;
		cx->txdl = 0;
	}
	if (cx->hbuf)
	{
		free(cx->hbuf);
		cx->hbuf = NULL;
	}
	if (cx->thdr)
	{
		free(cx->thdr);
		cx->thdr = NULL;
		cx->thlen = 0;
	}

	if (ret)
		*ret = cx->ret;
	if (len)
		*len = cx->rptr;
	if (cx->rbuf)
		cx->rbuf[cx->rptr] = 0;
	rxd = cx->rbuf;
	cx->rbuf = NULL;
	cx->rlen = 0;
	cx->rptr = 0;
	cx->contlen = 0;

	if (!cx->keep)
		http_async_req_close(ctx);
	else if (cx->cclose)
	{
		PCLOSE(cx->fd);
		cx->fd = PERROR;
		if (cx->fdhost)
		{
			free(cx->fdhost);
			cx->fdhost = NULL;
		}
		cx->state = HTS_STRT;
	}
	else
		cx->state = HTS_IDLE;

	return rxd;
}

void http_async_get_length(void *ctx, int *total, int *done)
{
	struct http_ctx *cx = (http_ctx *)ctx;
	if (done)
		*done = cx->rptr;
	if (total)
		*total = cx->contlen;
}

void http_async_req_close(void *ctx)
{
	struct http_ctx *cx = (http_ctx *)ctx;
	void *tmp;
	if (cx->host)
	{
		cx->keep = 1;
		tmp = http_async_req_stop(ctx, NULL, NULL);
		free(tmp);
	}
	free(cx->fdhost);
	PCLOSE(cx->fd);
	free(ctx);
}

char *http_simple_get(const char *uri, int *ret, int *len)
{
	void *ctx = http_async_req_start(NULL, uri, NULL, 0, 0);
	if (!ctx)
	{
		if (ret)
			*ret = 600;
		if (len)
			*len = 0;
		return NULL;
	}
	return http_async_req_stop(ctx, ret, len);
}
void http_auth_headers(void *ctx, const char *user, const char *pass, const char *session_id)
{
	char *tmp;
	int i;
	unsigned char hash[16];
	struct md5_context md5;

	if (user)
	{
		if (pass)
		{
			md5_init(&md5);
			md5_update(&md5, (unsigned char *)user, strlen(user));
			md5_update(&md5, (unsigned char *)"-", 1);

			md5_update(&md5, (unsigned char *)pass, strlen(pass));
			md5_final(hash, &md5);
			tmp = (char *)malloc(33);
			for (i=0; i<16; i++)
			{
				tmp[i*2] = hexChars[hash[i]>>4];
				tmp[i*2+1] = hexChars[hash[i]&15];
			}
			tmp[32] = 0;
			http_async_add_header(ctx, "X-Auth-Hash", tmp);
			free(tmp);
		}
		if (session_id)
		{
			http_async_add_header(ctx, "X-Auth-User-Id", user);
			http_async_add_header(ctx, "X-Auth-Session-Key", session_id);
		}
		else
		{
			http_async_add_header(ctx, "X-Auth-User", user);
		}
	}
}
char *http_auth_get(const char *uri, const char *user, const char *pass, const char *session_id, int *ret, int *len)
{
	void *ctx = http_async_req_start(NULL, uri, NULL, 0, 0);

	if (!ctx)
	{
		if (ret)
			*ret = 600;
		if (len)
			*len = 0;
		return NULL;
	}
	http_auth_headers(ctx, user, pass, session_id);
	return http_async_req_stop(ctx, ret, len);
}

char *http_simple_post(const char *uri, const char *data, int dlen, int *ret, int *len)
{
	void *ctx = http_async_req_start(NULL, uri, data, dlen, 0);
	if (!ctx)
	{
		if (ret)
			*ret = 600;
		if (len)
			*len = 0;
		return NULL;
	}
	return http_async_req_stop(ctx, ret, len);
}

const wchar_t *http_ret_wtext(int ret)
{
	switch (ret)
	{
	case 100:
		return TEXT_HTTP_STAT_100;
	case 101:
		return TEXT_HTTP_STAT_101;
	case 102:
		return TEXT_HTTP_STAT_102;

	case 200:
		return TEXT_HTTP_STAT_200;
	case 201:
		return TEXT_HTTP_STAT_201;
	case 202:
		return TEXT_HTTP_STAT_202;
	case 203:
		return TEXT_HTTP_STAT_203;
	case 204:
		return TEXT_HTTP_STAT_204;
	case 205:
		return TEXT_HTTP_STAT_205;
	case 206:
		return TEXT_HTTP_STAT_206;
	case 207:
		return TEXT_HTTP_STAT_207;

	case 300:
		return TEXT_HTTP_STAT_300;
	case 301:
		return TEXT_HTTP_STAT_301;
	case 302:
		return TEXT_HTTP_STAT_302;
	case 303:
		return TEXT_HTTP_STAT_303;
	case 304:
		return TEXT_HTTP_STAT_304;
	case 305:
		return TEXT_HTTP_STAT_305;
	case 306:
		return TEXT_HTTP_STAT_306;
	case 307:
		return TEXT_HTTP_STAT_307;

	case 400:
		return TEXT_HTTP_STAT_400;
	case 401:
		return TEXT_HTTP_STAT_401;
	case 402:
		return TEXT_HTTP_STAT_402;
	case 403:
		return TEXT_HTTP_STAT_403;
	case 404:
		return TEXT_HTTP_STAT_404;
	case 405:
		return TEXT_HTTP_STAT_405;
	case 406:
		return TEXT_HTTP_STAT_406;
	case 407:
		return TEXT_HTTP_STAT_407;
	case 408:
		return TEXT_HTTP_STAT_408;
	case 409:
		return TEXT_HTTP_STAT_409;
	case 410:
		return TEXT_HTTP_STAT_410;
	case 411:
		return TEXT_HTTP_STAT_411;
	case 412:
		return TEXT_HTTP_STAT_412;
	case 413:
		return TEXT_HTTP_STAT_413;
	case 414:
		return TEXT_HTTP_STAT_414;
	case 415:
		return TEXT_HTTP_STAT_415;
	case 416:
		return TEXT_HTTP_STAT_416;
	case 417:
		return TEXT_HTTP_STAT_417;
	case 418:
		return TEXT_HTTP_STAT_418;
	case 422:
		return TEXT_HTTP_STAT_422;
	case 423:
		return TEXT_HTTP_STAT_423;
	case 424:
		return TEXT_HTTP_STAT_424;
	case 425:
		return TEXT_HTTP_STAT_425;
	case 426:
		return TEXT_HTTP_STAT_426;
	case 444:
		return TEXT_HTTP_STAT_444;
	case 450:
		return TEXT_HTTP_STAT_450;
	case 499:
		return TEXT_HTTP_STAT_499;

	case 500:
		return TEXT_HTTP_STAT_500;
	case 501:
		return TEXT_HTTP_STAT_501;
	case 502:
		return TEXT_HTTP_STAT_502;
	case 503:
		return TEXT_HTTP_STAT_503;
	case 504:
		return TEXT_HTTP_STAT_504;
	case 505:
		return TEXT_HTTP_STAT_505;
	case 506:
		return TEXT_HTTP_STAT_506;
	case 507:
		return TEXT_HTTP_STAT_507;
	case 509:
		return TEXT_HTTP_STAT_509;
	case 510:
		return TEXT_HTTP_STAT_510;

	case 600:
		return TEXT_HTTP_STAT_600;
	case 601:
		return TEXT_HTTP_STAT_601;
	case 602:
		return TEXT_HTTP_STAT_602;
	case 603:
		return TEXT_HTTP_STAT_603;
	case 604:
		return TEXT_HTTP_STAT_604;
	case 605:
		return TEXT_HTTP_STAT_605;
	default:
		return TEXT_HTTP_STAT_UNKNOWN;
	}
}
char *http_multipart_post(const char *uri, const char *const *names, const char *const *parts, size_t *plens, const char *user, const char *pass, const char *session_id, int *ret, int *len)
{
	void *ctx;
	char *data = NULL, *tmp;
	int dlen = 0, i, j;
	unsigned char hash[16];
	unsigned char boundary[32], ch;
	int blen = 0;
	unsigned int map[62], m;
	struct md5_context md5;
	//struct md5_context md52;
	int own_plen = 0;

	if (names)
	{
		if (!plens)
		{
			own_plen = 1;
			for (i=0; names[i]; i++) ;
			plens = (size_t *)calloc(i, sizeof(size_t));
			for (i=0; names[i]; i++)
				plens[i] = strlen(parts[i]);
		}

retry:
		if (blen >= 31)
			goto fail;
		memset(map, 0, 62*sizeof(int));
		for (i=0; names[i]; i++)
		{
			for (ssize_t j=0; j<(ssize_t)plens[i]-blen; j++)
				if (!blen || !memcmp(parts[i]+j, boundary, blen))
				{
					ch = parts[i][j+blen];
					if (ch>='0' && ch<='9')
						map[ch-'0']++;
					else if (ch>='A' && ch<='Z')
						map[ch-'A'+10]++;
					else if (ch>='a' && ch<='z')
						map[ch-'a'+36]++;
				}
		}
		m = ~0;
		j = 61;
		for (i=0; i<62; i++)
			if (map[i]<m)
			{
				m = map[i];
				j = i;
			}
		if (j<10)
			boundary[blen] = '0'+j;
		else if (j<36)
			boundary[blen] = 'A'+(j-10);
		else
			boundary[blen] = 'a'+(j-36);
		blen++;
		if (map[j])
			goto retry;
		boundary[blen] = 0;

		for (i=0; names[i]; i++)
			dlen += blen+strlen(names[i])+plens[i]+128;
		dlen += blen+8;
		data = (char *)malloc(dlen);
		dlen = 0;
		for (i=0; names[i]; i++)
		{
			dlen += sprintf(data+dlen, "--%s\r\n", boundary);
			dlen += sprintf(data+dlen, "Content-transfer-encoding: binary\r\n");
			if (strchr(names[i], ':'))
			{
				tmp = mystrdup(names[i]);
				char *p = strchr(tmp, ':');
				*p = 0;
				dlen += sprintf(data+dlen, "content-disposition: form-data; name=\"%s\"; ", tmp);
				free(tmp);
				dlen += sprintf(data+dlen, "filename=\"%s\"\r\n\r\n", strchr(names[i], ':')+1);
			}
			else
				dlen += sprintf(data+dlen, "content-disposition: form-data; name=\"%s\"\r\n\r\n", names[i]);
			memcpy(data+dlen, parts[i], plens[i]);
			dlen += plens[i];
			dlen += sprintf(data+dlen, "\r\n");
		}
		dlen += sprintf(data+dlen, "--%s--\r\n", boundary);
	}

	ctx = http_async_req_start(NULL, uri, data, dlen, 0);
	if (!ctx)
		goto fail;

	if (user)
	{
		//http_async_add_header(ctx, "X-Auth-User", user);
		if (pass)
		{
			md5_init(&md5);
			md5_update(&md5, (unsigned char *)user, strlen(user));
			md5_update(&md5, (unsigned char *)"-", 1);
			m = 0;
			if (names)
			{
				for (i=0; names[i]; i++)
				{
					//md5_update(&md5, (unsigned char *)parts[i], plens[i]); //WHY?
					//md5_update(&md5, (unsigned char *)"-", 1);
					const char *p = strchr(names[i], ':');
					if (p)
						m += (p - names[i]) + 1;
					else
						m += strlen(names[i])+1;
				}

				tmp = (char *)malloc(m);
				m = 0;
				for (i=0; names[i]; i++)
				{
					const char *p = strchr(names[i], ':');
					if (m)
					{
						tmp[m] = ' ';
						m ++;
					}
					if (p)
					{
						memcpy(tmp+m, names[i], p-names[i]);
						m += p - names[i];
					}
					else
					{
						strcpy(tmp+m, names[i]);
						m += strlen(names[i]);
					}
				}
				tmp[m] = 0;
				http_async_add_header(ctx, "X-Auth-Objects", tmp);
				free(tmp);
			}

			md5_update(&md5, (unsigned char *)pass, strlen(pass));
			md5_final(hash, &md5);
			tmp = (char *)malloc(33);
			for (i=0; i<16; i++)
			{
				tmp[i*2] = hexChars[hash[i]>>4];
				tmp[i*2+1] = hexChars[hash[i]&15];
			}
			tmp[32] = 0;
			http_async_add_header(ctx, "X-Auth-Hash", tmp);
			free(tmp);
		}
		if (session_id)
		{
			http_async_add_header(ctx, "X-Auth-User-Id", user);
			http_async_add_header(ctx, "X-Auth-Session-Key", session_id);
		}
		else
		{
			http_async_add_header(ctx, "X-Auth-User", user);
		}
	}

	if (data)
	{
		tmp = (char *)malloc(32+strlen((char *)boundary));
		sprintf(tmp, "multipart/form-data, boundary=%s", boundary);
		http_async_add_header(ctx, "Content-type", tmp);
		free(tmp);
		free(data);
	}

	if (own_plen)
		free(plens);
	return http_async_req_stop(ctx, ret, len);

fail:
	free(data);
	if (own_plen)
		free(plens);
	if (ret)
		*ret = 600;
	if (len)
		*len = 0;
	return NULL;
}


void *http_multipart_post_async(const char *uri, const char *const *names, const char *const *parts, int *plens, const char *user, const char *pass, const char *session_id)
{
	void *ctx;
	char *data = NULL, *tmp;
	int dlen = 0, i, j;
	unsigned char hash[16];
	unsigned char boundary[32], ch;
	int blen = 0;
	unsigned int map[62], m;
	struct md5_context md5;
	//struct md5_context md52;
	int own_plen = 0;

	if (names)
	{
		if (!plens)
		{
			own_plen = 1;
			for (i=0; names[i]; i++) ;
			plens = (int *)calloc(i, sizeof(int));
			for (i=0; names[i]; i++)
				plens[i] = strlen(parts[i]);
		}

retry:
		if (blen >= 31)
			goto fail;
		memset(map, 0, 62*sizeof(int));
		for (i=0; names[i]; i++)
		{
			for (j=0; j<plens[i]-blen; j++)
				if (!blen || !memcmp(parts[i]+j, boundary, blen))
				{
					ch = parts[i][j+blen];
					if (ch>='0' && ch<='9')
						map[ch-'0']++;
					else if (ch>='A' && ch<='Z')
						map[ch-'A'+10]++;
					else if (ch>='a' && ch<='z')
						map[ch-'a'+36]++;
				}
		}
		m = ~0;
		j = 61;
		for (i=0; i<62; i++)
			if (map[i]<m)
			{
				m = map[i];
				j = i;
			}
		if (j<10)
			boundary[blen] = '0'+j;
		else if (j<36)
			boundary[blen] = 'A'+(j-10);
		else
			boundary[blen] = 'a'+(j-36);
		blen++;
		if (map[j])
			goto retry;
		boundary[blen] = 0;

		for (i=0; names[i]; i++)
			dlen += blen+strlen(names[i])+plens[i]+128;
		dlen += blen+8;
		data = (char *)malloc(dlen);
		dlen = 0;
		for (i=0; names[i]; i++)
		{
			dlen += sprintf(data+dlen, "--%s\r\n", boundary);
			dlen += sprintf(data+dlen, "Content-transfer-encoding: binary\r\n");
			if (strchr(names[i], ':'))
			{
				tmp = mystrdup(names[i]);
				char *p = strchr(tmp, ':');
				*p = 0;
				dlen += sprintf(data+dlen, "content-disposition: form-data; name=\"%s\"; ", tmp);
				free(tmp);
				dlen += sprintf(data+dlen, "filename=\"%s\"\r\n\r\n", strchr(names[i], ':')+1);
			}
			else
				dlen += sprintf(data+dlen, "content-disposition: form-data; name=\"%s\"\r\n\r\n", names[i]);
			memcpy(data+dlen, parts[i], plens[i]);
			dlen += plens[i];
			dlen += sprintf(data+dlen, "\r\n");
		}
		dlen += sprintf(data+dlen, "--%s--\r\n", boundary);
	}

	ctx = http_async_req_start(NULL, uri, data, dlen, 0);
	if (!ctx)
		goto fail;

	if (user)
	{
		//http_async_add_header(ctx, "X-Auth-User", user);
		if (pass)
		{
			md5_init(&md5);
			md5_update(&md5, (unsigned char *)user, strlen(user));
			md5_update(&md5, (unsigned char *)"-", 1);
			m = 0;
			if (names)
			{
				for (i=0; names[i]; i++)
				{
					//md5_update(&md5, (unsigned char *)parts[i], plens[i]); //WHY?
					//md5_update(&md5, (unsigned char *)"-", 1);
					const char *p = strchr(names[i], ':');
					if (p)
						m += (p - names[i]) + 1;
					else
						m += strlen(names[i])+1;
				}

				tmp = (char *)malloc(m);
				m = 0;
				for (i=0; names[i]; i++)
				{
					const char *p = strchr(names[i], ':');
					if (m)
					{
						tmp[m] = ' ';
						m ++;
					}
					if (p)
					{
						memcpy(tmp+m, names[i], p-names[i]);
						m += p - names[i];
					}
					else
					{
						strcpy(tmp+m, names[i]);
						m += strlen(names[i]);
					}
				}
				tmp[m] = 0;
				http_async_add_header(ctx, "X-Auth-Objects", tmp);
				free(tmp);
			}

			md5_update(&md5, (unsigned char *)pass, strlen(pass));
			md5_final(hash, &md5);
			tmp = (char *)malloc(33);
			for (i=0; i<16; i++)
			{
				tmp[i*2] = hexChars[hash[i]>>4];
				tmp[i*2+1] = hexChars[hash[i]&15];
			}
			tmp[32] = 0;
			http_async_add_header(ctx, "X-Auth-Hash", tmp);
			free(tmp);
		}
		if (session_id)
		{
			http_async_add_header(ctx, "X-Auth-User-Id", user);
			http_async_add_header(ctx, "X-Auth-Session-Key", session_id);
		}
		else
		{
			http_async_add_header(ctx, "X-Auth-User", user);
		}
	}

	if (data)
	{
		tmp = (char *)malloc(32+strlen((char *)boundary));
		sprintf(tmp, "multipart/form-data, boundary=%s", boundary);
		http_async_add_header(ctx, "Content-type", tmp);
		free(tmp);
		free(data);
	}

	if (own_plen)
		free(plens);

	return ctx;

fail:
	free(data);
	if (own_plen)
		free(plens);
	//if (ret)
	//	*ret = 600;
	//if (len)
	//	*len = 0;
	return NULL;
}
