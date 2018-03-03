
发送buffer和接收buffer的大小：好像是2倍，但是包括sizeof(sk_buff)+data的大小(headroom...data...tailroom).
这个buffer的空间是不会socket创建好后立即分配的。而后有数据包后，根据来的数据来跟sk_sndbuf/sk_rcvbuf的size比较的后来确定是否可以。
有空间，则让发送，否则不给发送
有空间，则给接收，否则不给接收
