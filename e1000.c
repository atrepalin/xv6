#include "types.h"
#include "e1000.h"
#include "defs.h"
#include "mmu.h"
#include "memlayout.h"

static volatile uint32_t *e1000; 

struct e1000_tx_desc tx_desc_buf[TXRING_LEN] __attribute__ ((aligned (PGSIZE)));
struct e1000_data tx_data_buf[TXRING_LEN] __attribute__ ((aligned (PGSIZE)));
struct e1000_rx_desc rx_desc_buf[RXRING_LEN] __attribute__ ((aligned (PGSIZE)));
struct e1000_data rx_data_buf[RXRING_LEN] __attribute__ ((aligned (PGSIZE)));

static void init_desc(void) {
    int i;
    for (i = 0; i < TXRING_LEN; ++i) {
        tx_desc_buf[i].buffer_addr = (uint64_t)V2P(&tx_data_buf[i]); 
        tx_desc_buf[i].lower.flags.length = 0;
        tx_desc_buf[i].lower.flags.cso = 0;
        tx_desc_buf[i].lower.flags.cmd = 0;
        tx_desc_buf[i].upper.fields.status = E1000_TXD_STAT_DD;
        tx_desc_buf[i].upper.fields.css = 0;
        tx_desc_buf[i].upper.fields.special = 0;
    }

    for (i = 0; i < RXRING_LEN; ++i) {
        rx_desc_buf[i].buffer_addr = (uint64_t)V2P(&rx_data_buf[i]);
        rx_desc_buf[i].length = 0;
        rx_desc_buf[i].csum = 0;
        rx_desc_buf[i].status = 0;
        rx_desc_buf[i].errors = 0;
        rx_desc_buf[i].special = 0;
    }
}

static void
e1000_init(){
	e1000[E1000_TDBAL] = (uint32_t)V2P(tx_desc_buf);
	e1000[E1000_TDBAH] = 0x0;
	e1000[E1000_TDH] = 0x0;
	e1000[E1000_TDT] = 0x0;
	e1000[E1000_TDLEN] = TXRING_LEN * sizeof(struct e1000_tx_desc);
	e1000[E1000_TCTL] = VALUEATMASK(1, E1000_TCTL_EN) |
						VALUEATMASK(1, E1000_TCTL_PSP) |
						VALUEATMASK(0x10, E1000_TCTL_CT) |
						VALUEATMASK(0x40, E1000_TCTL_COLD);
	e1000[E1000_TIPG] = VALUEATMASK(10, E1000_TIPG_IPGT) |
						VALUEATMASK(8, E1000_TIPG_IPGR1) |
						VALUEATMASK(6, E1000_TIPG_IPGR2);
	e1000[E1000_RAL] = 0x12005452;
	e1000[E1000_RAH] = 0x00005634 | E1000_RAH_AV;

	e1000[E1000_RDBAL] = (uint32_t)V2P(rx_desc_buf);
	e1000[E1000_RDBAH] = 0x0;
	e1000[E1000_RDLEN] = RXRING_LEN * sizeof(struct e1000_rx_desc);
	e1000[E1000_RDH] = 0x0;
	e1000[E1000_RDT] = RXRING_LEN - 1;
	e1000[E1000_RCTL] = E1000_RCTL_SBP        | 
                        E1000_RCTL_UPE        | 
                        E1000_RCTL_MPE        | 
                        E1000_RCTL_RDMTS_HALF | 
                        E1000_RCTL_SECRC      | 
                        E1000_RCTL_LPE        | 
                        E1000_RCTL_BAM        | 
                        E1000_RCTL_EN         |
                        E1000_RCTL_SZ_2048;
}

int
e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);
	init_desc();	
	e1000 = (void*)pcif->reg_base[0];
	cprintf("e1000: bar0  %x size0 %x\n", pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("e1000: status %x\n",((uint*)e1000)[STATUS/4]);
	e1000_init();
	return 0;
}

int
e1000_xmit(void * addr, size_t length){
	uint32_t tail = e1000[E1000_TDT];

	struct e1000_tx_desc * tail_desc = &tx_desc_buf[tail];

	if (tail_desc->upper.fields.status != E1000_TXD_STAT_DD)
	{
		return -1;
	}

	length = length > DATA_SIZE ? DATA_SIZE : length;
	memmove(&tx_data_buf[tail], addr, length);
	tail_desc->lower.flags.length = length;
	tail_desc->upper.fields.status = 0;
	tail_desc->lower.data |=  (E1000_TXD_CMD_RS |
							   E1000_TXD_CMD_EOP);

	e1000[E1000_TDT] = (tail + 1) % TXRING_LEN;
	return 0;
}

void
e1000_transmit(const char *buf, unsigned int len)
{
	e1000_xmit((void*)buf, len);
}

int
e1000_recv(uint8_t *data) {
    uint32_t next = (e1000[E1000_RDT] + 1) % RXRING_LEN;
    struct e1000_rx_desc *desc = &rx_desc_buf[next];

    if (!(desc->status & E1000_RXD_STAT_DD))
        return -1;

    size_t length = desc->length;
    memmove(data, &rx_data_buf[next], length);

    desc->status = 0;

    e1000[E1000_RDT] = next;

    return length;
}

int
e1000_receive(char *buf, unsigned int len)
{
	return e1000_recv((uint8_t*)buf);
}

void
e1000_get_mac(uint8_t *mac) {
	uint32_t ral = e1000[E1000_RAL];
	uint32_t rah = e1000[E1000_RAH];

	mac[0] = ral & 0xFF;
	mac[1] = (ral >> 8) & 0xFF;
	mac[2] = (ral >> 16) & 0xFF;
	mac[3] = (ral >> 24) & 0xFF;

	mac[4] = rah & 0xFF;
	mac[5] = (rah >> 8) & 0xFF;
}