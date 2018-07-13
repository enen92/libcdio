/*
  Copyright (C) 2004, 2005, 2006, 2008, 2009 Rocky Bernstein <rocky@gnu.org>
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Simple program to show using libiso9660 to extract a file from an
   ISO-9660 image.

   If a single argument is given, it is used as the ISO 9660 image to
   use in the extraction. Otherwise a compiled in default ISO 9660 image
   name (that comes with the libcdio distribution) will be used.
 */

/* This is the ISO 9660 image. */
#define ISO9660_IMAGE_PATH "../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/copying.iso"

#define LOCAL_FILENAME "copying"

/* portable.h has to come first else _FILE_OFFSET_BITS are redefined in
   say opensolaris. */
#include "portable.h"

#include <cdio/cdio.h>
#include <cdio/iso9660.h>

#include <stdio.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#define CEILING(x, y) ((x+(y-1))/y)

#define my_exit(rc)				\
  fclose (p_outfd);				\
  iso9660_statv2_free(p_statbuf);		\
  iso9660_close(p_iso);				\
  return rc;					\

int
main(int argc, const char *argv[])
{
  iso9660_statv2_t *p_statbuf;
  FILE *p_outfd;
  int j;
  char const *psz_image;
  char const *psz_fname;
  char const *psz_target;
  iso9660_t *p_iso;
  uint32_t num_extents;
  iso9660_extent_descr_t *extents;

  if (argc > 4) {
    printf("usage %s [ISO9660-image.ISO [ISO-filename [Local-filename]]]\n", argv[0]);
    printf("Extracts filename from ISO-9660-image.ISO\n");
    return 1;
  }
  
  if (argc > 1) 
    psz_image = argv[1];
  else 
    psz_image = ISO9660_IMAGE;

  if (argc > 2) 
    psz_fname = argv[2];
  else 
    psz_fname = LOCAL_FILENAME;

  if (argc > 3) 
    psz_target = argv[3];
  else 
    psz_target = psz_fname;

  p_iso = iso9660_open (psz_image);
  
  if (NULL == p_iso) {
    fprintf(stderr, "Sorry, couldn't open ISO 9660 image %s\n", psz_image);
    return 1;
  }

  p_statbuf = iso9660_ifs_statv2_translate (p_iso, psz_fname);

  if (NULL == p_statbuf) 
    {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file %s\n",
	      psz_fname);
      iso9660_close(p_iso);
      return 2;
    }

  if (!(p_outfd = fopen (psz_target, "wb")))
    {
      perror ("fopen()");
      iso9660_statv2_free(p_statbuf);
      iso9660_close(p_iso);
      return 3;
    }

  /* Copy the blocks from the ISO-9660 filesystem to the local filesystem. */

  num_extents = iso9660_statv2_get_extents(p_statbuf, &extents);

  /* The following alternatives demonstrate the normal case that multi-extent
     files are stored as a contiguous string of bytes, and the rather exotic
     case that the extents are not stored seamlessly in sequence.
     The call iso9660_statv2_has_extent_gaps() tells which situation is found.
   */

  if (!iso9660_statv2_has_extent_gaps(p_statbuf)) {

    /* If the program gets here, then the normal and simple situation is given.

       The following read procedure is very similar to the read procedure with
       legacy iso9660_stat_t. The only differences are the use of
         extents[0].lsn                   instead of  p_statv1->lsn
         iso9660_statv2_get_total_size()  instead of  p_statv1->size

       So if it is cumbersome to adapt the read procedures of an application
       to the new multi-extent capabilities of iso9660_statv2_t, then
       consider to make only this small change and to bail out with error if
       iso9660_statv2_has_extent_gaps() returns "true".
     */

    uint64_t to_write;
    size_t write_now;
    lsn_t lsn = extents[0].lsn;
    char buf[ISO_BLOCKSIZE];

    to_write = iso9660_statv2_get_total_size(p_statbuf);
    while (to_write > 0) {

      memset (buf, 0, ISO_BLOCKSIZE);

      if ( ISO_BLOCKSIZE != iso9660_iso_seek_read (p_iso, buf, lsn, 1) ) {
        fprintf(stderr, "Error reading ISO 9660 file %s at LSN %lu\n",
                psz_fname, (long unsigned int) lsn);
        my_exit(4);
      }

      write_now = to_write > ISO_BLOCKSIZE ? ISO_BLOCKSIZE : to_write;
      fwrite (buf, write_now, 1, p_outfd);
      if (ferror (p_outfd)) {
        perror ("fwrite()");
        my_exit(5);
      }

      to_write -= write_now;
      lsn++;
    }

  } else {

    /* If program execution gets here, then the file cannot be read in one
       sequential sweep.
       Of course the following read procedure is suitable for the normal case
       too.
     */
    fprintf(stderr, "Note: Found multi-extent file with gaps. Will apply suitable read procedure.\n");

    for (j = 0; j < num_extents; j++) {
      uint32_t to_write;
      size_t write_now;
      lsn_t lsn = extents[j].lsn;

      to_write = extents[j].size;
      while (to_write > 0) {
        char buf[ISO_BLOCKSIZE];

        memset (buf, 0, ISO_BLOCKSIZE);

        if ( ISO_BLOCKSIZE != iso9660_iso_seek_read (p_iso, buf, lsn, 1) ) {
          fprintf(stderr, "Error reading ISO 9660 file %s at LSN %lu\n",
          psz_fname, (long unsigned int) lsn);
          my_exit(4);
        }

        write_now = to_write > ISO_BLOCKSIZE ? ISO_BLOCKSIZE : to_write;
        fwrite (buf, write_now, 1, p_outfd);

	if (ferror (p_outfd)) {
          perror ("fwrite()");
          my_exit(5);
        }

        to_write -= write_now;
        lsn++;
      }
    }

  }

  fflush (p_outfd);

  printf("Extraction of file '%s' from %s successful.\n", 
	 psz_fname, psz_image);

  my_exit(0);
}
