# BZDG Batched Zerocopy DataGram

This is a UDP framework for the system that needs better performance than existing socket interface.

BZDG realize better performance than existing socket framework by following solution.

- Packet batching
- Zerocopy data transfer between user and kernel space
- Preallocation of sk_buff

Currently, Zerocopy and Preallocation method is not impremented.

But, in the 1GB data transfer benchmarks, BZDG realize about 40% performance improvement in the best case(512byte/packet and 80 batch).
