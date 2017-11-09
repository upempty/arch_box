## linux 0.11  
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
