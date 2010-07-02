/* vim: set shiftwidth=6 softtabstop=6 : */
#ifndef __mt_synch_H
#define __mt_synch_H
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

/*defined classes:
      mt_mutex
	    - mutex type
      mt_lock
	    - RAII locking class accepting anything with lock() and unlock()
	      member functions.
      mt_release_lock
	    - RAII unlocking class accepting an mt_lock.
	    - unlocks at creation and re-locks at destruction
      mt_condvar
	    - condition variable
      mt_sema
	    - semaphore
      mt_counter
	    - simple synchronized counter
	    - (should be) optimized when possible
      mt_tls
	    - thread-local variable
	    - (should be) optimized when possible
*/

/*TODO: optimized versions of mt_counter
Can use:
      inline assembly
      GCC atomic builtins
      Windows Interlocked builtins
*/

#include<pthread.h>
#include<semaphore.h>
#include<assert.h>

#include"mt_support.h"

class mt_condvar;

class mt_mutex : noncopyable {
private:
      pthread_mutex_t M;
public:
      mt_mutex(void) { 
	    int pthread_mutex_init_result = pthread_mutex_init(&M, 0);
	    assert(pthread_mutex_init_result == 0);
      }
      ~mt_mutex(void) {
	    int pthread_mutex_destroy_result = pthread_mutex_destroy(&M);
	    assert(pthread_mutex_destroy_result == 0);
      }

      void lock(void) {
	    int pthread_mutex_lock_result = pthread_mutex_lock(&M);
	    assert(pthread_mutex_lock_result == 0);
      }
      void unlock(void) {
	    int pthread_mutex_unlock_result = pthread_mutex_unlock(&M);
	    assert(pthread_mutex_unlock_result == 0);
      }

      friend class mt_condvar;
};

class mt_lock : noncopyable {
private:
      mt_mutex& M;

      mt_lock(void); /*disallowed!*/

public:
      explicit mt_lock(mt_mutex& nM) : M(nM) { M.lock(); }
      ~mt_lock() { M.unlock(); }

      friend class mt_release_lock;
      friend class mt_condvar;
};
class mt_release_lock : noncopyable {
private:
      mt_mutex& M;

      mt_release_lock(void); /*disallowed!*/

public:
      explicit mt_release_lock(mt_lock& L) : M(L.M) { M.unlock(); }
      ~mt_release_lock() { M.lock(); }
};

class mt_condvar : noncopyable {
private:
      pthread_cond_t CV;

public:
      mt_condvar(void) {
	    int pthread_cond_init_result = pthread_cond_init(&CV, 0);
	    assert(pthread_cond_init_result == 0);
      }
      void signal(void) {
	    int pthread_cond_signal_result = pthread_cond_signal(&CV);
	    assert(pthread_cond_signal_result == 0);
      }
      void wait(mt_lock& L) {
	    int pthread_cond_wait_result = pthread_cond_wait(&CV, &L.M.M);
	    assert(pthread_cond_wait_result == 0);
      }
};

class mt_sema : noncopyable {
private:
      mt_mutex M;
      mt_condvar CV;
      unsigned int S;
      unsigned int waiters;

public:
      explicit mt_sema(unsigned int i = 0) : S(i), waiters(0) { }
      ~mt_sema() {
      }

      void post(void) {
	    mt_lock L(M);
	    ++S;
	    if(waiters) CV.signal();
      }
      void wait(void) {
	    mt_lock L(M);
	    if(S == 0) {
		  ++waiters;
		  do {
			CV.wait(L);
		  } while(S == 0);
		  --waiters;
	    }
	    --S;
      }
};

/*TODO! this really really wants to be architecture-specific optimized*/
class mt_counter {
private:
      mt_mutex M;
      unsigned int C;

public:
      explicit mt_counter(unsigned int i = 0) : C(i) { }

      unsigned int set(unsigned int i) {
	    mt_lock L(M);
	    C = i;
	    return C;
      }

      unsigned int increment(void) {
	    mt_lock L(M);
	    ++C;
	    return C;
      }
      unsigned int decrement(void) {
	    mt_lock L(M);
	    --C;
	    return C;
      }
};

class mt_tls {
private:
      pthread_key_t K;

public:
      explicit mt_tls(void(*f)(void*) = 0) {
	    int pthread_key_create_result = pthread_key_create(&K, f);
	    assert(pthread_key_create_result == 0);
      }
      ~mt_tls() {
	    int pthread_key_delete_result = pthread_key_delete(K);
	    assert(pthread_key_delete_result == 0);
      }
      void* get(void) const {
	    return pthread_getspecific(K);
      }
      void* set(void* p) {
	    int pthread_setspecific_result = pthread_setspecific(K, p);
	    assert(pthread_setspecific_result == 0);
	    return p;
      }
};

#endif /* __mt_synch_H */
