
## communiction in local node for production
1. IPC via shared memory
keys: ComQueueId200; ComQueueId201;...
by: shm_open/mmap get peer process's queue(structure msg buffer, sempore)
2. send/receive sequence:
msg push->sem post
sem wait->msg pop
