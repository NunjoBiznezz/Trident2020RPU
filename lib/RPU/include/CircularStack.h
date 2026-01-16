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
 * CircularStack - A generic circular buffer/stack implementation
 *
 * This template class provides a thread-safe circular buffer using volatile
 * members to support interrupt-driven code. It's used by SwitchStack,
 * SolenoidStack, and SoundStack in the RPU library.
 *
 * @tparam T The type of elements stored in the stack
 * @tparam SIZE The maximum number of elements the stack can hold
 * @tparam EMPTY_VALUE The sentinel value used to indicate an empty slot
 */
template <typename T, uint8_t SIZE, T EMPTY_VALUE> class CircularStack {
 private:
   volatile T buffer[SIZE];
   volatile uint8_t firstIndex;
   volatile uint8_t lastIndex;

 public:
   CircularStack() : firstIndex(0), lastIndex(0) {
      for (uint8_t i = 0; i < SIZE; i++) {
         buffer[i] = EMPTY_VALUE;
      }
   }

   /**
    * Push a value onto the stack
    * @param value The value to push
    * @return true if successful, false if stack is full
    */
   bool push(T value) {
      if (!spaceLeft()) {
         return false;
      }
      buffer[lastIndex] = value;
      lastIndex++;
      if (lastIndex >= SIZE) {
         lastIndex = 0;
      }
      return true;
   }

   /**
    * Push a value to the front of the stack (higher priority)
    * @param value The value to push
    * @return true if successful, false if stack is full
    */
   bool pushFront(T value) {
      if (!spaceLeft()) {
         return false;
      }
      if (firstIndex == 0) {
         firstIndex = SIZE - 1;
      } else {
         firstIndex--;
      }
      buffer[firstIndex] = value;
      return true;
   }

   /**
    * Pull the first value from the stack
    * @return The first value, or EMPTY_VALUE if stack is empty
    */
   T pull() {
      if (isEmpty()) {
         return EMPTY_VALUE;
      }
      T value = buffer[firstIndex];
      buffer[firstIndex] = EMPTY_VALUE;
      firstIndex++;
      if (firstIndex >= SIZE) {
         firstIndex = 0;
      }
      return value;
   }

   /**
    * Check if there's space left in the stack
    * @return true if there's at least one empty slot
    */
   bool spaceLeft() const {
      return buffer[lastIndex] == EMPTY_VALUE;
   }

   /**
    * Check if the stack is empty
    * @return true if the stack contains no elements
    */
   bool isEmpty() const {
      return buffer[firstIndex] == EMPTY_VALUE;
   }

   /**
    * Peek at the value that would be pulled next without removing it
    * @return The first value, or EMPTY_VALUE if stack is empty
    */
   T peek() const {
      return buffer[firstIndex];
   }

   /**
    * Reset the stack to empty state
    */
   void reset() {
      firstIndex = 0;
      lastIndex = 0;
      for (uint8_t i = 0; i < SIZE; i++) {
         buffer[i] = EMPTY_VALUE;
      }
   }
};
