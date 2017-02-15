#include "speaker.h"
#include "server.h"
#include "client.h"
#include "log.h"
#include "player.h"
#include "proto.h"
#include "eventlog.h"

extern Speaker speaker;
extern Log xt_log;

Client::Client(int fd_in) :
		_ev_nodata_tstamp(60 * 30)
{
	_state = PARSE_HEADER;
	_cur_head_len = 0;
	_header_p = (struct Header*)_header;
	is_close = 0;
	is_login = 0;
	fd = fd_in;

	_ev_read.data = this;
	ev_io_init(&_ev_read, Client::read_cb, fd, EV_READ);
	ev_io_start(speaker.loop, &_ev_read);

	ev_io_init(&_ev_write, Client::write_cb, fd, EV_WRITE);

	_ev_nodata_timer.data = this;
	ev_timer_init(&_ev_nodata_timer, Client::timer_cb, _ev_nodata_tstamp, _ev_nodata_tstamp);
	ev_timer_start(speaker.loop, &_ev_nodata_timer);
	xt_log.debug("client[%d] open.\n", fd);
}

Client::~Client()
{
	ev_io_stop(speaker.loop, &_ev_read);
	ev_io_stop(speaker.loop, &_ev_write);
	ev_timer_stop(speaker.loop, &_ev_nodata_timer);
	while (!_write_q.empty()) {
		delete _write_q.front();
		_write_q.pop_front();
	}
	close(fd);
//	xt_log.debug("client fd[%d] uid[%d] destrutor\n", fd, uid);
}

inline void Client::destroy(Client *c)
{
	speaker.server->del_client(c->fd);
	xt_log.debug("client destroy.\n");
}

void Client::pre_destroy(Client *c)
{
	if (c->_write_q.empty()) {
		Client::destroy(c);
	} else {
		c->is_close = 1;
	}
}

void Client::read_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	int ret;
	static char recv_buf[DEF_BUF_LEN];

	Client *c = (Client*)w->data;
	c->update_timer();

	if (c->_state == PARSE_HEADER) {
		ret = read(c->fd, &c->_header[c->_cur_head_len], sizeof(struct Header) - c->_cur_head_len);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
				xt_log.debug("read header failed[%s]\n", strerror(errno));
				return;
			}
			/*TODO read header return less than 0.*/
			xt_log.error("read header failed[%s].\n", strerror(errno));
			Client::destroy(c);
			return;
		}
		if (ret == 0) {
			/*TODO read header return equal 0.*/
			xt_log.error("connection close in read header[%d].\n", c->fd);
			Client::destroy(c);
			return;
		}

		c->_cur_head_len += ret;
		if (c->_cur_head_len == sizeof(c->_header)) {
			if (c->_header_p->length > MAX_BUF_LEN || c->_header_p->length == 0) {
				xt_log.error("fd[%d] receive an error length package.\n", c->fd);
				Client::destroy(c);
				return;
			}
			c->_state = PARSE_BODY;
			c->_cur_head_len = 0;
			c->body.clear();
		}
	} else if (c->_state == PARSE_BODY) {
		ret = read(c->fd, recv_buf, c->_header_p->length - c->body.length());
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
				xt_log.debug("read body failed[%s].\n", strerror(errno));
				return;
			}
			xt_log.debug("read body failed[%s].\n", strerror(errno));
			Client::destroy(c);
			return;
		}
		if (ret == 0) {
			xt_log.error("connection close in read body[%d].\n", c->fd);
			Client::destroy(c);
			return;
		}

		recv_buf[ret] = '\0';
		c->body.append(recv_buf);
		if (c->body.length() == c->_header_p->length) {
			c->_state = PARSE_HEADER;
			if (c->packet.parse(c->body) < 0) {
				xt_log.error("body parse error.\n");
				Client::destroy(c);
				return;
			}

			xt_log.debug("parse body.\n");
			if (speaker.server->dispatch(c) < 0){
				xt_log.error("dispatch error.\n");
				pre_destroy(c);
			}
		}
	}
}

