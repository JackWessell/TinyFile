# TinyFile - A daemon and client library
Tinyfile is my project aimed at understanding a variety of systems programming topics including but not limited to daemons, IPC, and shared memory. The main executable (TinyFile.c) initializes a daemon process which can then be interacted with via the rest of the library. Clients will request the daemon's services to have it compress files, however this functionality is modular and can easily be replaced with whatever suits your needs. This repository has three components: two major and one minor. 

The first major component is the daemon. To compile the executable, cd into the src directory and type "make" This creates the runnable executable. 
The daemon can be created via the command line with the following syntax: ./TinyFile --n_sms <NUM_SHARED_MEMORY_SEGMENTS> --sms_size <SHARED_MEMORY_SEGMENT_SIZE> 
At this point, the daemon will be running!

Interacting with the daemon requires clients. To compile the executable responsible for creating a client, cd into src and type "make client" 
A client can then be created via the following: ./TinyClient -n <TINY_CLIENT_NAME> --files <PATH_TO_FILES.txt> --num <NUMBER_OF_INDIVIDUAL_FILES>
There are a couple of things to take note of when initializing a client. First, the -n argument must begin with a backslash as it is used to create the client's message queues. Second, the --files argument should point to a text file. This text file should contain a line containing a path to a file you wish to compress, followed by the mode of communication (synchronous or asynchronous), followed by a file, etc. Finally, the --num argument should contain the total number of files expressed in --files. This should be equal to the number of lines in the file divided by 2. I have provided 3 such text files along with corresponding data to compress in the directory input. input_sync.txt contains 5 files all compressed via synchronous (blocking) transfer. input_async.txt contains 5 files all compressed via asynchronous (non-blocking) transfer. Finally, input_mixed.txt contains 20 files with a mix of blocking and non-blocking modes of transfer.

Finally, there is one additional minor portion: a small testing executable. To compile it, cd into src and type "make tester"
A test executed via the following: ./TinyTest -n <NUM_CLIENTS>
When executed, the process spawns -n child processes, all of which call the ./TinyClient executable. Each of these processes compresses the files contained in input_mixed.txt. I tested this with n = 100, representing 2000 file compressions, some blocking and some non-blocking, across 100 different processes all interacting with the same system. This represented a stress test of my system and it performed well (although I did not focus on QoS for this project).
