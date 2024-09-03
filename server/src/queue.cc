#include "queue.h"

void Q::alloc_block(Queue_Obj_Pool* qop){
    Queue_Obj_Block* qob = create_block(qop);

    for (l4_uint64_t i = 0; i < qop->block_size; i++) {
        Queue_Obj* qo = &qob->start[i];
        qo->used = false;
        qo->block = qob;
    }

    add_block_to_pool(qop, qob);
}

void Q::free_block(Queue_Obj_Pool* qop, Queue_Obj_Block* qob) {
    remove_block_from_pool(qop, qob);

    destroy_block(qob);
}