
# Buffer vs Queues
Buffer size was configured, but buffer is not acutual allocated;  
Queue is inited header.  
data transmit into kernel or data come from NIC, it then will alloc_skb for sk_buff structure and packet data area.  

## socket send buffer/recv buffer size
发送buffer和接收buffer的大小：好像是2倍，每个元素的结构是[sizeof(sk_buff)+data]的大小(headroom...data...tailroom).  
这个buffer的空间是不会在socket创建好后立即分配的。而是而后有数据包发送或接收后，根据来的数据跟sk_sndbuf/sk_rcvbuf的size比较的后来确定是否可以。 
```
e.g put into recv buffer queue
int sock_queue_rcv_skb(struct sock *sk, struct sk_buff *skb)
{
	int err;
	int skb_len;
	unsigned long flags;
	struct sk_buff_head *list = &sk->sk_receive_queue;
	...
	
struct sk_buff *skb = alloc_skb(size, priority);
		if (skb) {
			skb_set_owner_r(skb, sk);
        ...
```
有空间，则让发送，否则不给发送  
有空间，则给接收，否则不给接收  

## socket several tx rx queues  
queue is stored pointer or descriptor. e.g storing sk_buff ptr, which will pointer real packet data   

# packet transmit
layer 5 Session layer(sockets/files)
write / sendto / sendmsg

layer 4 Transport layer(TCP)
tcp_sendmsg
TCP performs timeout and retransmission
routers

layer 3 Network layer
ip_queue_xmit/ip_output
bridge

layer 2 Link layer
dev_queue_xmit/netif_schedule

layer 1 Hardware layer
