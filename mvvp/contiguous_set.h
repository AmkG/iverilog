/* vim: set shiftwidth=6 softtabstop=6 : */
#ifndef __contiguous_set_H
#define __contiguous_set_H
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
A contiguous set is a set where we expect most entries to
be (1) contiguous to one another in terms of a ++ operator,
and (2) will tend to be inserted "in order".

For example a contiguous set is more compact for representing
the set {1, 2, 3, 4, 5, 42, 43, 44, 45}.

The representation is practically a binary tree whose entries
are ranges.
*/

/*
operations:

      o.find(t)
	    => returns a bool whether or not the entry is
	    found in the set
      o.insert(t)
	    => PRECONDITION: o.find(t) must be false
	    => inserts t into the set
	    => POSTCONDITION: o.find(t) will be true afterwards
*/

template<class
      /*CopyConstructible LessThanComparable EqualityComparable*/
	    T>
class contiguous_set {
private:
      class node {
      private:
	    node(void); /*disallowed!*/
      public:
	    T begin;
	    T end; /*one past the end*/
	    node* L; /*less*/
	    node* R; /*greater*/
	    explicit node(T const& e) : begin(e), end(e) { ++end; }
	    node(node const& o)
		  : begin(o.begin), end(o.end)
		  , L(o.L ? new node(*o.L) : 0)
		  , R(o.R ? new node(*o.R) : 0) { }
	    ~node() {
		  delete L; delete R;
	    }
      };

      static inline bool find_at(node* N, T const& t) {
	    for(;;) {
		  if(!N) return 0;
		  if(t < N->begin) {
			N = N->L;
		  } else if(t < N->end) {
			return 1;
		  } else {
			N = N->R;
		  }
	    }
      }
      static inline void insert_at(node** pN, T const& t) {
	    for(;;) {
		  node*& N = *pN;
		  if(!N) {
			N = new node(t); return;
		  } else if(t < N->begin) {
			pN = &N->L;
		  } else if(t < N->end) {
			bool contiguous_set_should_not_insert_when_found = 0;
			assert(contigious_set_should_not_insert_when_found);
		  } else if(t == N->end) {
			++N->end; return;
		  } else {
			pN = &N->R;
		  }
	    }
      }

      node* root;

public:
      contiguous_set(void) { }
      contiguous_set(contiguous_set const& o)
	    : root(o.root ? new node(*o.root) : 0) { }
      ~contiguous_set() { delete root; }
      bool find(T const& t) const {
	    return find_at(root, t);
      }
      void insert_at(T const& t) {
	    insert_at(&root, t);
      }
};

#endif /* __contiguous_set_H */
