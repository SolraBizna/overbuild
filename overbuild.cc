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

static int ifd; // Inotify File Descriptor

static void run_command(const char* path) {
  system(path);
}

static void wait_for_events() {
  static char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
  fcntl(ifd, F_SETFL, 0);
 again:
  if(read(ifd, buf, sizeof(buf)) <= 0) {
    switch(errno) {
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
    case EAGAIN:
      return;
    case EINTR: goto again;
    default:
      perror("read(ifd)");
      exit(1);
    }
  }
  else {
    auto p = (struct inotify_event*)buf;
    if(p->len && (p->name[0] == '#' || p->name[p->len-1] == '~')) goto again;
    fcntl(ifd, F_SETFL, O_NONBLOCK);
    goto again;
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
    if(inotify_add_watch(ifd, argv[i], IN_CLOSE_WRITE) < 0) {
      perror(argv[i]);
      return 1;
    }
  }
  while(1) {
    run_command(argv[1]);
    wait_for_events();
  }
  return 0;
}
