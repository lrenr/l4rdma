#include <stdio.h>
#include "interface.h"
#include "device.h"

Registry main_srv;
L4RDMA_Server l4rdma_server;
 
int main(int argc, char **argv) {
	
	if (!main_srv.registry()->register_obj(&l4rdma_server, "l4rdma_server").is_valid()) {
		printf("Could not register service\n");
		return 1;
	}

	printf("Welcome to the l4rdma server\n");

	L4::Cap<L4vbus::Vbus> vbus = L4Re::Env::env()->get_cap<L4vbus::Vbus>("vbus_mlx");
	if (!vbus.is_valid()) throw;
	L4vbus::Pci_dev dev = Device::pci_get_dev(vbus);
	l4_uint8_t *bar0 = Device::map_pci_bar(dev, 0);
	printf("bar0:%p\n", bar0);


	main_srv.loop();

	return 0;
}
