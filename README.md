This is my toy project TinyFile. It is a small client-service application I wrote to learn about some OS concepts like IPC and multithreading/processing. It consists of a daemon service, a client library, and a sample application. IMPORTANT: the daemon is set to run indefinitely until it is killed. If you do not kill the daemon properly, it may not appropriately free its message queues or allocated shared memory. I have implemented a proper clean up function that handles the SIGTERM signal. So, to properly terminate the daemon once you have completed your workflow, run: 
```shell
pkill -15 TinyFile
```
 Alternatively, you can run ps -e, find the PID of the daemon, and run 
 ```shell
kill -15 <PID>
```
. I will now explain the 3 executables included in my project and how to 

The first major component is the daemon. To compile the executable, cd into the src directory and type "make" to create the runnable executable. The daemon can be created via the command line with the following syntax: 
```shell
./TinyFile --n_sms <NUM_SHARED_MEMORY_SEGMENTS> --sms_size <SHARED_MEMORY_SEGMENT_SIZE>
```

At this point, the daemon will be running!

Interacting with the daemon requires clients. To compile the executable responsible for creating a client, cd into src and type "make client" A client can then be created via the following:
```shell
./TinyClient -n <TINY_CLIENT_NAME> --files <PATH_TO_FILES.txt> --num <NUMBER_OF_INDIVIDUAL_FILES>
```

There are a couple of things to take note of when initializing a client. First, the -n argument must begin with a backslash as it is used to create the client's message queues. Second, the --files argument should point to a text file. This text file should contain a line containing a path to a file you wish to compress, followed by the mode of communication (synchronous or asynchronous), followed by a file, etc. Finally, the --num argument should contain the total number of files expressed in --files. This should be equal to the number of lines in the file divided by 2. I have provided 3 such text files along with corresponding data to compress in the directory input. input_sync.txt contains 5 files all compressed via synchronous (blocking) transfer. input_async.txt contains 5 files all compressed via asynchronous (non-blocking) transfer. Finally, input_mixed.txt contains 20 files with a mix of blocking and non-blocking modes of transfer.

Finally, there is one additional minor portion: a small testing executable. To compile it, cd into src and type "make tester" A test executed via the following: 
```shell
./TinyTest -n <NUM_CLIENTS>
```
When executed, the process spawns -n child processes, all of which call the ./TinyClient executable. Each of these processes compresses the files contained in input_mixed.txt. I tested this with n = 100, representing 2000 file compressions, some blocking and some non-blocking, across 100 different processes all interacting with the same system. This represented a stress test of my system and it performed well (although QoS was not a point of focus for this project).
