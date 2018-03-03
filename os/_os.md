## sleep & wakeup
linux 0.11  
###  sleep_on wake_up  
```  
sleep_on(&wait);
wait --> pointing to current sleeping process;
tmp  --> pointing to previous sleeped process;

wake_up(&wait); 
if sleep order A->B->C, then C->B->A:
woke process order: 
(wait's pointing)C-->
(tmp point to previous)B-->
(tmp point to previous)A-->
(tmp point to previous)NULL
```  
#### sleep_on  
```c  
    void sleep_on(struct task_struct **p)  
    {  
        struct task_struct *tmp;  
      
        if (!p)  
            return;  
        if (current == &(init_task.task))  
            panic("task[0] trying to sleep");  
        tmp = *p;  
        *p = current;  
        current->state = TASK_UNINTERRUPTIBLE;  
        schedule();  
        if (tmp)  
            tmp->state=0;  
    } 
```  
#### wake_up  
```c  
    void wake_up(struct task_struct **p)  
    {  
        if (p && *p) {  
            (**p).state=0;  
            *p=NULL;  
        }  
    }
```  

## schedule  

进程调度--每个进程都会有schedule()
When the scheduler is invoked to run a new process

 By assigning timeslices, the scheduler makes global decisions for the running system, 
and prevents individual processes from dominating over the processor resources.

fork-copy_process，READY state in task queue， 然后等待调度。
调度让新加入队列就绪的pcb进程运行跑起来。（最多有64个任务同时存在）
 invokes the schedule function to switch to the other process. 


schedule to new process(task)

