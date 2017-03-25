#include "hlddz.h"
#include "game.h"
#include "client.h"
#include "proto.h"
#include "log.h"

extern HLDDZ hlddz;
extern Log xt_log;

char policy_cross_domain[] = "<cross-domain-policy>\
<allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>\0";
char policy_file[] = "<policy-file-request/>";

Client::Client(int fd_in) :
_ev_nodata_tstamp(60 * 320)
{
    /* set state */
    _state          = PARSE_HEADER;
    _cur_head_len   = 0;
    _header_p       = (struct Header*)_header;
    is_close        = 0;
	fd              = fd_in;
	is_err			= 0;
	uid				= -1;
	player          = NULL;
	position 		= POSITION_WAIT;
	is_robot_svr = false;
	
	_ev_read.data = this;
	ev_io_init(&_ev_read, Client::read_cb, fd, EV_READ);
	ev_io_start(hlddz.loop, &_ev_read);

    ev_io_init(&_ev_write, Client::write_cb, fd, EV_WRITE);

    _ev_nodata_timer.data = this;
    ev_timer_init(&_ev_nodata_timer, Client::nodata_timer_cb,
        _ev_nodata_tstamp, _ev_nodata_tstamp);
    ev_timer_start(hlddz.loop, &_ev_nodata_timer);
#if 0
    int set = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    //xt_log.debug("client[%d] open\n", fd);
}

Client::~Client()
{
	ev_io_stop(hlddz.loop, &_ev_read);
	ev_io_stop(hlddz.loop, &_ev_write);
	ev_timer_stop(hlddz.loop, &_ev_nodata_timer);
	while (!_write_q.empty()) {
		delete (_write_q.front());
		_write_q.pop_front();
	}
	close(fd);
	//xt_log.info("~client fd[%d] uid[%d] destrutor\n", fd, uid);
}

void Client::destroy(Client *client)
{
    //xt_log.info("client destroy fd[%d] uid[%d] destroy\n", client->fd, client->uid);
    hlddz.game->del_client(client);
}

void Client::pre_destroy(Client *client)
{
    if (client->_write_q.empty())
		Client::destroy(client);
    else
		client->is_close = 1;	
}

void Client::read_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	int ret;
	static char recv_buf[DEF_BUF_LEN];

	Client *self = (Client*) w->data;

	self->update_timer();

	if (self->_state == PARSE_HEADER) {
		ret = read(self->fd, &self->_header[self->_cur_head_len],
				sizeof(struct Header) - self->_cur_head_len);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
				xt_log.warn("read cb read header failed[%s]\n", strerror(errno));
				return;
			}
			xt_log.error("read header failed[%s]\n", strerror(errno));
			Client::destroy(self);
			return;
		}

		if (ret == 0) {
			xt_log.error("connection close in read header[%d]\n", self->fd);
			Client::destroy(self);
			return;
		}

		if (self->_header[0] == '<' && self->_header[1] == 'p'
				&& self->_header[2] == 'o' && self->_header[3] == 'l') {
			self->_state = PARSE_CROSS_DOMAIN;
			self->_cur_head_len = 0;
			self->_cur_policy_len = 4;
			return;
		}

		self->_cur_head_len += ret;

		if (self->_cur_head_len == sizeof(self->_header)) {
			if (self->_header_p->length > MAX_BUF_LEN
					|| self->_header_p->length == 0) {
				xt_log.error("fd[%d] recv an error len package, len:%d\n", self->fd, self->_header_p->length);
				Client::destroy(self);
				return;
			}

			self->_state = PARSE_BODY;
			self->_cur_head_len = 0;
			self->body.clear();
		}
	} else if (self->_state == PARSE_BODY) {
		ret = read(self->fd, recv_buf, self->_header_p->length - self->body.length());
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
				xt_log.debug("read body failed[%s]\n", strerror(errno));
				return;
			}
			xt_log.error("read body failed[%s]\n", strerror(errno));
			Client::destroy(self);
			return;
		}

		if (ret == 0) {
			xt_log.error("connection close in read body[%d]\n", self->fd);
			Client::destroy(self);
			return;
		}

		recv_buf[ret] = '\0';
		self->body.append(recv_buf);

		if (self->body.length() == self->_header_p->length) {
			self->_state = PARSE_HEADER;
			if (self->packet.parse(self->body) < 0) {
				xt_log.error("parse err!!\n");
				Client::destroy(self);
				return;
			}

			time_t begin = time(NULL);
			int ret = hlddz.game->dispatch(self);
			time_t end = time(NULL);
			int total = end - begin;
			if (total >= 1) {
				xt_log.error("slow cmd: [%d]\n", total);
			}
			if (ret < 0) {
				xt_log.error("dispatch err\n");
				pre_destroy(self);
				return;
			}
			if (self->is_err == 1) {
				xt_log.error("client is err\n");
				Client::destroy(self);
			}
		}
    } else if (self->_state == PARSE_CROSS_DOMAIN) {
		ret = read(self->fd, recv_buf, sizeof(policy_file) - self->_cur_policy_len);
		if (ret <= 0) {
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
				xt_log.warn("read body failed[%s]\n", strerror(errno));
				return;
			}
			xt_log.error("read body failed[%s]\n", strerror(errno));
			Client::destroy(self);
			return;
		}
		if (ret == 0) {
			xt_log.info("connection close in read body[%d]\n", self->fd);
			Client::destroy(self);
			return;
		}

		self->_cur_policy_len += ret;
		if (self->_cur_policy_len == sizeof(policy_file)) {
			/* completed */
			self->_state = PARSE_HEADER;
			self->_cur_policy_len = 0;
			self->send(policy_cross_domain, sizeof(policy_cross_domain));
			return;
		}
    }
}

