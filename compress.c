/* $Id: compress.c,v 1.1.1.1 1999-12-06 03:09:16 toast Exp $

   $Log: not supported by cvs2svn $
   Revision 1.7  1999/05/10 08:50:05  burnsbr
   *** empty log message ***

   Revision 1.6  1999/05/10 08:38:44  burnsbr
   *** empty log message ***

   Revision 1.5  1999/05/10 08:30:29  burnsbr
   added inflation code

   Revision 1.4  1999/04/27 10:03:33  burnsbr
   added configure support

   Revision 1.3  1999/04/26 02:35:32  burnsbr
   compression now works.. yahoo

   Revision 1.2  1999/04/23 12:01:59  burnsbr
   added licence stuff.

   Revision 1.1  1999/04/23 11:58:25  burnsbr
   Initial revision


*/

/*
  compress.c - code for handling deflation
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

#include "config.h"

#include <zlib.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>

#include "jartool.h"
#include "pushback.h"

extern int seekable;

static char rcsid[] = "$Id: compress.c,v 1.1.1.1 1999-12-06 03:09:16 toast Exp $";

static z_stream zs;

void init_compression(){

  memset(&zs, 0, sizeof(z_stream));

  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  zs.opaque = Z_NULL;

  /* Why -MAX_WBITS?  zlib has an undocumented feature, where if the windowbits
     parameter is negative, it omits the zlib header, which seems to kill
     any other zip/unzip program.  This caused me SO much pain.. */
  if(deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 
                  9, Z_DEFAULT_STRATEGY) != Z_OK){
    
    fprintf(stderr, "Error initializing deflation!\n");
    exit(1);
  }
}

int compress_file(int in_fd, int out_fd, struct zipentry *ze){
  Bytef in_buff[RDSZ];
  Bytef out_buff[RDSZ];
  unsigned int rdamt, wramt;
  unsigned long tr = 0;

  rdamt = 0;

  zs.avail_in = 0;
  zs.next_in = in_buff;
  
  zs.next_out = out_buff;
  zs.avail_out = (uInt)RDSZ;
  
  ze->crc = crc32(0L, Z_NULL, 0); 
  
  for(; ;){
    
    /* If deflate is out of input, fill the input buffer for it */
    if(zs.avail_in == 0 && zs.avail_out > 0){
      if((rdamt = read(in_fd, in_buff, RDSZ)) == 0)
        break;

      if(rdamt == -1){
        perror("read");
        exit(1);
      }
      
      /* compute the CRC while we're at it */
      ze->crc = crc32(ze->crc, in_buff, rdamt); 

      /* update the total amount read sofar */
      tr += rdamt;

      zs.next_in = in_buff;
      zs.avail_in = rdamt;
    }
    
    /* deflate the data */
    if(deflate(&zs, 0) != Z_OK){
      fprintf(stderr, "Error deflating! %s:%d\n", __FILE__, __LINE__);
      exit(1);
    }
    
    /* If the output buffer is full, dump it to disk */
    if(zs.avail_out == 0){

      if(write(out_fd, out_buff, RDSZ) != RDSZ){
        perror("write");
        exit(1);
      }

      /* clear the output buffer */
      zs.next_out = out_buff;
      zs.avail_out = (uInt)RDSZ;
    }

  }
  
  /* If we have any data waiting in the buffer after we're done with the file
     we can flush it */
  if(zs.avail_out < RDSZ){

    wramt = RDSZ - zs.avail_out;

    if(write(out_fd, out_buff, wramt) != wramt){
      perror("write");
      exit(1);
    }
    /* clear the output buffer */
    zs.next_out = out_buff;
    zs.avail_out = (uInt)RDSZ;
  }
  

  /* finish deflation.  This purges zlib's internal data buffers */
  while(deflate(&zs, Z_FINISH) == Z_OK){
    wramt = RDSZ - zs.avail_out;

    if(write(out_fd, out_buff, wramt) != wramt){
      perror("write");
      exit(1);
    }

    zs.next_out = out_buff;
    zs.avail_out = (uInt)RDSZ;
  }

  /* If there's any data left in the buffer, write it out */
  if(zs.avail_out != RDSZ){
    wramt = RDSZ - zs.avail_out;

    if(write(out_fd, out_buff, wramt) != wramt){
      perror("write");
      exit(1);
    }
  }

  /* update fastjar's entry information */
  ze->usize = (ub4)zs.total_in;
  ze->csize = (ub4)zs.total_out;

  /* Reset the deflation for the next time around */
  if(deflateReset(&zs) != Z_OK){
    fprintf(stderr, "Error resetting deflation\n");
    exit(1);
  }
  
  return 0;
}

