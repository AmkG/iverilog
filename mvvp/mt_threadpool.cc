/* vim: set shiftwidth=6 softtabstop=6 : */
/*
 * Copyright (c) 2010 Alan Manuel K. Gloria (almkglor@gmail.com)
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

#include"mt_threadpool.h"

/*In the future, use thread-local stores with a small deque
(i.e. work-stealing)
*/
class mt_threadpool::per_thread {
public:
};

class mt_threadpool::core {
private:
      /*synchronized*/
      mt_mutex M;
      mt_condvar CV;
      size_t waiters;
      size_t top;
      size_t capacity;
      task** arr;
      /*endsynchronized*/
public:
      core(void);
      ~core();

      void enqueue(task* t);
      task* dequeue(void);

      void core_thread_code(void);
};

static mt_threadpool::core global_queue;

void mt_threadpool::core::core_thread_code(void) {
      per_thread pt;
      for(;;) {
	    /*TODO: use per-thread work stealing*/
	    task* t = dequeue();
	    t->task_core_execute(pt);
      }
}

static void per_thread_code(void* _ignored_) {
      global_queue.core_thread_code();
}

mt_threadpool::core::core(void)
      : waiters(0), top(0), capacity(0), arr(0) { }
mt_threadpool::core::~core() {
      delete [] arr;
}

void mt_threadpool::core::enqueue(task* t) {
      mt_lock L(M);
      if(top == capacity) {
	    size_t ncapacity = (capacity + 4);
	    ncapacity += ncapacity / 2;
	    task** narr = new task*[ncapacity];
	    for(size_t i = 0; i < top; ++i) {
		  narr[i] = arr[i];
	    }
	    delete [] arr;
	    arr = narr;
	    capacity = ncapacity;
      }
      arr[top] = t;
      ++top;
      if(waiters != 0) {
	    CV.signal();
      }
}

mt_threadpool::task* mt_threadpool::core::dequeue(void) {
      mt_lock L(M);
      if(top == 0) {
	    ++waiters;
	    do {
		  CV.wait(L);
	    } while(top == 0);
	    --waiters;
      }
      --top;
      return arr[top];
}

/*
 * mt_threadpool::task member functions
 */

void mt_threadpool::task::task_core_execute(per_thread& pt) {
      task* to_wait_on;
      void* rv;

      mt_mutex dummyM; /*dummy mutex to force memory fences*/
      mt_lock L(dummyM);

      task* self = this;
      do {
	    to_wait_on = self->task_wait_on();
	    if(to_wait_on) {
		  mt_lock L2(to_wait_on->M);
		  if(to_wait_on->finished) {
			continue;
		  } else {
			self->todo_list_next = to_wait_on->todo_list;
			to_wait_on->todo_list = self;
			return;
		  }
	    } else {
		  rv = self->task_execute();

		  {mt_release_lock R(L);
			task* remaining;
			thread_waiters_s* waiters;
			{mt_lock L2(self->M);
			      self->return_value = rv;
			      remaining = self->todo_list;
			      self->todo_list = 0;
			      waiters = self->thread_waiters;
			      self->thread_waiters = 0;
			      self->finished = 1;
			}
			if(waiters) { waiters->post(); }
			if(!remaining) return;
			while(remaining->todo_list_next) {
			      task* to_enqueue = remaining;
			      remaining = remaining->todo_list_next;
			      /*TODO: insert into per_thread structure
			      rather than global pool
			      */
			      to_enqueue->task_enqueue();
			}
			self = remaining;
		  }
	    }
      } while(self);
}

void mt_threadpool::task::task_enqueue(void) {
      global_queue.enqueue(this);
}

bool mt_threadpool::task::task_completed(void) {
      mt_lock L(M);
      return finished;
}

void* mt_threadpool::task::task_wait_completion(bool reset_flag) {
      mt_lock L(M);
      if(!finished) {
	    if(thread_waiters) {
		  thread_waiters->inc_waiters();
		  thread_waiters_s* waiter = thread_waiters;
		  {mt_release_lock R(L);
			waiter->wait();
		  }
	    } else {
		  thread_waiters_s my_waiter;
		  thread_waiters = &my_waiter;
		  {mt_release_lock R(L);
			my_waiter.owner_wait();
		  }
	    }
      }
      void* rv = return_value;
      if(reset_flag) {
	    finished = 0; return_value = 0;
	    return rv;
      } else {
	    return rv;
      }
}

mt_threadpool::task::task(void)
      : finished(0)
      , return_value(0)
      , todo_list(0)
      , todo_list_next(0)
      , thread_waiters(0) { }

mt_threadpool::task::task(task const&)
      : finished(0)
      , return_value(0)
      , todo_list(0)
      , todo_list_next(0)
      , thread_waiters(0) { }

/*
 * mt_threadpool::task default implementations
 */

mt_threadpool::task* mt_threadpool::task::task_wait_on(void) {
      return 0;
}

void mt_threadpool::task::task_cleanup(void*) {
      return;
}



