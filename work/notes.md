
## Activator:
0 main thread / listener thread /worker thread  
1 main thread do all work, and then goes to wait state  
2 listener thread if listener thread having excepiton, it will send mutex signal to main thread  

listener thread--> listen socket/accept=receive socket-->  
-->put msg to one task --> add task to thread pool(tasks threads)->  
-->signal-->  
thread in run--> receive condition-> execute()  
(RequestTask::execute() real action here, main execute after msg from ldap server)--->  
--> _eventReceiver->deleteRequest.....  
                    AddRequest.....  
                    
                    
