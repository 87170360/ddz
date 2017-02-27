#include "hlddz.h"
#include "log.h"
#include "eventlog.h"

extern DDZ ddz;
extern Log xt_log;

static int incr_eventlog(EventLog &el)
{
	int ret;
	char field[32];
	snprintf(field, 31, "%d%d", el.uid, el.ts);

	ret = ddz.eventlog_rc->command(
			"hmset log:%s uid %d tid %d vid %d zid %d type %d alter_type %d alter_value %d current_value %d ts %d",
			field, el.uid, el.tid, el.vid, el.zid, el.type, el.alter_type, el.alter_value, el.current_value, el.ts);

	if (ret < 0) {
		xt_log.error("incr eventlog error uid[%d] tid[%d].\n", el.uid, el.tid);
		return -1;
	}

	return 0;
}

int commit_eventlog(int my_uid, int my_tid, int my_alter_value, int my_current_value, int my_type, int my_alter_type)
{
	time_t ts;
	ts = time(NULL);

	EventLog el;
	el.uid = my_uid;
	el.tid = my_tid;
	el.vid = ddz.conf["tables"]["vid"].asInt();
	el.zid = ddz.conf["tables"]["zid"].asInt();
	el.type = my_type;
	el.alter_type = my_alter_type;
	el.alter_value = my_alter_value;
	el.current_value = my_current_value;
	el.ts = (int)ts;

	return incr_eventlog(el);
}


