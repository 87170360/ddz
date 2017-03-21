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

#include "hlddz.h"
#include "log.h"
#include "game.h"
#include "proto.h"
#include "client.h"
#include "player.h"
#include "table.h"

extern HLDDZ hlddz;
extern Log xt_log;

Game::Game()
{
    robot_client = NULL;
}

Game::~Game()
{
}

void dump_game_info(char *tag)
{
}

int Game::start()
{
    /* first init table */
    init_table();

    init_accept();

    return 0;
}

/* init table from config
 * todo read config from db or other */
int Game::init_table()
{
    for (int i = hlddz.conf["tables"]["begin"].asInt(); i < hlddz.conf["tables"]["end"].asInt(); i++)
    {
        Table *table = new Table();
        if (table->init(i) < 0) {
            xt_log.error("init table tid[%d] init err.\n", i);
            exit(1);
        }
        zero_tables[i] = table;
        all_tables[i] = table;
    }
    xt_log.info("total tables[%d]\n", zero_tables.size());

    return 0;
}

int Game::init_accept()
{
    xt_log.info("Listening on %s:%d\n",
            hlddz.conf["game"]["host"].asString().c_str(),
            hlddz.conf["game"]["port"].asInt());

    struct sockaddr_in addr;

    _fd = socket(PF_INET, SOCK_STREAM, 0);
    if (_fd < 0) {
        xt_log.error("File[%s] Line[%d]: socket failed: %s\n", __FILE__, __LINE__, strerror(errno));
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(hlddz.conf["game"]["port"].asInt());
    addr.sin_addr.s_addr = inet_addr(hlddz.conf["game"]["host"].asString().c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        xt_log.error("game::init_accept Incorrect ip address!");
        close(_fd);
        _fd = -1;
        exit(1);
    }

    int on = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        xt_log.error("File[%s] Line[%d]: setsockopt failed: %s\n", __FILE__, __LINE__, strerror(errno));
        close(_fd);
        return -1;
    }

    if (bind(_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        xt_log.error("File[%s] Line[%d]: bind failed: %s\n", __FILE__, __LINE__, strerror(errno));
        close(_fd);
        return -1;
    }

    fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);
    listen(_fd, 10000);

    _ev_accept.data = this;
    ev_io_init(&_ev_accept, Game::accept_cb, _fd, EV_READ);
    ev_io_start(hlddz.loop, &_ev_accept);

    xt_log.info("listen ok\n");

    return 0;
}

