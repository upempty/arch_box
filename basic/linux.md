1. 自旋锁  
自旋锁必须是忙等模式。假如它是睡眠模式，那么就会允许中断触发，就会被别的进程中断抢占操作。  
2. socket flow: int socket(int domain, int type, int protocol)  
```
a) registered after system startup:   
module_init for inet_init  

b) 
libc socket()-->socket.S(系统调用号socketcall->eax--执行系统调用ENTER_KERNEL)
-->内核收到系统调用的命令后从eax寄存器中取出系统调用号到table里查找执行
->
syscall_handler_t *sys_call_table[] -> 
[ __NR_socketcall ] = sys_socketcall ->
sys_socketcall->sys_connect--sock = sockfd_lookup(fd, &err)--->sock->ops->connect(inet_stream_connect)->  
->


```