void Client::write_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	Client *self = (Client*) w->data;

	if (self->_write_q.empty()) {
		//xt_log.debug("stop write event\n");
		ev_io_stop(EV_A_ w);
		if (self->is_close == 1) {
			Client::destroy(self);
			return;
		}
		if (self->cmd_type == 1) {
			Client::destroy(self);
			return;
		}
		return;
	}

	Buffer* buffer = self->_write_q.front();
	ssize_t written = write(self->fd, buffer->dpos(), buffer->nbytes());
	if (written < 0) {
		if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
			xt_log.warn("write failed[%s]\n", strerror(errno));
			return;
		}
		/* todo close this client */
		xt_log.error("unknow err in written [%d]\n", self->fd);
		Client::destroy(self);
		return;
	}

	buffer->pos += written;
	if (buffer->nbytes() == 0) {
		self->_write_q.pop_front();
		delete buffer;
	}
}

void Client::nodata_timer_cb(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Client *self = (Client*)w->data;
    xt_log.info("nodata_timer_cb client fd[%d] uid[%d] timeout\n", self->fd, self->uid);
//    Client::pre_destroy(self);   // cfc remark and add under if by 20140124
    if (self->player) {
    	hlddz.game->del_player(self->player);
    } else {
    	Client::pre_destroy(self);
    }
}

int Client::send(const char *buf, unsigned int len)
{
	if (fd > 0) {
		if (_write_q.empty()) {
			_ev_write.data = this;
			ev_io_start(hlddz.loop, &_ev_write);
			//xt_log.debug("start write event\n");
		}
		_write_q.push_back(new Buffer(buf, len));
		//xt_log.debug("write data\n", buf);
		return 0;
	}
	return -1;
}

int Client::send(const std::string &res)
{
	return send(res.c_str(), res.length());
	//return safe_writen(res.c_str(), res.length());
}

unsigned int Client::safe_writen(const char *buf, unsigned int len)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = (char*) buf;
	nleft = len;

	while (nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0) {
			if (errno == EINTR)
				nwritten = 0;
			else {
				is_err = 1;
				return -1;
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}

	return len;
}

int Client::update_timer()
{
    //ev_timer_stop(hlddz.loop, &_ev_nodata_timer);
    //ev_timer_start(hlddz.loop, &_ev_nodata_timer);
    ev_timer_again(hlddz.loop, &_ev_nodata_timer);
    //xt_log.debug("client[%d] update timer\n", fd);
    return 0;
}

void Client::join_table_failed(Client *client)
{
    /*
	Jpacket packet;
	packet.val["cmd"] = SERVER_LOGIN_ERR_UC;
	packet.val["code"] = 2;
	packet.val["msg"] = "no table";
	packet.end();
	client->send(packet.tostring());

	Client::pre_destroy(client);
    */
}

void Client::set_positon(int pos)
{
	position = pos;
}
