/*
 * eventlog.h
 *
 *  Created on: 2014年1月2日
 *      Author: chenfc
 */

#ifndef EVENTLOG_H_
#define EVENTLOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
///---事件日志(写Redis库的log表)
struct EventLog {
	int uid;           // uid为0，表示台费
	int tid;
	int vid;
	int zid;
	int type;          // 流水类型 200玩牌 201大喇叭 202淘汰场金币 203兑奖券 204RMB 205彩票 206互动表情
	int alter_type;    // 更改类型   1 rmb  2 money  3 coin
	int alter_value;   // 更改的值
	int current_value; // 当前的值
	int ts;            // 当前时间戳
};

extern int commit_eventlog(int my_uid, int my_tid, int my_alter_value, int my_current_value, int my_type, int my_alter_type);



#endif /* EVENTLOG_H_ */
