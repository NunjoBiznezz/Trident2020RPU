#pragma once
#include "SimpleQueue.h"
#include <stdint.h>

constexpr uint16_t INVALID_NOTIFICATION = 0xFFFF;

struct NotificationEntry {
   uint8_t priority;
   uint16_t notificationNum;
   unsigned int duration;
};

// Specializes SimpleQueue for voice notifications. Adds priority-aware
// clearing and a pull() that skips entries invalidated by clearUpToPriority().
template <uint8_t SIZE>
class NotificationQueue : public SimpleQueue<NotificationEntry, SIZE> {
   using Base = SimpleQueue<NotificationEntry, SIZE>;

public:
   bool push(uint16_t notificationNum, unsigned int duration, uint8_t priority) {
      return Base::push({priority, notificationNum, duration});
   }

   // Dequeues the next valid entry, skipping any that were invalidated.
   // Returns an entry with notificationNum == INVALID_NOTIFICATION when empty.
   NotificationEntry pull() {
      while (!Base::isEmpty()) {
         NotificationEntry e = Base::pull();
         if (e.notificationNum != INVALID_NOTIFICATION) return e;
      }
      return {0, INVALID_NOTIFICATION, 0};
   }

   void clearAll() { Base::reset(); }

   // Marks entries whose priority <= maxPriority as invalid without shifting.
   void clearUpToPriority(uint8_t maxPriority) {
      uint8_t i = this->head_;
      while (i != this->tail_) {
         if (this->buffer_[i].priority <= maxPriority) {
            this->buffer_[i].notificationNum = INVALID_NOTIFICATION;
         }
         if (++i >= SIZE) i = 0;
      }
   }

   uint8_t topPriority() const {
      uint8_t i = this->head_;
      uint8_t top = 0;
      while (i != this->tail_) {
         if (this->buffer_[i].notificationNum != INVALID_NOTIFICATION && this->buffer_[i].priority > top) {
            top = this->buffer_[i].priority;
         }
         if (++i >= SIZE) i = 0;
      }
      return top;
   }
};
