/*
  $Id: utils.h,v 1.3 2004/12/19 01:43:38 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static inline char *
catstring(char *buff, const char *s)
{
  if(s){
    if(buff)
      buff=realloc(buff,strlen(buff)+strlen(s)+1);
    else
      buff=calloc(strlen(s)+1,1);
    strcat(buff,s);
  }
  return(buff);
}
