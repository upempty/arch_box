

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

setup_idt:
	lea edx,ignore_int		 
	mov eax,00080000h		 
	mov ax,dx				 

	mov dx,8E00h 
			 
	lea edi,_idt
	mov ecx,256
	
		
