/* vim: set shiftwidth=6 softtabstop=6 : */
#ifndef __access_set_H
#define __access_set_H
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

#include<stdlib.h>
#include<stdint.h>

#include"hash_mix.h"

class access_set {
private:
      static const size_t bloom_bits = 1024;

      static const size_t word_bits = (8 * sizeof(int));
      static const size_t bloom_words = bloom_bits / word_bits;
      static const size_t real_bloom_bits = bloom_words * word_bits;

      int array[bloom_words];

public:
      access_set(void) {
	    for(size_t i = 0; i < bloom_words; ++i) { array[i] = 0; }
      }
      void insert_i(uintptr_t I) {
	    int p = hash::mix<52309, 198311>(I);
      }
      template<class T>
      void insert(T* p) {
	    insert_i(reinterpret_cast<uintptr_t>(p));
      }
      bool intersects(access_set const& o) const {
	    for(size_t i = 0; i < bloom_words; ++i) {
		  if((array[i] & o.array[i]) != 0) return 1;
	    }
	    return 0;
      }
};

#endif /* __access_set_H */

