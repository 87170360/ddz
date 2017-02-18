#ifndef _DAEMONIZE_H_
#define _DAEMONIZE_H_

#ifdef    __cplusplus
extern "C" {
#endif

int daemonize();
int single_instance_running(const char* pid_file);

#ifdef    __cplusplus
}
#endif

#endif
