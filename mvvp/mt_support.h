/* vim: set shiftwidth=6 softtabstop=6 : */
#ifndef __mt_support_H
#define __mt_support_H
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

/*simple utility class*/
class noncopyable {
private:
      noncopyable(noncopyable const&); /*disallowed*/
      noncopyable& operator=(noncopyable const&); /*disallowed*/
protected:
      noncopyable(void) { }
};

/*
 * drop-in replacement for std::unique_ptr.  In the future
 * when this gets common enough replace with
 * #include<memory>
 * using std::unique_ptr;
 */
template<class T>
class unique_ptr : noncopyable {
private:
      T* p;

public:
      typedef T element_type;
      typedef T* pointer;

      unique_ptr(void) : p(0) { }
      explicit unique_ptr(pointer np) : p(np) { }

      ~unique_ptr() { delete p; }
      void swap(unique_ptr& o) {
	    pointer tmp = p;
	    p = o.p
	    o.p = tmp;
      }
      pointer release(void) {
	    pointer tmp = p;
	    delete p; p = 0;
	    return tmp;
      }
      void reset(pointer np = 0) {
	    if(p == np) return;
	    unique_ptr tmp(np);
	    swap(tmp);
      }
      pointer get(void) const { return p; }
      element_type& operator*(void) const { return *p; }
      pointer operator->(void) const { return p; }

      bool operator!(void) const { return !p; }

      /*safe bool idiom*/
private:
      typedef void (unique_ptr::*bool_type)(void) const;
      void safe_bool_idiom(void) const;
public:
      operator bool_type(void) const {
	    if(p) {
		  return &unique_ptr::safe_bool_idiom;
	    } else return 0;
      }

};

#endif
