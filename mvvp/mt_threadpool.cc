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

void mt_threadpool::task::task_core_execute(void) {
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
			      to_enqueue->task_enqueue();
			}
			self = remaining;
		  }
	    }
      } while(self);
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

