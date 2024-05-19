# Code Map for Linux 6.2

Commit id: c9c3395d5e3dcc6daee66c6908354d47bf98cb0c .

arch/x86/include/asm/page.h
- `__pa`
- `virt_to_page`

`arch/x86/include/asm/page_types.h`
- `PAGE_MASK` # offset bits all 0; other bits all 1

include/linux/device.h
- `struct device`

include/linux/dma-direct.h
- `struct bus_dma_region`
- `translate_dma_to_phys`
- `translate_phys_to_dma`

include/linux/dma-map-ops.h
- `get_dma_ops`

include/linux/dma-mapping.h
- `dma_map_single`
- `dma_map_single_attrs`
- `dma_unmap_single`
- `dma_unmap_single_attrs`

include/linux/etherdevice.h
- `is_broadcast_ether_addr`
- `is_multicast_ether_addr`
- `is_unicast_ether_addr`
- `is_zero_ether_addr`

include/linux/ieee80211.h
- `struct ieee80211_hdr`

include/linux/list.h
- `INIT_LIST_HEAD`
- `__list_add`
- `list_add`
- `list_add_tail`
- `__list_del`
- `list_del`
- `__list_del_entry`
- `list_del_init`

include/linux/mm.h
- `offset_in_page`

`include/linux/mm_types.h`
- struct page

include/linux/netdevice.h
- `dev_kfree_skb_any`

include/linux/pci.h
- `struct pci_dev`

include/linux/printk.h
- printk

include/linux/skbuff.h
- `dev_alloc_skb`
- `netdev_alloc_skb`
- `struct sk_buff`
- `skb_put_data`
- `skb_reserve`
- `skb_tail_pointer`

include/linux/types.h
- `dma_addr_t`
- `struct list_head`

include/net/mac80211.h
- `struct ieee80211_hw`
- `struct ieee80211_ops`
- `struct ieee80211_rx_status`

`include/uapi/linux/if_ether.h`
- `ETH_ALEN`

kernel/dma/direct.h
- `dma_direct_map_page`

kernel/dma/mapping.c
- `dma_map_page_attrs`
- `dma_unmap_page_attrs`

kernel/panic.c
- panic

kernel/printk/printk.c
- `_printk`

lib/vsprintf.c
- `ptr_to_id` # hash a pointer

net/core/skbuff.c
- `__netdev_alloc_skb`
- `skb_put`

net/mac80211/rx.c
- `ieee80211_rx_irqsafe`
