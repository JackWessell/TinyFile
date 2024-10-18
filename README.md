# TinyFile - A daemon and client library
Tinyfile is my project aimed at understanding a variety of systems programming topics, including but not limited to daemons, semaphores, and shared memory. The main executable (TinyFile.c) initializes a daemon process which can then be interacted with via the rest of the library. In this repository, clients can place files into shared memory managed by the daemon. The daemon will then retrieve these files, compress them, and send them back. The primary constraint is that all file transfers must go through shared memory - this necessitates utilizing additional functionality such as semaphores and message queues.

../bin/TinyClient -n "/TinyClient" --files ../input/input_async.txt --num 5