void Game::accept_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
    if (EV_ERROR & revents) {
        xt_log.error("got invalid event\n");
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int fd = accept(w->fd, (struct sockaddr *) &client_addr, &client_len);
    if (fd < 0) {
        xt_log.error("accept error[%s]\n", strerror(errno));
        return;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

    Client *client = new (std::nothrow) Client(fd);
    Game *game = (Game*) (w->data);
    if (client) {
        game->fd_client[fd] = client;
    } else
        close(fd);
}

void Game::del_client(Client *client)
{
    if (fd_client.find(client->fd) == fd_client.end()) {
        xt_log.error("del client free client err[miss].\n");
        return;
    }
    fd_client.erase(client->fd);

    // todo
    if (client->player) {
        Player *player = client->player;
        if (client->position == POSITION_WAIT) {
            // todo delete this status
            if (offline_players.find(player->uid) != offline_players.end()) {
                offline_players.erase(player->uid);
                //xt_log.info("del client player uid[%d] offline.\n", player->uid);
            }
            if (online_players.find(player->uid) != online_players.end()) {
                online_players.erase(player->uid);
                //xt_log.info("del client player uid[%d] online.\n", player->uid);
            }
            delete player;
        } else if (client->position == POSITION_TABLE) {
            if (online_players.find(player->uid) != online_players.end()) {
                online_players.erase(player->uid);
                offline_players[player->uid] = client->player;
                player->start_offline_timer();
                player->client = NULL;
                xt_log.debug("del client player uid[%d] online and add this uid to offline\n", player->uid);
            }
            client->player->client = NULL;
        }
    }
    if (client->is_robot_svr) {
        robot_client = NULL;
    }

    //xt_log.info("del client fd[%d].\n", client->fd);
    delete client;
    dump_game_info("del_client");
}

int Game::dispatch(Client *client)
{
    client->cmd_type = 0;
    int cmd = client->packet.safe_check();
    if (cmd < 0) 
    {
        xt_log.error("the cmd format is error.\n");
        return -1;
    }

    if (cmd == CLIENT_LOGIN) 
    {
        if (client->player == NULL) 
        {
            int ret = add_player(client);
            if (ret == -1) 
            {
                return -1;
            } 
            else if (ret == 1) 
            {//断线重连
                all_tables[client->player->m_tid]->reLogin(client->player);
                return 0;
            }
            else if (ret == 2) 
            {//断线重连
                all_tables[client->player->m_tid]->reLogin(client->player);
                return 0;
            }

            ret = handler_login_table(client);
            return ret;
        }
        xt_log.error("CLIENT_LOGIN_REQ player must be NULL.\n");
        return -1;
    }

    if (safe_check(client, cmd) < 0) {
        return -1;
    }

    Player *player = client->player;
    // dispatch 
    switch (cmd) {
        case CLIENT_PREPARE:
            {
                all_tables[player->m_tid]->msgPrepare(player);
            } 
            break;
        case CLIENT_CALL:
            {
                all_tables[player->m_tid]->msgCall(player);
            } 
            break;
        case CLIENT_DOUBLE:
            {
                all_tables[player->m_tid]->msgDouble(player);
            } 
            break;
        case CLIENT_OUT:
            {
                all_tables[player->m_tid]->msgOut(player);
            } 
            break;
        default:
            {
               xt_log.error("invalid command[%d]\n", cmd);
               return -1;
            }
    }

    // xt_log.debug("dispatch succ\n");
    return 0;
}

int Game::safe_check(Client *client, int cmd)
{
    if (online_players.find(client->uid) == online_players.end()) {
        xt_log.error("safe check uid[%d] must be online player.\n", client->uid);
        return -1;
    }

    Player *player = client->player;
    if (player == NULL) {
        xt_log.error("safe check client player is NULL.\n");
        return -1;
    }
    if (all_tables.find(player->m_tid) == all_tables.end())
    {
        xt_log.error("safe_check uid[%d] is not in tid[%d]\n", player->uid, player->m_tid);
        return -1;
    }

    return 0;
}

int Game::handler_login_table(Client *client)
{
    Player *player = client->player;
    if (client->position == POSITION_TABLE) {
        xt_log.error("handler_login_table uid[%d] have been in table\n", player->uid);
        return -1;
    }
    int ret = 0;

    ret = login_table(client, two_tables, three_tables);
    if (ret == 0)
        return 0;
    else if (ret == -2)
        return -2;

    ret = login_table(client, one_tables, two_tables);
    if (ret == 0)
        return 0;
    else if (ret == -2)
        return -2;

    ret = login_table(client, zero_tables, one_tables);
    if (ret == 0)
        return 0;
    else if (ret == -2)
        return -2;

    xt_log.error("handler login table no seat.\n");
    return -1;
}

int Game::login_table(Client *client, std::map<int, Table*> &a, std::map<int, Table*> &b)
{
    if(a.empty())
    {
        return -1;
    }

    Player* player = client->player;
    Table* target = NULL;
    for(map<int, Table*>::iterator it = a.begin(); it != a.end(); it++)
    {
        Table *table = (*it).second; 
        if(table->m_state != STATE_PREPARE || player->m_tid == table->m_tid || table->m_players.find(player->uid) != table->m_players.end())
        {
            continue;
        }
        if(table->m_players.find(player->uid) != table->m_players.end())
        {
            xt_log.error("login table uid[%d] is in tid[%d]\n", player->uid, table->m_tid); 
            return -2;
        }
        target = table;
        break;
    }

    if(target == NULL)
    {
        xt_log.error("%s:%d, not found table!\n", __FILE__, __LINE__); 
        return 0;
    }

    a.erase(target->m_tid);
    b[target->m_tid] = target;

    client->set_positon(POSITION_TABLE);
    target->login(client->player);
    return 0;
}

int Game::handle_logout_table(int tid)
{
    map<int, Table*>::iterator it;
    it = all_tables.find(tid);
    if (it == all_tables.end()) {
        return -1;
    }
    Table *table = (*it).second;

    it = three_tables.find(tid);
    if (it != three_tables.end()) {
        three_tables.erase(it);
        two_tables[tid] = table;
        return 0;
    }

    it = two_tables.find(tid);
    if (it != two_tables.end()) {
        two_tables.erase(it);
        one_tables[tid] = table;
        return 0;
    }

    it = one_tables.find(tid);
    if (it != one_tables.end()) {
        one_tables.erase(it);
        zero_tables[tid] = table;
        return 0;
    }

    return -1;
}

int Game::send_error(Client *client, int cmd, int error_code)
{
    Jpacket error;
    error.val["cmd"] = cmd;
    error.val["err"] = error_code;
    error.end();
    return client->send(error.tostring());
}

int Game::check_skey(Client *client)
{
    if (client->uid < XT_ROBOT_UID_MAX) 
    {
        return 0;
    }
#if 1
    int i = client->uid % hlddz.main_size;
    int ret = hlddz.main_rc[i]->command(" hget u:%d skey", client->uid);
    if (ret < 0) {
        xt_log.error("check skey error, because get player infomation error.\n");
        return -1;
    }

    xt_log.debug("skey [%s] [%s]\n", client->skey.c_str(), hlddz.main_rc[i]->reply->str);
    if (hlddz.main_rc[i]->reply->str && client->skey.compare(hlddz.main_rc[i]->reply->str) != 0) {
        xt_log.error("check skey error, client[%s] server[%s].\n", client->skey.c_str(), hlddz.main_rc[i]->reply->str);
        return -1;
    }
#endif	
    return 0;
}

int Game::add_player(Client *client)
{
    Json::Value &val = client->packet.tojson();
    int uid = val["uid"].asInt();
    client->uid = uid;
    client->skey = val["skey"].asString();
    client->vid = val["vid"].asInt();
    client->zid = val["zid"].asInt();

    if (check_skey(client) < 0) 
    {
        Jpacket packet;
        packet.val["cmd"] = SERVER_RESPOND;
        packet.val["code"] = 505;
        packet.val["msg"] = "skey error";
        packet.end();
        client->send(packet.tostring());
        return -1;
    }

    /* rebind by online */
    if (online_players.find(uid) != online_players.end()) {
        xt_log.debug("player[%d] rebind by online get info ok\n", uid);
        Player *player = online_players[uid];
        if (all_tables.find(player->m_tid) == all_tables.end()) 
        {
            xt_log.error("add player rebind by online uid[%d] is not in tid[%d].\n", player->uid, player->m_tid);
            return -1;
        }
        Client *oldClient = player->client;
        player->set_client(client);
        client->set_positon(POSITION_TABLE);
        fd_client.erase(oldClient->fd);
        delete oldClient;
        player->stop_offline_timer();
        dump_game_info("rebind by online");
        return 2;
    }

    /* rebind by offline */
    if (offline_players.find(uid) != offline_players.end())
    {
        xt_log.debug("player[%d] rebind by offline get info ok\n", uid);
        Player *player = offline_players[uid];
        if (all_tables.find(player->m_tid) == all_tables.end())
        {
            xt_log.error("rebind by offline uid[%d] is not in table[%d]\n", player->uid, player->m_tid);
            return -1;
        }
        offline_players.erase(uid);
        online_players[uid] = player;

        player->set_client(client);
        client->set_positon(POSITION_TABLE);
        player->stop_offline_timer();
        dump_game_info("rebind by offline");
        return 1;
    }

    /* set player info */
    Player *player = new (std::nothrow) Player();
    if (player == NULL) 
    {
        xt_log.error("new player err");
        return -1;
    }

    player->set_client(client);
    int ret = player->init();
    if (ret < 0) 
    {
        return -1;
    }
    online_players[uid] = player;


    if(hlddz.cache_rc->command("hset gameinfo %d %d", hlddz.conf["game"]["port"].asInt(), fd_client.size()) < 0)
    {
        xt_log.error("cache redis error(set gameinfo");
    }

    //xt_log.info("add player uid[%d] login success.\n", uid);
    return 0;
}

int Game::del_player(Player *player)
{
    int ret = 0;
    if (all_tables.find(player->m_tid) != all_tables.end()) {
        if (ret < 0) {
            xt_log.error("del player table handler logout\n");
            return -1;
        }
        if (ret < 0) {
            xt_log.error("del player table del player\n");
            return -1;
        }
        ret = handle_logout_table(player->m_tid);
        if (ret < 0) {
            xt_log.error("del player table handle logout table.\n");
            return -1;
        }
    }

    if (offline_players.find(player->uid) != offline_players.end()) {
        offline_players.erase(player->uid);
        //xt_log.info("del player uid[%d] offline.\n", player->uid);
    }

    if (online_players.find(player->uid) != online_players.end()) {
        online_players.erase(player->uid);
        xt_log.info("del player uid[%d] online.\n", player->uid);
    }

    if (player->client) {
        Client *client = player->client;
        client->position = POSITION_WAIT;
        Client::pre_destroy(client);
        client->player = NULL;
        delete player;
        dump_game_info("del_player");
        return 0;
    }

    xt_log.debug("del player[%p] uid[%d]\n", player, player->uid);
    delete player;

    if(hlddz.cache_rc->command("hset gameinfo %d %d",hlddz.conf["game"]["port"].asInt(),fd_client.size()))
    {
        xt_log.error("cache redis error(set gameinfo");
    }

    return 0;
}

int Game::change_table(Player *player)
{
    xt_log.info("change table uid[%d] money[%d] tid[%d].\n", player->uid, player->money, player->m_tid);
    int ret = 0;
    if (all_tables.find(player->m_tid) != all_tables.end()) {
        player->logout_type = 2;   
        if (ret < 0) {
            xt_log.error("change table handler logout error.\n");
            return -1;
        }
        if (ret < 0) {
            xt_log.error("change table del player error.\n");
            return -1;
        }
        ret = handle_logout_table(player->m_tid);
        if (ret < 0) {
            xt_log.error("change table handle logout table error.\n");
            return -1;
        }
        Client *client = player->client;
        client->position = POSITION_WAIT;
        dump_game_info("handler_login_table begin.");
        ret = handler_login_table(client);
        dump_game_info("handler_login_table end.");
    }
    return 0;
}
