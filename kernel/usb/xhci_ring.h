#pragma once

#include <kernel/usb/xhci_trb.h>
#include <kernel/sleep.h>

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
    assert((uint32_t) &trb_ring_ == (uint32_t) this);
    return (uint32_t) &trb_ring_;
  }

  TRBTemplate* begin() {
    return trb_ring_;
  }

  TRBTemplate* end() {
    return (TRBTemplate*) trb_ring_ + sizeof(trb_ring_) / sizeof(*trb_ring_);
  }
 protected:
  // 256 items
  TRBTemplate trb_ring_[4096 / sizeof(TRBTemplate)] __attribute__((aligned(4096))); 
  bool use_link_trb_;
};

class ProducerTRBRing : public TRBRing {
 public:
  explicit ProducerTRBRing(bool producer_cycle_state = true)
    : TRBRing(true),
      producer_cycle_state_(producer_cycle_state),
      shadow_dequeue_ptr_(trb_ring_),
      enqueue_ptr_(trb_ring_) {
  }

  /*
   * Return the address of the enqueued TRB. Note, it's not the address
   * of the input argument but the copy inside the queue.
   */
  TRBTemplate* enqueue(TRBTemplate trb) {
    // consume potential link TRB and calculate next_trb
    // after the loop, we are safe to write the TRB into enqueue_ptr_
    TRBTemplate* next_trb = nullptr;
    while (true) {
      assert(enqueue_ptr_->c != producer_cycle_state_);
      next_trb = get_next_trb(enqueue_ptr_);
      // TODO: we need better handling for ring overflow
      assert(next_trb != shadow_dequeue_ptr_ && "producer ring overflow");

      // advance pertential link trb
      LinkTRB* link_trb = enqueue_ptr_->to_trb_type<LinkTRB>();
      if (link_trb) {
        link_trb->c = producer_cycle_state_;
        if (link_trb->tc) {
          producer_cycle_state_ = !producer_cycle_state_;
        }
        enqueue_ptr_ = next_trb;
      } else {
        break;
      }
    }

    TRBTemplate* ret = enqueue_ptr_;
    trb.c = producer_cycle_state_;
    *enqueue_ptr_ = trb;

    enqueue_ptr_ = next_trb;
    return ret;
  }

  // Only producer ring uses link trb. Wrap around for ConsumderRing/event ring
  // is easier to handle.
  static TRBTemplate* get_next_trb(TRBTemplate* cur) {
    if (cur->trb_type == TRBType::LINK) {
      return (TRBTemplate*) (cur->expect_trb_type<LinkTRB>().ring_segment_pointer_low);
    } else {
      return cur + 1;
    }
  }

  // TODO: in theory we could set shadow_dequeue_ptr_ to next(ptr_in_event_trb)?
  void update_shadow_dequeue_ptr(TRBTemplate *ptr_in_event_trb) {
    shadow_dequeue_ptr_ = ptr_in_event_trb;
  }

 private:
  // shadow the dequeue_ptr of the device. This may move slower than
  // the device's copy of dequeue_ptr. Update this by the TRB pointer
  // in event TRBs.
  TRBTemplate* shadow_dequeue_ptr_;
  TRBTemplate* enqueue_ptr_;
  bool producer_cycle_state_;
};

// update the host controller's copy of the event ring dequeue ptr.
// Assume a single event ring for now.
void update_hc_event_ring_dequeue_ptr(TRBTemplate *dequeue_ptr);

class ConsumerTRBRing : public TRBRing {
 public:
  explicit ConsumerTRBRing() :
    TRBRing(false),
    dequeue_ptr_(trb_ring_) { }

  TRBTemplate dequeue() {
    // TODO: this is a blocking call right now
    while (!hasItem()) {
      dumbsleep(1);
    }
    assert(hasItem());
    TRBTemplate item = *dequeue_ptr_++;

    if (dequeue_ptr_ == end()) {
      dequeue_ptr_ = trb_ring_;
      consumer_cycle_state_ = !consumer_cycle_state_;
    }

    update_hc_event_ring_dequeue_ptr(dequeue_ptr_);
    return item;
  }

  bool hasItem() const {
    return dequeue_ptr_->c == consumer_cycle_state_;
  }

  void skip_queued_trbs() {
    while (hasItem()) {
      TRBTemplate item = dequeue();
      printf("Skip event trb %d (%s)\n", item.trb_type, trb_type_str((TRBType) item.trb_type));
    }
  }
 private:
  TRBTemplate* dequeue_ptr_;
  bool consumer_cycle_state_ = 1;
};
