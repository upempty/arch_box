## linux kernel networking
1. layer 5 Session layer(sockets/files)  
write / sendto / sendmsg
2. layer 4 Transport layer(TCP)  
tcp_sendmsg  
*TCP performs timeout and retransmission*  

3. layer 3 Network layer  
ip_queue_xmit/ip_output  
4. layer 2 Link layer  
dev_queue_xmit/netif_schedule  
5. layer 1 Hardware layer  
