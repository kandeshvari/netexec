#pragma once

#include "log.h"
#include "list.h"
#include "http.h"
#include "ssl.h"
#include "job.h"
#include "jsonrpc.h"

#include "global.h"

//void free_ev_structs(struct ev_loop *loop, struct ev_io *watcher);
//void recv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
//static void readfd_cb(EV_P_ ev_io *w, int revents);
//static void sigchild_cb(EV_P_ ev_child *w, int revents);
file_watcher_t *spawn_process(job_t *job);