void end_compression(){
  int rtval;

  /* Oddly enough, zlib always returns Z_DATA_ERROR if you specify no
     zlib header.  Go fig. */
  if((rtval = deflateEnd(&zs)) != Z_OK && rtval != Z_DATA_ERROR){
    fprintf(stderr, "Error calling deflateEnd\n");
    fprintf(stderr, "error: (%d) %s\n", rtval, zs.msg);
    exit(1);
  }
}


void init_inflation(){

  memset(&zs, 0, sizeof(z_stream));
    
  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  zs.opaque = Z_NULL;
  
  if(inflateInit2(&zs, -15) != Z_OK){
    fprintf(stderr, "Error initializing deflation!\n");
    exit(1);
  }

}

int inflate_file(pb_file *pbf, int out_fd, struct zipentry *ze){
  Bytef in_buff[RDSZ];
  Bytef out_buff[RDSZ];
  unsigned int rdamt;
  int rtval;
  ub4 crc = 0;

  zs.avail_in = 0;

  crc = crc32(crc, NULL, 0); /* initialize crc */

  /* loop until we've consumed all the compressed data */
  for(;;){
    
    if(zs.avail_in == 0){
      if((rdamt = pb_read(pbf, in_buff, RDSZ)) == 0)
        break;
      else if(rdamt < 0){
        perror("read");
        exit(1);
      }

#ifdef DEBUG
      printf("%d bytes read\n", rdamt);
#endif

      zs.next_in = in_buff;
      zs.avail_in = rdamt;
    }

    zs.next_out = out_buff;
    zs.avail_out = RDSZ;
    
    if((rtval = inflate(&zs, 0)) != Z_OK){
      if(rtval == Z_STREAM_END){
#ifdef DEBUG
        printf("end of stream\n");
#endif
        if(zs.avail_out != RDSZ){
          crc = crc32(crc, out_buff, (RDSZ - zs.avail_out));

          if(out_fd >= 0)
            if(write(out_fd, out_buff, (RDSZ - zs.avail_out)) != 
               (RDSZ - zs.avail_out)){
              perror("write");
              exit(1);
            }
        }
        
        break;
      } else {
        fprintf(stderr, "Error inflating file! (%d)\n", rtval);
        exit(1);
      }
    } else {
      if(zs.avail_out != RDSZ){
        crc = crc32(crc, out_buff, (RDSZ - zs.avail_out));

        if(out_fd >= 0)
          if(write(out_fd, out_buff, (RDSZ - zs.avail_out)) != 
             (RDSZ - zs.avail_out)){
            perror("write");
            exit(1);
          }
        zs.next_out = out_buff;
        zs.avail_out = RDSZ;
      }
    }
  }
#ifdef DEBUG
  printf("done inflating\n");
#endif

#ifdef DEBUG
  printf("%d bytes left over\n", zs.avail_in);
#endif

#ifdef DEBUG    
  printf("CRC is %x\n", crc);
#endif

  ze->crc = crc;
  
  pb_push(pbf, zs.next_in, zs.avail_in);

  ze->usize = zs.total_out;

  inflateReset(&zs);
  return 0;
}
