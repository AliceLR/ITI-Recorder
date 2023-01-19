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

#ifndef EVENT_HPP
#define EVENT_HPP

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <queue>

class Event
{
public:
  int time_ms;

  Event(int _time_ms): time_ms(_time_ms) {}

  virtual void task() const = 0;
};

class EventCompare
{
public:
  bool operator()(const std::shared_ptr<Event> &a,
   const std::shared_ptr<Event> &b) const
  {
    return a->time_ms > b->time_ms;
  }
};

class EventSchedule
{
  static constexpr int START_TIME = -3;
  int prev_time_ms = START_TIME;
  int max_time_ms = 0;

  std::priority_queue<
    std::shared_ptr<Event>,
    std::vector<std::shared_ptr<Event>>,
    EventCompare> queue;

  std::vector<std::shared_ptr<Event>> dont_free;

public:
  static constexpr int NOTICE_TIME = -2;
  static constexpr int PROGRAM_TIME = -1;

  void push(const std::shared_ptr<Event> &e)
  {
    max_time_ms = std::max(max_time_ms, e->time_ms);
    queue.push(e);
    dont_free.push_back(e);
  }

  void push(std::shared_ptr<Event> &&e)
  {
    max_time_ms = std::max(max_time_ms, e->time_ms);
    dont_free.push_back(e);
    queue.push(std::move(e));
  }

  std::shared_ptr<Event> pop()
  {
    std::shared_ptr<Event> p = queue.top();

    prev_time_ms = p->time_ms;

    queue.pop();
    return p;
  }

  const std::shared_ptr<Event> &peek() const
  {
    return queue.top();
  }

  bool has_next() const
  {
    return !queue.empty();
  }

  int previous_time() const
  {
    return prev_time_ms;
  }

  int next_time() const
  {
    return peek()->time_ms;
  }

  int remaining_duration() const
  {
    return max_time_ms - prev_time_ms;
  }

  int total_duration() const
  {
    return max_time_ms;
  }
};


/* Events */
class NoticeEvent : public Event
{
  std::vector<char> message;

public:
  NoticeEvent(const char *_message, int _time_ms): Event(_time_ms)
  {
    size_t len = strlen(_message);
    message.insert(message.begin(), _message, _message + len + 1);
  }

  //virtual ~NoticeEvent() {}

  virtual void task() const
  {
    fprintf(stderr, "%s\n", message.data());
  }

  static void schedule(EventSchedule &ev, const char *_message)
  {
    ev.push(std::shared_ptr<Event>(new NoticeEvent(_message, EventSchedule::NOTICE_TIME)));
  }
};

#endif /* EVENT_HPP */
