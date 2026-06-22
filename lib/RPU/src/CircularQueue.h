/**************************************************************************
 *     This file is part of the RPU OS for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 1/16/2026

    RPU OS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPU OS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>

/**
 * CircularQueue - A generic circular buffer (FIFO queue) implementation
 *
 * This template class provides a thread-safe circular buffer using volatile
 * members to support interrupt-driven code. It is used by SwitchStack,
 * SolenoidStack, and SoundStack in the RPU library.
 *
 * @tparam T The type of elements stored in the queue
 * @tparam SIZE The maximum number of elements the queue can hold
 * @tparam EMPTY_VALUE The sentinel value used to indicate an empty slot
 */
template <typename T, uint8_t SIZE, T EMPTY_VALUE> class CircularQueue {
 private:
   volatile T buffer[SIZE];
   volatile uint8_t head;  // read (dequeue) end
   volatile uint8_t tail;  // write (enqueue) end

 public:
   CircularQueue() : head(0), tail(0) {
      for (uint8_t i = 0; i < SIZE; i++) {
         buffer[i] = EMPTY_VALUE;
      }
   }

   T emptyValue() const {
      return EMPTY_VALUE;
   }

   /**
    * Enqueue a value at the tail
    * @param value The value to enqueue
    * @return true if successful, false if queue is full or value equals EMPTY_VALUE
    */
   bool push(T value) {
      if (!spaceLeft() || (value == EMPTY_VALUE)) {
         return false;
      }
      buffer[tail] = value;
      tail++;
      if (tail >= SIZE) {
         tail = 0;
      }
      return true;
   }

   /**
    * Enqueue a value at the head (higher priority — will be dequeued next)
    * @param value The value to enqueue
    * @return true if successful, false if queue is full
    */
   bool pushFront(T value) {
      if (!spaceLeft()) {
         return false;
      }
      if (head == 0) {
         head = SIZE - 1;
      } else {
         head--;
      }
      buffer[head] = value;
      return true;
   }

   /**
    * Dequeue the next value from the head
    * @return The next value, or EMPTY_VALUE if the queue is empty
    */
   T pull() {
      if (isEmpty()) {
         return EMPTY_VALUE;
      }
      T value = buffer[head];
      buffer[head] = EMPTY_VALUE;
      head++;
      if (head >= SIZE) {
         head = 0;
      }
      return value;
   }

   /**
    * Check if there is space to enqueue at least one more element
    * @return true if at least one slot is empty
    */
   bool spaceLeft() const {
      return buffer[tail] == EMPTY_VALUE;
   }

   /**
    * Check if the queue is full
    * @return true if no slots are empty
    */
   bool isFull() const {
      return buffer[tail] != EMPTY_VALUE;
   }

   /**
    * Check if the queue is empty
    * @return true if the queue contains no elements
    */
   bool isEmpty() const {
      return buffer[head] == EMPTY_VALUE;
   }

   /**
    * Peek at the value that would be dequeued next without removing it
    * @return The next value, or EMPTY_VALUE if the queue is empty
    */
   T peek() const {
      return buffer[head];
   }

   /**
    * Reset the queue to empty state
    */
   void reset() {
      head = 0;
      tail = 0;
      for (uint8_t i = 0; i < SIZE; i++) {
         buffer[i] = EMPTY_VALUE;
      }
   }

   bool hasValue(T value) const {
      uint8_t h = head;
      uint8_t t = tail;

      while (h != t) {
         if (buffer[h] == value) {
            return true;
         }
         h++;
         if (h >= SIZE) {
            h = 0;
         }
      }
      return false;
   }
};
