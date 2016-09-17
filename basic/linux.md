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
