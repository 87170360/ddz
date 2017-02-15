#ifndef __CLIENT_H__
#define __CLIENT_H__

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
#include "buffer.h"



enum position_state
{
	POSITION_WAIT = 0,
	POSITION_TABLE       ///---入座
};

class Player;

//封装socket通讯
class Client {
public:
    int                 cmd_type;   ///0 or 1  ???
    std::string         body;
	int 				fd;
	int					is_err;
	int					uid;
	int					vid;
	int					zid;
	std::string			skey;
    Jpacket             packet;
	Player*				player;      
    int                 is_close;
	int					position;
	bool            is_robot_svr;   ///是否为机器人
	
private:
    int                 _state;
	ev_io 				_ev_write;
	ev_io 				_ev_read;

    ev_timer            _ev_nodata_timer;
    ev_tstamp           _ev_nodata_tstamp;
	
    std::list<Buffer*>  _write_q;                       ///---待写（发送）队列
    char                _header[sizeof(struct Header)]; ///---4字节的字符数组
    struct Header       *_header_p;
    unsigned int        _cur_head_len;
    unsigned int        _cur_policy_len;

public:
    Client(int fd_in);
    virtual ~Client();
	static void destroy(Client *client);
	static void pre_destroy(Client *client);
    static void read_cb(struct ev_loop *loop, struct ev_io *w, int revents);
	static void write_cb(struct ev_loop *loop, struct ev_io *w, int revents);
    static void nodata_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents);
    int send(const char *buf, unsigned int len);
	int send(const std::string &res);
	unsigned int safe_writen(const char *buf, unsigned int len);
    int update_timer(void);
	static void join_table_failed(Client *client);
	void set_positon(int pos);
};


#endif // endif __CLIENT_H__
