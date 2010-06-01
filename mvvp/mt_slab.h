/* vim: set shiftwidth=6 softtabstop=6 : */
#ifndef __mt_slab_H
#define __mt_slab_H
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

/*
drop-in multithreaded replacement for slab allocator

This allocator is really a "magazine allocator".
A thread-local structure retains two magazines, which
are small sets of objects that are ready for allocation
(in effect, a thread-local slab).  When both magazines
are empty, we synchronize on the global slab allocator
and acquire a magazine's worth of objects.  When both
magazines are full, we also synchronize on the global
slab allocator and release a magazine's worth of
objects.
*/

#include"mt_support.h"

#define MT_MAGAZINE_SIZE 16

template <size_t SLAB_SIZE, size_t CHUNK_COUNT> class slab_t {
private:

      union item_cell_u;

      /*next magazine, and next object within the same magazine*/
      struct nexts_s {
	    item_cell_u* object;
	    item_cell_u* magazine;
      };
      /*the use of the above cell means that objects less than
      sizeof(void*) * 2 are less efficiently allocated.  But
      slab_t is used primarily in the schedule.cc event classes.
      The basic event_s class is already typically sizeof(void*)
      * 2, since it has a next pointer and uses virtual methods,
      implying a vtable pointer.
      */

      union item_cell_u {
	    nexts_s next;
	    char space[SLAB_SIZE];
      };

      static const size_t raw_magazine_count = CHUNK_COUNT / MT_MAGAZINE_SIZE;
      static const size_t real_chunk_count = MT_MAGAZINE_SIZE * raw_magazine_count;

      /*synchronized*/
      mt_spin_mutex slab_M;
      /*if heap is empty, is some other thread already allocating?*/
      bool allocating;
      /*if we found an empty heap while another thread is allocating, we are a waiter
      on the semaphore below
      */
      size_t waiters;
      mt_sema waiter_sema;

      /*the actual heap*/
      item_cell_u* heap;
public:
      unsigned long pool;
private:
      /*end synchronized*/

      item_cell_u initial_heap[real_chunk_count];

      /*structure used for thread-local allocation*/
      class tla {
      private:
	    item_cell_u* first; /*two magazines*/
	    item_cell_u* second;

	    size_t capacity; /*number of objects in the two magazines*/

      public:
	    void* alloc_slab(slab_t* parent);
	    void  free_slab(slab_t* parent, void*);
	    tla(void) : first(0), second(0), capacity(0) { }
      };

      friend class tla;

      /*slot for thread-local allocator*/
      mt_tls tlp;
      /*destructs the thread-local allocator structure*/
      static inline void tla_free(void*);

      /*given an arrayed chunk, sets up the magazine chain structure*/
      void setup_magazines(item_cell_u*);

      /*used by thread-local allocators to get space from global heap*/
      item_cell_u* get_magazine(void);
      /*used by thread-local allocators to return space to global heap*/
      void return_magazine(item_cell_u*);

public:
      slab_t(void);

      void* alloc_slab(void);
      void  free_slab(void*);
};

template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
slab_t<SLAB_SIZE, CHUNK_COUNT>::slab_t(void) : tla(0, &tla_free) {
      pool = real_chunk_count;
      heap = initial_heap;
      setup_magazines(heap);
}

template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
void slab_t<SLAB_SIZE, CHUNK_COUNT>::setup_magazines(item_cell_u* arr) {
      size_t i;
      for(i = 0; i < raw_magazine_count - 1; ++i) {
	    for(size_t j = 0; j < MT_MAGAZINE_SIZE - 1; ++j) {
		  arr[i * MT_MAGAZINE_SIZE + j]->next.object =
			&arr[i * MT_MAGAZINE_SIZE + j + 1]
		  ;
	    }
	    arr[i * MT_MAGAZINE_SIZE]->next.magazine =
		  &arr[(i + 1) * MT_MAGAZINE_SIZE]
	    ;
      }
      /*fill in the last magazine*/
      for(size_t j = 0; j < MT_MAGAZINE_SIZE - 1; ++j) {
	    arr[i * MT_MAGAZINE_SIZE + j]->next.object =
		  &arr[i * MT_MAGAZINE_SIZE + j + 1]
	    ;
      }
      arr[i * MT_MAGAZINE_SIZE]->next.magazine = 0;
}

