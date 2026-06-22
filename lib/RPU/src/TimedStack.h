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

struct TimedSolenoidEntry {
   uint8_t inUse;
   unsigned long pushTime;
   uint8_t solenoidNumber;
   uint8_t numPushes;
   uint8_t disableOverride;
};

struct TimedSoundEntry {
   uint8_t inUse;
   unsigned long pushTime;
   unsigned short soundNumber;
   uint8_t numPushes;
};

/**
 * TimedStack - A fixed-size array of timed entries
 *
 * This template class manages a collection of time-delayed actions.
 * It provides direct access to entries for flexible usage patterns.
 * Used by TimedSolenoidStack and TimedSoundStack in the RPU library.
 *
 * The EntryType must have an 'inUse' field for tracking entry status.
 *
 * @tparam EntryType The struct type for entries (must have inUse field)
 * @tparam SIZE The maximum number of timed entries
 */
template <typename EntryType, uint8_t SIZE> class TimedStack {
 public:
   EntryType entries[SIZE];

   TimedStack() {
      // Initialize all entries to zero
      for (uint8_t count = 0; count < SIZE; count++) {
         // This works for POD types and will zero-initialize all fields
         entries[count] = {0};
      }
   }

   /**
    * Find the first available slot and return its index
    * @return Index of free slot, or SIZE if no slots available
    */
   uint8_t findFreeSlot() const {
      for (uint8_t count = 0; count < SIZE; count++) {
         if (!entries[count].inUse) {
            return count;
         }
      }
      return SIZE;
   }

   /**
    * Get the maximum number of entries
    * @return The size of the stack
    */
   constexpr uint8_t getSize() const {
      return SIZE;
   }

   /**
    * Reset all entries to zero state
    */
   void reset() {
      for (uint8_t count = 0; count < SIZE; count++) {
         entries[count] = {0};
      }
   }
};
