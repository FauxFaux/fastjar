/* $Id: compress.h,v 1.1.1.1 1999-12-06 03:09:12 toast Exp $

   $Log: not supported by cvs2svn $
   Revision 1.3  1999/05/10 08:32:09  burnsbr
   added new function protos.

   Revision 1.2  1999/04/23 12:02:20  burnsbr
   added licence

   Revision 1.1  1999/04/23 11:59:37  burnsbr
   Initial revision


*/

/*
  compress.h - header for compression
  Copyright (C) 1999  Bryan Burns
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* Initializes the compression data structure(s) */
void init_compression();

/* Compresses the file specified by in_fd and appends it to out_fd */
int compress_file(int in_fd, int out_fd, struct zipentry *ze);

/* Frees memory used by compression function */
void end_compression();

void init_inflation();
int inflate_file(pb_file *, int, struct zipentry *);
void end_inflation();