template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
item_cell_u* slab_t<SLAB_SIZE, CHUNK_COUNT>::get_magazine(void) {
      item_cell_u* rv;
      size_t to_wake = 0;
      {mt_lock L(slab_M);
	    /*slab_M is a spinlock, which is good for the typical case
	    when the global slab allocator can provide a magazine.  But
	    when insufficient memory is available, we need to ask memory
	    from the system allocator, and worse, we need to initialize
	    that memory.  This can take time.  This is not good for
	    spinlocks.  In such a case we emulate "full" mutex locks
	    using a flag and a semaphore, keeping the actual spinlock
	    free during allocation and waiting for allocation to
	    complete.
	    */
	    if(!heap) {
		  if(!allocating) {
			allocating = 1;
			item_cell_u* nheap;
			{mt_release_lock R(L);
			      nheap = new item_cell_u[real_chunk_count];
			      setup_magazines(nheap);
			}
			heap = nheap;
			pool += real_chunk_count;
			allocating = 0;
			to_wake = waiters; waiters = 0;
		  } else {
			++waiters;
			{mt_release_lock R(L);
			      waiter_sema.wait();
			}
		  }
	    }
	    rv = heap;
	    heap = rv->next.magazine;
      }
      while(to_wake) { waiter_sema.post(); --to_wake; }
      return rv;
}

template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
void slab_t<SLAB_SIZE, CHUNK_COUNT>::return_magazine(item_cell_u* rv) {
      mt_lock L(slab_M);
      rv->next.magazine = heap;
      heap = rv;
}

/*thread-local allocation*/
template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
void* slab_t<SLAB_SIZE, CHUNK_COUNT>::tla::alloc_slab(slab_t<SLAB_SIZE, CHUNK_COUNT>* parent) {
      if(capacity == 0) {
	    first = parent->get_magazine();
	    capacity = MT_MAGAZINE_SIZE;
      }
      --capacity;
      void* rv;
      if(capacity < MT_MAGAZINE_SIZE) {
	    rv = (void*) first;
	    first = first->next.object;
      } else {
	    rv = (void*) second;
	    second = second->next.object;
      }
      return rv;
}
/*thread-local freeing*/
template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
void slab_t<SLAB_SIZE, CHUNK_COUNT>::tla::free_slab(slab_t<SLAB_SIZE, CHUNK_COUNT>* parent, void* vrv) {
      if(capacity == 2 * MT_MAGAZINE_SIZE) {
	    parent->return_magazine(first);
	    first = second; second = 0;
	    capacity = MT_MAGAZINE_SIZE;
      }
      item_cell_u* rv = reinterpret_cast<item_cell_u*>(vrv);
      if(capacity < MT_MAGAZINE_SIZE) {
	    rv->next.object = first;
	    first = rv;
      } else {
	    rv->next.object = second;
	    second = rv;
      }
      ++capacity;
}
/*thread-local allocator cleanup*/
template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
void slab_t<SLAB_SIZE, CHUNK_COUNT>::tla_free(void* p) {
      tla* tp = reinterpret_cast<tla*>(p);
      delete tp;
}

template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
void* slab_t<SLAB_SIZE, CHUNK_COUNT>::alloc_slab(void) {
      tla* tp = reinterpret_cast<tla*>(tlp.get());
      if(!tp) {
	    tp = new tla;
	    tlp.set((void*) tp);
      }
      return tp->alloc_slab(this);
}
template<size_t SLAB_SIZE, size_t CHUNK_COUNT>
void slab_t<SLAB_SIZE, CHUNK_COUNT>::free_slab(void* vrv) {
      tla* tp = reinterpret_cast<tla*>(tlp.get());
      if(!tp) {
	    tp = new tla;
	    tlp.set((void*) tp);
      }
      tp->free_slab(this, vrv);
}

#endif /* __mt_slab_H */