schedule--> cpu scan -->TR---TSS
in schedule()：
switch_to (next);		// 切换到任务号为next的任务，并运行之
(
	__tmp = (unsigned short)_TSS(n);---

	_asm {
		mov ebx, offset task
		mov eax, n
		mov ecx, [ebx+eax*4]
		cmp ecx, current/是当前任务吗， 是就什么也不做 */ 
		je l1 
		xchg ecx,current/* current = task[n] */
		 
		mov ax, __tmp
)


拷贝process的时候
会执行：
set_tss_desc
	set_tss_desc (gdt + (nr << 1) + FIRST_TSS_ENTRY, &(p->tss));
	set_ldt_desc (gdt + (nr << 1) + FIRST_LDT_ENTRY, &(p->ldt));


p->state = TASK_RUNNING;---设置就绪状态/* do this last, just in case */


然后schedule来到执行改task及TSS

这样就调度到另一个进程执行了。
copy_process()'s task_struct.



每个任务（进程或线程）都对应一个独立的TSS，TSS就是内存中的一个结构体，里面包含了几乎所有的CPU寄存器的映像。
有一个任务寄存器（Task Register，简称TR）指向当前进程对应的TSS结构体，
所谓的TSS切换就将CPU中几乎所有的寄存器都复制到TR指向的那个TSS结构体中保存起来，同时找到一个目标TSS，
即要切换到的下一个进程对应的TSS，将其中存放的寄存器映像“扣在”CPU上，就完成了执行现场的切换。


	
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!do_timer in timer interrupt. register resided do_timer address.
https://en.wikipedia.org/wiki/Linux_kernel
man clone

一个tick来重现算了时间片，
时间片---


每个cpu 140个队列（优先级的100个，common的40个）
给他10s，用了9秒，就剩下的权重少了

根据使用量来设置权重。
counter大，表明运行时间不长


align 4
_timer_interrupt:-------时钟中断 int 0x20

do_timer里
	if ((--current->counter) > 0)
		return;		给定的运行时间还没完，就退出，仍然在当前进程
	否则重新调度，内部选择大的counter的来运行
	schedule()

对于一个进程如果时间片用完，则进行任务切换
do_timer--10毫秒

调度时钟 int 0x20，100HZ

设置始终中断---set_intr_gate (0x20, &timer_interrupt);=========idt设置中断门寄存器，绑定到timer函数。
lea edi,_idt

head.s->call setup_idt	设置中断描述表-寄存器
```  
setup_idt:
	lea edx,ignore_int		 
	mov eax,00080000h		 
	mov ax,dx				 

	mov dx,8E00h 
			 
	lea edi,_idt
	mov ecx,256
	
```  

## stack for execution  
```  
Special region for memory that stores temporary variables created by each function(including main)  
stack is for each function, push variable, and modify any value by ebp pointer +- offset(e.g:-0x18(%rbp);  
and finally pop them after exiting the function.   
objdump -d a.out to check code executing virtual address  		
```  
## Thread Pool type 1  
maxinum, free list/pending list/active list queue for management   
null->pending->create thread->active-->   
->nextpending-........................>no task in pending-->free list  

opencount > maxinum, then exit. wait other busy thread to exit then create it.  

## xxx
1. 自旋锁  
自旋锁必须是忙等模式。假如它是睡眠模式，那么就会允许中断触发，就会被别的进程中断抢占操作。  

2. 序列化  
进程间消息发送，如果是约定某个结构的消息，带有指针变量，这样一般的拷贝只会传递指针地址到对端。  
如果不进行序列化，发送后就会有指向数据丢失，无法被完整的接收。  
序列化(这里用protobuf方法)可以解决这样的问题:  
```
struct PlayerProfile_Struct
{
/*00000*/ uint32 checksum;
/*00004*/ uint32 gender;
/*00008*/ char *name;
};


// -------------------------------------------------
// PlayerProfile_Struct.proto
message PlayerProfile_Struct{
  required uint32 checksum = 1;
  required uint32 gender = 2;
  optional string name = 3;
}
 
// -------------------------------------------------
// Create an instance and serialize it to a file.
PlayerProfile_Struct player;
player.set_checksum(50193);
player.set_name("Alex&Anni");
player.set_gender(2);
 
fstream out("alex_anni.pb", ios::out | ios::binary | ios::trunc);
player.SerializeToOstream(&out);
out.close();
 
// -------------------------------------------------
// Create an instance and deserialize it from a file.
PlayerProfile_Struct player;
fstream in("alex_anni.pb", ios::in | ios::binary);
if (!player.ParseFromIstream(&in)) {
    cerr << "Failed to parse alex_anni.pb." << endl;
    exit(1);
}
 
if (player.has_name()) {
    cout << "e-mail: " << player.name() << endl;
}

```

3. 硬中断和软中断(上半部和下半部)  

4. socket flow:   
```
a) registered after system startup:   
module_init for inet_init  

b) int socket(int domain, int type, int protocol)
libc socket()-->socket.S(系统调用号socketcall->eax--执行系统调用ENTER_KERNEL)
-->内核收到系统调用的命令后从eax寄存器中取出系统调用号到table里查找执行
->
syscall_handler_t *sys_call_table[] -> 
[ __NR_socketcall ] = sys_socketcall ->
->sys_socketcall->  
->sys_socket->sock_create-> inet_create-->  
  
  
==then operation for socket is using the searched specified proto's operation  
->sys_connect--sock = sockfd_lookup(fd, &err)--->sock->ops->connect(inet_stream_ops's inet_stream_connect)->  
->sk->sk_prot->connect(tcp_prot's tcp_v4_connect)  --> tcp_connect ->  queuetail tcp_transmit_skb-->  
->queue_xmit(ipv4_specific's ip_queue_xmit)-->NF_HOOK(PF_INET, NF_IP_LOCAL_OUT, skb, NULL, rt->u.dst.dev, dst_output)  
->skb->dst->output(skb)(ip_output)-->
ip_fragment(break smaller piece) --> __skb_push output(skb2)(output sending queue)  
or 
ip_finish_output->

(
static struct inet_protosw inetsw_array[] =
{
        {
                .type =       SOCK_STREAM,
                .protocol =   IPPROTO_TCP,
                .prot =       &tcp_prot,-----------
                .ops =        &inet_stream_ops,----
                .capability = -1,
                .no_check =   0,
                .flags =      INET_PROTOSW_PERMANENT,
        },
)

```

5. setitimer  
sys_setitimer  ? 
