I've been using this tool for years, and I see no reason not to release it now.

This is an incredibly, _stupidly_\* simple system for automatically compiling a project with every change that is made. It is a surprising timesaver, especially for large C++ projects. I spent less than an hour writing the initial version (mostly tuning `oversub.sh` and not overbuild itself), and had made up that time within a week.

It only runs on Linux, because it uses inotify.

Example usage:

    overbuild path/to/oversub.sh src include

This example will run `oversub.sh` every time any file in the `src` or `include` directory is modified. Any command or program can be used. If there are subdirectories of `src` or `include`, they will _not_ be monitored unless explicitly specified.

The `oversub.sh` included with this package runs `make -O -k -j2` (-O = group job outputs together, -k = continue building as far as possible after error, -j2 = run two jobs in parallel) and digests the output. `oversub_j1.sh` is the same, but does a serial build rather than a parallel one.

\* If you save a change, and then save a second change after that file is read during the build but before the output is saved, and the first build is successful, the second build may not detect the second change. I've been using overbuild for two years now and I've only had this happen twice. The situation can usually be recovered by simply saving the file again. Caveat user. I would fix this but the underlying circumstances are quite rare...
