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
      mt_spin_mutex
	    - spinlock-based mutex
	    - (should be) optimized when possible
      mt_lock
	    - RAII locking class accepting anything with lock() and unlock()
	      member functions.
      mt_release_lock
	    - RAII unlocking class accepting an mt_lock.
	    - unlocks at creation and re-locks at destruction
      mt_sema
	    - semaphore
      mt_counter
	    - simple synchronized counter
	    - (should be) optimized when possible
      mt_tls
	    - thread-local variable
	    - (should be) optimized when possible
*/

/*TODO: optimized versions of mt_spin_mutex and mt_counter
Can use:
      inline assembly
      GCC atomic builtins
      Windows Interlocked builtins
*/

#include<pthread.h>
#include<semaphore.h>
#include<assert.h>

#include"mt_support.h"

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
};
/*NOTE!  Sufficiently old implementations of pthreads may not provide
spinlocks.  In such cases if we can't implement architecture-specific
efficient spinlocks we might #define mt_spin_mutex mt_mutex.
In addition, a spinlock using inline assembly or GCC atomic builtins
is more likely to be better optimized.
*/
class mt_spin_mutex : noncopyable {
private:
      pthread_spinlock_t M;
public:
      mt_spin_mutex(void) { 
	    int pthread_spin_init_result = pthread_spin_init(&M, PTHREAD_PROCESS_PRIVATE);
	    assert(pthread_spin_init_result == 0);
      }
      ~mt_spin_mutex(void) {
	    int pthread_spin_destroy_result = pthread_spin_destroy(&M);
	    assert(pthread_spin_destroy_result == 0);
      }

      void lock(void) {
	    int pthread_spin_lock_result = pthread_spin_lock(&M);
	    assert(pthread_spin_lock_result == 0);
      }
      void unlock(void) {
	    int pthread_spin_unlock_result = pthread_spin_unlock(&M);
	    assert(pthread_spin_unlock_result == 0);
      }
};

template<class T> class mt_release_lock;
template<class T>
class mt_lock : noncopyable {
private:
      T& M;

      mt_lock(void); /*disallowed!*/

public:
      mt_lock(T& nM) : M(nM) { M.lock(); }
      ~mt_lock() { M.unlock(); }

      friend class mt_release_lock<T>;
};
template<class T>
class mt_release_lock : noncopyable {
private:
      T& M;

      mt_release_lock(void); /*disallowed!*/

public:
      mt_release_lock(mt_lock<T>& L) : M(L.M) { M.unlock(); }
      ~mt_release_lock() { M.lock(); }
};

class mt_sema : noncopyable {
private:
      sem_t S;

public:
      explicit mt_sema(unsigned int i = 0) {
	    int sem_init_result = sem_init(&S, 0, i);
	    assert(sem_init_result == 0);
      }
      ~mt_sema() {
	    int sem_destroy_result = sem_destroy(&S);
	    assert(sem_destroy_result == 0);
      }

      void post(void) {
	    int sem_post_result = sem_post(&S);
	    assert(sem_post_result == 0);
      }
      void wait(void) {
	    int sem_wait_result = sem_wait(&S);
	    assert(sem_wait_result == 0);
      }
};

/*TODO! this really really wants to be architecture-specific optimized*/
class mt_counter {
private:
      mt_spin_mutex M;
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
	    assert(pthread_key_create_result == 0);
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
