# ConnectX-4 based NIC driver for L4Re

Runs as an L4Re userspace server.
Executing commands on the firmware command interface, and NIC initialization are working.

More information in the overview: [here!](overview.pdf)

### modules.list entry to run
```
entry l4rdma
moe l4rdma.cfg
module l4re
module ned
module io
module l4rdma.io
module l4rdma
module l4rdma-test
```
