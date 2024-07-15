#include "event.h"
#include "device.h"

using namespace Device;

void Event::init_eq(MEM::Queue<EQE>& eq) {
    /* set ownership bit to 1 */
    for (l4_size_t i = 0; i < eq.size - 1; i++) {
        iowrite32be(&eq.start[i].ctrl, 0x1);
    }
}