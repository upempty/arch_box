
## volatile  
volatile int reg;  
reg = *CACHE_REG;  
Whenever compiler encounter any reference of volatile variable is always load the value of variable from memory  
so that if any external source has modified the value in the memory complier will get its updated value.  
