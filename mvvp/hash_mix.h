/* vim: set shiftwidth=6 softtabstop=6 : */
#ifndef __hash_mix_H
#define __hash_mix_H
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

namespace hash {
      /*mixer hash.  a and b are random constants.  c is the number to hash*/
      template<int A, int B>
      int mix(int c) {
	    int a = A; int b = B;
	    a=a-b;  a=a-c;  a=a^(c >> 13);
	    b=b-c;  b=b-a;  b=b^(a << 8); 
	    c=c-a;  c=c-b;  c=c^(b >> 13);
	    a=a-b;  a=a-c;  a=a^(c >> 12);
	    b=b-c;  b=b-a;  b=b^(a << 16);
	    c=c-a;  c=c-b;  c=c^(b >> 5);
	    a=a-b;  a=a-c;  a=a^(c >> 3);
	    b=b-c;  b=b-a;  b=b^(a << 10);
	    c=c-a;  c=c-b;  c=c^(b >> 15);
	    return c;
      }
};

#endif /* __hash_mix_H */

