#ifndef __event_s_H
#define __event_s_H
/*
 * Copyright (c) 2001-2010 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

struct parallel_event_s;

/*
 * The event_s and event_time_s structures implement the Verilog
 * stratified event queue.
 *
 * The event_time_s objects are one per time step. Each time step in
 * turn contains a list of event_s objects that are the actual events.
 *
 * The event_s objects are base classes for the more specific sort of
 * event.
 */
struct event_s {
      struct event_s*next;
      virtual ~event_s() { }
      virtual void run_run(void) =0;

	// Write something about the event to stderr
      virtual void single_step_display(void);

	// Fallback new/delete
      static void*operator new (size_t size) { return ::new char[size]; }
      static void operator delete(void*ptr)  { ::delete[]( (char*)ptr ); }
#     ifdef MULTITHREADED
private:
      /*
       * Used to query determine if this event_s is also a
       * parallel_event_s, which is a derived class from this.
       *
       * We could use dynamic_cast<> to determine if an event_s is
       * also a parallel_event_s, but it'll be more expensive than
       * querying a single boolean.
       */
      bool is_parallel_flag_;
protected:
      void parallel_make_parallel(void) { is_parallel_flag_ = 1; }
public:
      parallel_event_s* parallel_get_parallel_event(void);
      event_s() { is_parallel_flag_ = 0; }
#     endif /*MULTITHREADED*/
};

struct event_time_s {
      event_time_s() {
	    count_time_events += 1;
	    start = 0;
	    active = 0;
	    nbassign = 0;
	    rwsync = 0;
	    rosync = 0;
	    del_thr = 0;
      }
      vvp_time64_t delay;

      struct event_s*start;
      struct event_s*active;
      struct event_s*nbassign;
      struct event_s*rwsync;
      struct event_s*rosync;
      struct event_s*del_thr;

      struct event_time_s*next;

      static void* operator new (size_t);
      static void operator delete(void*obj, size_t s);
};

/*
 * The parallel_event_s structure allows an event to be placed in
 * a parallel execution queue for execution in a thread pool.
 */
struct parallel_event_s : public event_s {
#     ifdef MULTITHREADED
      parallel_event_s() { parallel_make_parallel(); }
#     endif /*MULTITHREADED*/
};

#ifdef MULTITHREADED
inline parallel_event_s* event_s::parallel_get_parallel_event(void) {
      if(is_parallel_flag_) return static_cast<parallel_event_s*>(this);
      else return 0;
}
#endif /*MULTITHREADED*/

#endif /* __event_s_H */
