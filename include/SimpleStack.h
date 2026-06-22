//
// Created by Doug Bercot on 6/22/26.
//

#ifndef TRIDENT2020RPU_SIMPLESTACK_H
#define TRIDENT2020RPU_SIMPLESTACK_H

#include <stdint.h>

template <typename T, uint8_t SIZE, T EMPTY_VALUE> class SimpleStack {

public:
   SimpleStack() : first_(0), last_(0) {
      for (uint8_t i = 0; i < SIZE; i++) {
         buffer[i] = EMPTY_VALUE;
      }
   }

   bool isEmpty() const { return first_ == last_; }
   bool isFull() const { return (first_ + 1) % SIZE == last_; }

   T peek() const { return buffer[last_]; }

   T pop() {
      T value = buffer[last_];
      last_ = (last_ + 1) % SIZE;
      return value;
   }

   bool push(T value) {
      if (isFull()) {
         return false;
      }
      buffer[first_] = value;
      first_ = (first_ + 1) % SIZE;
      return true;
   }

protected:
   uint8_t first_;
   uint8_t last_;
   T buffer[SIZE];
};

#endif // TRIDENT2020RPU_SIMPLESTACK_H
