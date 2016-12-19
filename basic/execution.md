### stack for execution  
```  
Special region for memory that stores temporary variables created by each function(including main)  
stack is for each function, push variable, and modify any value by ebp pointer +- offset(e.g:-0x18(%rbp);  
and finally pop them after exiting the function.   
objdump -d a.out to check code executing virtual address  
```  
