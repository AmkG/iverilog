/* vim: set shiftwidth=6 softtabstop=6 : */
#ifndef __mt_threadpool_H
#define __mt_threadpool_H
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

#include"mt_support.h"
#include"mt_synch.h"

namespace mt_threadpool {
      class core;
      class per_thread;

      class task {
      private:

	    class thread_waiters_s {
	    private:
		  mt_sema S;
		  mt_sema owner_S;
		  size_t waiters;
	    public:
		  thread_waiters_s(void) : waiters(0) { }
		  void inc_waiters(void) { ++waiters; } /*synchronized*/
		  void wait(void) { S.wait(); }
		  void post(void) {
			for(size_t i = waiters; i > 0; --i) { S.post(); }
			owner_S.post();
		  }
		  void owner_wait(void) { owner_S.wait(); }
	    };

	    /*synchronized*/
	    mt_mutex M;
	    bool finished;
	    void* return_value;
	    task* todo_list;
	    task* todo_list_next;
	    thread_waiters_s* thread_waiters;
	    /*end synchronized*/

	    void task_core_execute(per_thread&);

      protected:
	    task(void);
	    task(task const&);

	    /* task_wait_on()
	     *
	     * override this member function in order to create an
	     * "execution guard".  If this function returns a pointer
	     * to some other task, this task will be delayed until
	     * after that other task completes.
	     *
	     * This member function is called repeatedly until it
	     * returns a null pointer.  Only when this member function
	     * returns null will this task actually be executed.  If
	     * this member function ever returns a non-null pointer,
	     * it will be called repeatedly.
	     *
	     * Provided you do not violate other thread pool
	     * requirements, this member function will only be called
	     * from one thread at a time.  However it may be called
	     * from different threads at different times.  It is
	     * recommended that the deriving class include state
	     * variables for keeping track of where in its list of
	     * dependencies it is now waiting on.
	     *
	     * The default implementation returns null, meaning to
	     * execute the task immediately.
	     */
	    virtual task* task_wait_on(void);

	    /* task_execute()
	     *
	     * override this member function to perform the "payload"
	     * of your actual task.  Its return value will be stored.
	     */
	    virtual void* task_execute(void) =0;

	    /* task_cleanup()
	     *
	     * override this member function to clean up the return
	     * value of your payload.
	     */
	    virtual void task_cleanup(void*);

      public:
	    /* task_enqueue()
	     *
	     * Enqueues the task, allowing it to be executed on the
	     * thread pool.
	     *
	     * A task can be safely enqueued only exactly once at any
	     * time.  When completed, use task_release() to reset the
	     * task to its pre-running state.
	     */
	    void task_enqueue(void);

	    /* task_completed()
	     *
	     * return true if the task has completed.  Note that
	     * if it returns false, by the time it returns the task
	     * may have already completed; use some other
	     * synchronization mechanism if you trigger on
	     * non-completion.
	     */
	    bool task_completed(void);

	    /* task_release()
	     *
	     * Returns the given void* and resets the state of the
	     * task.   The task no longer owns the return value of
	     * the payload function, and is no longer considered
	     * "completed".
	     *
	     * This task can only be safely called if the task is
	     * already completed.
	     */
	    void* task_release(void);

	    /* task_wait_completion(reset_flag = false)
	     *
	     * Blocks the current thread until this task completes.
	     * If the given reset_flag is true, also reset the
	     * task before returning.
	     *
	     * This function can be safely called with false
	     * reset_flag from multiple threads.  If
	     * reset_flag is true, it can only be safely called
	     * from one thread.
	     */
	    void* task_wait_completion(bool reset_flag = 0);

	    friend class core;
	    friend class per_thread;
      };
};

#endif /* __mt_threadpool_H */

