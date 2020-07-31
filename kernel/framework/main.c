#include <kernel.h>
#include <klib.h>

int main() {
    printf("cpu_reset\n");
    _ioe_init();
    _cte_init(os->trap);
    _vme_init(pmm->alloc, pmm->free);
    os->init();
    _mpe_init(os->run);
    return 1;
}
