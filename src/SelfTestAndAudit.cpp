/**************************************************************************
*     This file is part of the RPU OS for Arduino Project.

   I, Dick Hamill, the author of this program disclaim all copyright
   in order to make this program freely available in perpetuity to
   anyone who would like to use it. Dick Hamill, 6/1/2020

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

#include "SelfTestAndAudit.h"

static unsigned long LastSelfTestChange = 0;

unsigned long GetLastSelfTestChangedTime() {
   return LastSelfTestChange;
}

void SetLastSelfTestChangedTime(unsigned long setSelfTestChange) {
   LastSelfTestChange = setSelfTestChange;
}
