#pragma once

#include <kernel/usb/xhci_trb.h>

// All TRB (except LinkTRB) on the list contains all 0 initially. This is archieved by the
// TRBCommon ctor.
class TRBRing {
 public:
  explicit TRBRing(bool use_link_trb) : use_link_trb_(use_link_trb) {
    assert((get_addr() & 0xFFF) == 0); // page alignment
    assert(sizeof(trb_ring_) == 4096);

    if (use_link_trb) {
      trb_ring_[trb_capacity() - 1] = LinkTRB(get_addr()).toTemplate();
    }
  }

  // total number of allocated TRBs belong to the ring.
  uint32_t trb_capacity() const {
    return sizeof(trb_ring_) / sizeof(*trb_ring_);
  }

  uint32_t get_addr() const {
    return (uint32_t) &trb_ring_;
  }
 protected:
  // 256 items
  TRBTemplate trb_ring_[4096 / sizeof(TRBTemplate)] __attribute__((aligned(4096))); 
  bool use_link_trb_;
};

class ProducerTRBRing : public TRBRing {
 public:
  explicit ProducerTRBRing(bool producer_cycle_state)
    : TRBRing(true),
      producer_cycle_state_(producer_cycle_state),
      enqueue_ptr_(trb_ring_) {
  }

  /*
   * Return the address of the enqueued TRB. Note, it's not the address
   * of the input argument but the copy inside the queue.
   */
  TRBTemplate* enqueue(TRBTemplate trb) {
    TRBTemplate* ret = enqueue_ptr_;
    trb.c = producer_cycle_state_;

    // TODO: handle the case of
    // - link trb
    // - queue full
    printf("==== need revise ProducerTRBRing::enqueue ====\n");

    *enqueue_ptr_++ = trb;
    return ret;
  }
 private:
  TRBTemplate* enqueue_ptr_;
  bool producer_cycle_state_;
};

class ConsumerTRBRing : public TRBRing {
 public:
  explicit ConsumerTRBRing() :
    TRBRing(false),
    dequeue_ptr_(trb_ring_) { }

  TRBTemplate dequeue() {
    // TODO: this is a blocking call right now
    while (!hasItem()) {
      msleep(1);
    }
    assert(hasItem());
    TRBTemplate item = *dequeue_ptr_++;
    return item;
  }

  bool hasItem() const {
    // TODO: handle wrap around and segments
    return dequeue_ptr_->c == consumer_cycle_state_;
  }

  void skip_queued_trbs() {
    // TODO: handle wrap around and segments
    while (hasItem()) {
      TRBTemplate item = dequeue();
      printf("Skip event trb %d (%s)\n", item.trb_type, trb_type_str((TRBType) item.trb_type));
    }
  }
 private:
  TRBTemplate* dequeue_ptr_;
  // TODO: update this when wrap around happens
  bool consumer_cycle_state_ = 1;
};
