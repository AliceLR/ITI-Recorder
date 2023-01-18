/* ITI Recorder
 *
 * Copyright (C) 2023 Alice Rowan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "Platform.hpp"

void Platform::delay(unsigned ms)
{
  struct timespec req;
  struct timespec rem;

  req.tv_sec = ms / 1000;
  req.tv_nsec = (ms % 1000) * 1000000;

  while(nanosleep(&req, &rem))
  {
    if(errno != EINTR)
      return;

    req = rem;
  }
}

void Platform::wait_input()
{
  for(int c = 0; c != '\n' && c != EOF; c = fgetc(stdin));
}
