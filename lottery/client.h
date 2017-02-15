#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <list>

#include <ev.h>
#include <json/json.h>

#include "jpacket.h"

#ifndef MAX_BUF_LEN
#   define MAX_BUF_LEN (1024*8)
#endif

#ifndef DEF_BUF_LEN
#   define DEF_BUF_LEN (1024*8)
#endif

enum parse_state {
	PARSE_HEADER    = 0,
	PARSE_BODY      = 1,
	PARSE_ERR       = 2,
};

struct Buffer {
	char     *data;
	ssize_t  len;
	ssize_t  pos;

	Buffer(const char *bytes, ssize_t nbytes) {
		pos = 0;
		len = nbytes;
		data = new char[nbytes];
		memcpy(data, bytes, nbytes);
	}

	virtual ~Buffer() {
		delete [] data;
	}

	char *dpos() {
		return data + pos;
	}

	ssize_t nbytes() {
		return len - pos;
	}
};

class Player;
///---≥ÈΩ±÷’∂À
class Client {
public:
	int cmd_type;
	int index;

	std::string body;
	int fd;
	Jpacket packet;
	Player *player;
	int is_close;
	int is_login;

	// client information
	int uid;
	std::string skey;
	std::string name;
	int sex;
	std::string birthday;
	std::string zone;
	std::string contact;
	std::string ps;
	std::string avatar;
	int exp;
	int rmb;
	int money;
	int coin;
	int total_board;
	int total_win;
	std::string best_cards;
	int pcount;
	int vtime;
	int vlevel;

private:
	int            _state;
	ev_io          _ev_write;
	ev_io          _ev_read;

	ev_timer       _ev_nodata_timer;
	ev_tstamp      _ev_nodata_tstamp;

	std::list<Buffer*> _write_q;
	char               _header[sizeof(struct Header)];
	struct Header      *_header_p;
	unsigned int       _cur_head_len;

public:
	Client(int fd_in);
	virtual ~Client();
	int init_client();
	void dump_client_info();
	static void destroy(Client *c);
	static void pre_destroy(Client *c);
	static void read_cb(struct ev_loop *loop, struct ev_io *w, int revents);
	static void write_cb(struct ev_loop *loop, struct ev_io *w, int revents);
	static void timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
	int send(const char *buf, unsigned int len);
	int send(const std::string &res) {return send(res.c_str(), res.length());}
	int update_timer(void);

	int incr_money(int type, int value);
	int hget_money();
	int eventlog_update(int value);  // cfc add 20140114
	int incr_achievement_count(int type, int value);

	int get_money();
};

#endif   /*_CLIENT_H_*/



