
## libevent source code analysis
### event_base_loop-> evsel->dispatch -> event_process_active -> event_process_active_single_queue
1. *event_process_active_single_queue*: fetch from active list to process the event  