void Client::write_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	Client *c = (Client*)w->data;
	if (c->_write_q.empty()) {
		ev_io_stop(EV_A_ w);
		if (c->is_close == 1) {
			Client::destroy(c);
		}
		if (c->cmd_type == 1) {
			Client::destroy(c);
			return;
		}
		return;
	}

	Buffer *buffer = c->_write_q.front();
	ssize_t written = write(c->fd, buffer->dpos(), buffer->nbytes());
	if (written < 0) {
		if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
			xt_log.debug("write failed[%s]\n", strerror(errno));
			return;
		}
		/*TODO write error. close this client */
		xt_log.error("unknow error in write [%d].\n", c->fd);
		Client::destroy(c);
		return;
	}

	buffer->pos += written;
	if (buffer->nbytes() == 0) {
		c->_write_q.pop_front();
		delete buffer;
	}
}

void Client::timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
	Client *c = (Client*)w->data;
	Client::pre_destroy(c);
}

int Client::send(const char *buf, unsigned int len)
{
	if (fd > 0) 
	{
		if (_write_q.empty()) 
		{
			_ev_write.data = this;
			ev_io_start(speaker.loop, &_ev_write);
		}

		_write_q.push_back(new Buffer(buf, len));
		return 0;
	}

	return -1;
}

int Client::update_timer() {
	ev_timer_again(speaker.loop, &_ev_nodata_timer);
	return 0;
}

int Client::init_client()
{
	int ret = speaker.main_rc[index]->command("hgetall hu:%d", uid);
	if (ret < 0) {
		xt_log.error("client init error, because get player infomation error.\n");
		return -1;
	}

	if (speaker.main_rc[index]->is_array_return_ok() < 0) {
		xt_log.error("client init error, because get player infomation error.\n");
		return -1;
	}

	skey = speaker.main_rc[index]->get_value_as_string("token");
	name = speaker.main_rc[index]->get_value_as_string("userName");
	sex = speaker.main_rc[index]->get_value_as_int("sex");
	exp = speaker.main_rc[index]->get_value_as_int("exp");
	money = speaker.main_rc[index]->get_value_as_int("money");
	rmb = speaker.main_rc[index]->get_value_as_int("rmb");
	coin = speaker.main_rc[index]->get_value_as_int("coin");
	total_board = speaker.main_rc[index]->get_value_as_int("totalBoard");
	total_win = speaker.main_rc[index]->get_value_as_int("totalWin");

	return 0;
}

int Client::hget_money()
{
	int ret = speaker.main_rc[index]->command("hget hu:%d money", uid);
	if (ret < 0) {
		xt_log.debug("client get money error.\n");
		return -1;
	}
	money = atoi(speaker.main_rc[index]->reply->str);

	return 0;
}


int Client::get_money()
{
	if(uid<XT_LOTTERY_ROBOT_MAX_ID)
	{
		return 2000000;
	}


	return money;
}





int Client::incr_money(int type, int value)
{
	if(uid< XT_LOTTERY_ROBOT_MAX_ID)
	{
		return 0;
	}



	int ret;

	if (type == 0) 
	{
		ret = speaker.main_rc[index]->command("hincrby hu:%d money %d", uid, value);
	}
   	else 
	{
		ret = speaker.main_rc[index]->command("hincrby hu:%d money -%d", uid, value);
	}

	if (ret < 0) 
	{
		xt_log.error("incr money uid[%d] incr_money error.\n", uid);
		return -1;
	}

	xt_log.info("Func[%s] incr money money old[%d] new[%d].\n", 
		__FUNCTION__, money, speaker.main_rc[index]->reply->integer);

	money = speaker.main_rc[index]->reply->integer; //update the money.

	return 0;
}



int Client::eventlog_update(int value)
{
	int ret;
	ret = commit_eventlog(uid, 0, value, money, 205, 2);
	if (ret < 0) {
		xt_log.error("eventlog update error value[%d].\n", value);
		return -1;
	}
	return 0;
}

int Client::incr_achievement_count(int type, int value)
{
	if (value == 0) {
		return 0;
	}
	int ret = speaker.main_rc[index]->command("hincrby st:%d lottery_count %d", uid, value);
	if (ret < 0) {
		xt_log.error("incr achievement count error type[%d] value[%d].\n", type, value);
		return -1;
	}
	return 0;
}
