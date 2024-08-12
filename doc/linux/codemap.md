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
- `dma_capable`
- `translate_dma_to_phys`
- `translate_phys_to_dma`

include/linux/dma-map-ops.h
- `get_dma_ops`

include/linux/dma-mapping.h
- `dma_alloc_coherent`
- `dma_free_coherent`
- `dma_map_single`
- `dma_map_single_attrs`
- `dma_unmap_single`
- `dma_unmap_single_attrs`

include/linux/etherdevice.h
- `is_broadcast_ether_addr`
- `is_multicast_ether_addr`
- `is_unicast_ether_addr`
- `is_zero_ether_addr`

include/linux/export.h
- `EXPORT_SYMBOL`
- `EXPORT_SYMBOL_GPL`

include/linux/ieee80211.h
- `struct ieee80211_hdr`

include/linux/net.h
- `struct socket` # general BSD socket. Contains `struct sock *sk;`

include/linux/netfilter.h
- `NF_HOOK`

include/linux/swiotlb.h
- `struct io_tlb_mem`
- `is_swiotlb_buffer`

include/linux/list.h
- `INIT_LIST_HEAD`
- `__list_add`
- `list_add`
- `list_add_tail`
- `__list_del`
- `list_del`
- `__list_del_entry`
- `list_del_init`

include/linux/minmax.h
- `min_not_zero`

include/linux/mm.h
- `offset_in_page`

`include/linux/mm_types.h`
- struct page

include/linux/netdevice.h
- `dev_kfree_skb_any`
- `dev_queue_xmit`

include/linux/pci.h
- `struct pci_dev`

include/linux/pfn.h
- `PFH_PHYS`
- `PHYS_PFN`

include/linux/printk.h
- printk

include/linux/skbuff.h
- `dev_alloc_skb`
- `netdev_alloc_skb`
- `struct sk_buff`
- `struct sk_buff_head`
- `__skb_dequeue`
- `__skb_insert`
- `skb_peek`
- `skb_put_data`
- `skb_reserve`
- `skb_tail_pointer`
- `__skb_queue_before`
- `__skb_queue_tail`
- `__skb_unlink`

include/linux/types.h
- `dma_addr_t`
- `struct list_head`

include/linux/workqueue.h
- `create_workqueue`
- `struct work_struct`
- `queue_work`

include/net/arp.h

include/net/mac80211.h
- `struct ieee80211_hw`
- `struct ieee80211_ops`
- `struct ieee80211_rx_status`

include/net/sock.h
- `struct sock`

`include/uapi/linux/if_arp.h`
- struct arphdr

`include/uapi/linux/if_ether.h`
- `ETH_ALEN`

kernel/dma/direct.h
- `dma_direct_map_page`
- `dma_direct_unmap_page`

kernel/dma/swiotlb.c
- `swiotlb_bounce`
- `swiotlb_map`
- `swiotlb_tbl_map_single`
- `swiotlb_tbl_unmap_single`

kernel/dma/mapping.c
- `dma_alloc_attrs`
- `dma_free_attrs`
- `dma_map_page_attrs`
- `dma_unmap_page_attrs`

kernel/panic.c
- panic

kernel/printk/printk.c
- `_printk`

kernel/workqueue.c
- `alloc_workqueue`
- `create_worker`
- `process_one_work`
- `__queue_work`
- `queue_work_on`
- `struct worker_pool`
- `struct workqueue_struct`
- `worker_thread`

lib/vsprintf.c
- `ptr_to_id` # hash a pointer

net/core/dev.c
- `dev_qdisc_enqueue`
- `__dev_queue_xmit`
- `__dev_xmit_skb`
- `netdev_core_pick_tx`
- `netdev_pick_tx`

net/core/skbuff.c
- `__netdev_alloc_skb`
- `skb_dequeue`
- `skb_put`

net/core/sock.c
- `sk_alloc`

`net/ipv4/af_inet.c`
- `inet_create` # create `struct sock` for the passed in `struct socket`. Also setup `sock->ops`.
- `inet_family_ops`
- `inet_init` # call `sock_register(&inet_family_ops)`
- `__inet_stream_connect` # calls `sock->sk->sk_prot->connect`
- `inet_stream_connect`
- `inet_stream_ops`
- `inetsw_array`

net/ipv4/arp.c
- `arp_create`
- `arp_send`
- `arp_send_dst`
- `arp_xmit`
- `arp_xmit_finish`

`net/ipv4/tcp_ipv4.c`
- `struct proto tcp_prot`
- `tcp_v4_connect`

net/mac80211/cfg.c
- `ieee80211_auth` # static

`net/mac80211/driver-ops.h`
- `drv_tx` # calls `local->ops->tx`

`net/mac80211/ieee80211_i.h`
- `ieee80211_tx_skb` # call `ieee80211_tx_skb_tid`
- `ieee80211_tx_skb_tid_band`

net/mac80211/main.c
- `ieee80211_alloc_hw_nm` # register `ieee80211_scan_work`

net/mac80211/mlme.c
- `ieee80211_auth` # calls `ieee80211_send_auth`
- `ieee80211_mgd_auth` # calls `ieee80211_auth` in the same file

net/mac80211/rx.c
- `ieee80211_rx_irqsafe`

net/mac80211/scan.c
- `ieee80211_scan_state_send_probe`
- `ieee80211_scan_work` # `ieee80211_queue_delayed_work` is called. It seems to reschedule the work item
- `ieee80211_send_scan_probe_req` # call `ieee80211_build_probe_req` & `ieee80211_tx_skb_tid_band`

net/mac80211/tx.c
- `ieee80211_queue_skb`
- `__ieee80211_tx` # call `ieee80211_tx_frags`
- `ieee80211_tx` # calls `ieee80211_queue_skb` & `__ieee80211_tx`
- `ieee80211_tx_frags` # call `drv_tx`
- `__ieee80211_tx_skb_tid_band`
- `ieee80211_tx_skb_tid`
- `ieee80211_xmit`

net/mac80211/util.c
- `ieee80211_build_probe_req`
- `ieee80211_send_auth` # call `ieee80211_tx_skb`

net/socket.c
- `sock_alloc` # `inode->i_ops` is setup to `&sockfs_inode_ops`
- `__sock_create`
- `sock_create`
- `sock_register`
- `sock_sendmsg` # calls `sock_sendmsg_nosec`
- `sock_sendmsg_nosec` # most likely calls `sock->ops->sendmsg`
- `__sys_connect`
- `__sys_connect_file` # calls `sock->ops->connect`
- `____sys_sendmsg` # calls `sock_sendmsg`
- `___sys_sendmsg` # copy msg from user mode to kernel mode and calls `____sys_sendmsg`
- `__sys_sendmsg` # calls `___sys_sendmsg`
- `__sys_socket`
- `__sys_socket_create`
- `SYSCALL_DEFINE3(connect, ...)` # defines `sys_connect`
- `SYSCALL_DEFINE3(sendmsg, ...)` # defines `sys_sendmsg`
- `SYSCALL_DEFINE3(socket, ...)` # defines `sys_socket`

net/wireless/mlme.c
- `cfg80211_mlme_auth` # calls `rdev_auth`

net/wireless/nl80211.c
- `nl80211_authenticate` # calls `cfg80211_mlme_auth`

`net/wireless/rdev-ops.h`
- `rdev_auth`
