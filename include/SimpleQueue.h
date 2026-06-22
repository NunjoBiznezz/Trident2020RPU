#pragma once
#include <stdint.h>

// Generic FIFO circular queue. Capacity is SIZE-1 (one slot reserved to
// distinguish full from empty without a sentinel value).
// Protected members allow subclasses to iterate the live region.
template <typename T, uint8_t SIZE>
class SimpleQueue {
public:
   SimpleQueue() : head_(0), tail_(0) {}

   bool isEmpty() const { return head_ == tail_; }
   bool isFull() const { return ((tail_ + 1) % SIZE) == head_; }

   int spaceLeft() const {
      if (tail_ >= head_) return (SIZE - 1) - (tail_ - head_);
      return head_ - tail_ - 1;
   }

   T peek() const {
      if (isEmpty()) return T{};
      return buffer_[head_];
   }

   bool push(T value) {
      if (isFull()) return false;
      buffer_[tail_] = value;
      tail_ = (tail_ + 1) % SIZE;
      return true;
   }

   T pull() {
      if (isEmpty()) return T{};
      T value = buffer_[head_];
      head_ = (head_ + 1) % SIZE;
      return value;
   }

   void reset() {
      head_ = 0;
      tail_ = 0;
   }

protected:
   uint8_t head_;
   uint8_t tail_;
   T buffer_[SIZE];
};
