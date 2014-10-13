I have implemented a multithreaded HTTP web server . It works in the following manner

1)This web server receives HTTP request from clients for files(which can be documents , imgaes or videos). 
2)One of the thread receives the request and queues it.
3)One another  thread schedules these request on the basis of the scheduling policy(Two scheduling poliies have been supported i.e FCFS and SJF).
4)SJF policy depends on the size of the file . So the file with shortest size will be served first.
5)Once the requests have been scheduled ,the worker threads in the thread pool start serving the request in the new sorted order.
6)This thread pool is created when the web server is started . The number of threads in the thread pool is a paramter that is taken while starting the web server.
7)The HTTP server could be run in Daemon as well as Debug Mode.

The project gave a good understanding about operating system scheduling algorithms , deadlock prevention concepts , creting thread pool , POSIX thread concepts .



