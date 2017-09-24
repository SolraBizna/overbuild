/*
  Copyright (C) 2014, 2015, 2016 Solra Bizna

  This file is part of overbuild.

  overbuild is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  overbuild is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with overbuild.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/select.h>

#include <string>
#include <list>
#include <unordered_map>

namespace {
  int ifd; // Inotify File Descriptor
  std::list<std::string> paths_to_watch;
  std::unordered_map<int, std::string> watched_paths;
  void run_command(const char* path) {
    system(path);
  }
  void wait_for_events() {
    static char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
    static bool initial = true;
    fcntl(ifd, F_SETFL, 0);
    while(true) {
      bool watched_at_least_one_new_file = false;
      if(!paths_to_watch.empty()) {
        auto it = paths_to_watch.begin();
        while(it != paths_to_watch.end()) {
          auto dis = it++;
          int wd = inotify_add_watch(ifd, dis->c_str(),
                                     IN_CLOSE_WRITE|IN_IGNORED|IN_MOVE_SELF);
          if(wd < 0) {
            fprintf(stderr, "%s: %s\n",
                    dis->c_str(), strerror(errno));
          }
          else {
            if(!initial) {
              fprintf(stderr, "%s: (now being watched)\n",
                      dis->c_str());
            }
            watched_paths[wd] = *dis;
            paths_to_watch.erase(dis);
            if(!watched_at_least_one_new_file) {
              watched_at_least_one_new_file = true;
              /* enter non-blocking mode, so the loop becomes a poll */
              fcntl(ifd, F_SETFL, O_NONBLOCK);
            }
          }
        }
        initial = false;
      }
      bool ready_to_read;
      if(!paths_to_watch.empty() && !watched_at_least_one_new_file) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(ifd, &fds);
        struct timeval timeout;
        do {
          // try calling select over and over again if necessary, until we end
          // up with a non-error return
          timeout = {2, 0};
        } while(select(ifd+1, &fds, nullptr, nullptr, &timeout) < 0);
        ready_to_read = FD_ISSET(ifd, &fds);
      }
      else ready_to_read = true;
      if(ready_to_read) {
        /* wait for at least one event */
        if(read(ifd, buf, sizeof(buf)) <= 0) {
          switch(errno) {
#if EAGAIN != EWOULDBLOCK
          case EWOULDBLOCK:
#endif
          case EAGAIN:
            return; // this is where we exit the loop
          case EINTR: continue;
          default:
            perror("read(ifd)");
            exit(1);
          }
        }
        else {
          auto p = (struct inotify_event*)buf;
          if(p->mask & IN_CLOSE_WRITE) {
            if(p->len && (p->name[0] == '#' || p->name[p->len-1] == '~'))
              continue;
            /* enter non-blocking mode, so the loop becomes a poll */
            fcntl(ifd, F_SETFL, O_NONBLOCK);
            continue;
          }
          else if(p->mask & IN_MOVE_SELF) {
            /* this will trigger IN_IGNORED, so we will re-watch whatever
               appears in its place */
            inotify_rm_watch(ifd, p->wd);
          }
          else if(p->mask & IN_IGNORED) {
            auto it = watched_paths.find(p->wd);
            if(it == watched_paths.end())
              fprintf(stderr, "Warning: Got IN_IGNORED for unknown watch descriptor %i\n", p->wd);
            else {
              auto path = it->second;
              fprintf(stderr, "%s was deleted or moved\n", path.c_str());
              paths_to_watch.emplace_back(path);
              watched_paths.erase(it);
              /* enter non-blocking mode, so the loop becomes a poll */
              fcntl(ifd, F_SETFL, O_NONBLOCK);
            }
          }
        }
      }
    }
  }
}

int main(int argc, char* argv[]) {
  if(argc <= 2) {
    fprintf(stderr, "Usage: %s command dir1 [dir2 ...]\n", argv[0]);
    return 1;
  }
  ifd = inotify_init();
  if(ifd < 0) { perror("inotify_init"); return 1; }
  for(int i = 2; i < argc; ++i) {
    paths_to_watch.emplace_back(argv[i]);
  }
  while(1) {
    wait_for_events();
    run_command(argv[1]);
  }
  return 0;
}
