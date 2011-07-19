#ifndef NODE_BLEND_SRC_QUANTIZE_H
#define NODE_BLEND_SRC_QUANTIZE_H
/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

/*
 * derivations from pngquant and ppmquant
 *
 ** pngquant.c - quantize the colors in an alphamap down to a specified number
 **
 ** Copyright (C) 1989, 1991 by Jef Poskanzer.
 ** Copyright (C) 1997, 2000, 2002 by Greg Roelofs; based on an idea by
 **                                Stefan Schneider.
 **
 ** Permission to use, copy, modify, and distribute this software and its
 ** documentation for any purpose and without fee is hereby granted, provided
 ** that the above copyright notice appear in all copies and that both that
 ** copyright notice and this permission notice appear in supporting
 ** documentation.  This software is provided "as is" without express or
 ** implied warranty.
 */

typedef struct {
   unsigned char r,g,b,a;
} rgbaPixel;

typedef struct {
   unsigned char r,g,b;
} rgbPixel;

#define PAM_GETR(p) ((p).r)
#define PAM_GETG(p) ((p).g)
#define PAM_GETB(p) ((p).b)
#define PAM_GETA(p) ((p).a)
#define PAM_ASSIGN(p,red,grn,blu,alf) \
      do { (p).r = (red); (p).g = (grn); (p).b = (blu); (p).a = (alf); } while (0)
#define PAM_EQUAL(p,q) \
      ((p).r == (q).r && (p).g == (q).g && (p).b == (q).b && (p).a == (q).a)
#define PAM_DEPTH(newp,p,oldmaxval,newmaxval) \
      PAM_ASSIGN( (newp), \
            ( (int) PAM_GETR(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
            ( (int) PAM_GETG(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
            ( (int) PAM_GETB(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval), \
            ( (int) PAM_GETA(p) * (newmaxval) + (oldmaxval) / 2 ) / (oldmaxval) )


/* from pamcmap.h */

typedef struct acolorhist_item *acolorhist_vector;
struct acolorhist_item {
   rgbaPixel acolor;
   int value;
};

typedef struct acolorhist_list_item *acolorhist_list;
struct acolorhist_list_item {
   struct acolorhist_item ch;
   acolorhist_list next;
};

typedef acolorhist_list *acolorhash_table;

#define MAXCOLORS  32767

#define LARGE_NORM
#define REP_AVERAGE_PIXELS

typedef struct box *box_vector;
struct box {
   int ind;
   int colors;
   int sum;
};

static acolorhist_vector mediancut
(acolorhist_vector achv, int colors, int sum, unsigned char maxval, int newcolors);
static int redcompare (const void *ch1, const void *ch2);
static int greencompare (const void *ch1, const void *ch2);
static int bluecompare (const void *ch1, const void *ch2);
static int alphacompare (const void *ch1, const void *ch2);
static int sumcompare (const void *b1, const void *b2);

static acolorhist_vector pam_acolorhashtoacolorhist
(acolorhash_table acht, int maxacolors);
static acolorhist_vector pam_computeacolorhist
(rgbaPixel **apixels, int cols, int rows, int maxacolors, int* acolorsP);
static acolorhash_table pam_computeacolorhash
(rgbaPixel** apixels, int cols, int rows, int maxacolors, int* acolorsP);
static acolorhash_table pam_allocacolorhash (void);
static int pam_addtoacolorhash
(acolorhash_table acht, rgbaPixel *acolorP, int value);
static int pam_lookupacolor (acolorhash_table acht, rgbaPixel* acolorP);
static void pam_freeacolorhist (acolorhist_vector achv);
static void pam_freeacolorhash (acolorhash_table acht);


/*
 ** Here is the fun part, the median-cut colormap generator.  This is based
 ** on Paul Heckbert's paper, "Color Image Quantization for Frame Buffer
 ** Display," SIGGRAPH 1982 Proceedings, page 297.
 */

static acolorhist_vector mediancut
(acolorhist_vector achv, int colors, int sum, unsigned char maxval, int newcolors)
{
   acolorhist_vector acolormap;
   box_vector bv;
   register int bi, i;
   int boxes;

   bv = (box_vector) malloc( sizeof(struct box) * newcolors );
   acolormap =
         (acolorhist_vector) malloc( sizeof(struct acolorhist_item) * newcolors);
   if ( bv == (box_vector) 0 || acolormap == (acolorhist_vector) 0 ) {
      fprintf( stderr, "  out of memory allocating box vector\n" );
      fflush(stderr);
      exit(6);
   }
   for ( i = 0; i < newcolors; ++i )
      PAM_ASSIGN( acolormap[i].acolor, 0, 0, 0, 0 );

   /*
    ** Set up the initial box.
    */
   bv[0].ind = 0;
   bv[0].colors = colors;
   bv[0].sum = sum;
   boxes = 1;

   /*
    ** Main loop: split boxes until we have enough.
    */
   while ( boxes < newcolors ) {
      register int indx, clrs;
      int sm;
      register int minr, maxr, ming, mina, maxg, minb, maxb, maxa, v;
      int halfsum, lowersum;

      /*
       ** Find the first splittable box.
       */
      for ( bi = 0; bi < boxes; ++bi )
         if ( bv[bi].colors >= 2 )
            break;
      if ( bi == boxes )
         break;        /* ran out of colors! */
      indx = bv[bi].ind;
      clrs = bv[bi].colors;
      sm = bv[bi].sum;

      /*
       ** Go through the box finding the minimum and maximum of each
       ** component - the boundaries of the box.
       */
      minr = maxr = PAM_GETR( achv[indx].acolor );
      ming = maxg = PAM_GETG( achv[indx].acolor );
      minb = maxb = PAM_GETB( achv[indx].acolor );
      mina = maxa = PAM_GETA( achv[indx].acolor );
      for ( i = 1; i < clrs; ++i )
      {
         v = PAM_GETR( achv[indx + i].acolor );
         if ( v < minr ) minr = v;
         if ( v > maxr ) maxr = v;
         v = PAM_GETG( achv[indx + i].acolor );
         if ( v < ming ) ming = v;
         if ( v > maxg ) maxg = v;
         v = PAM_GETB( achv[indx + i].acolor );
         if ( v < minb ) minb = v;
         if ( v > maxb ) maxb = v;
         v = PAM_GETA( achv[indx + i].acolor );
         if ( v < mina ) mina = v;
         if ( v > maxa ) maxa = v;
      }

      /*
       ** Find the largest dimension, and sort by that component.  I have
       ** included two methods for determining the "largest" dimension;
       ** first by simply comparing the range in RGB space, and second
       ** by transforming into luminosities before the comparison.  You
       ** can switch which method is used by switching the commenting on
       ** the LARGE_ defines at the beginning of this source file.
       */
#ifdef LARGE_NORM
      if ( maxa - mina >= maxr - minr && maxa - mina >= maxg - ming && maxa - mina >= maxb - minb )
         qsort(
               (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
               alphacompare );
      else if ( maxr - minr >= maxg - ming && maxr - minr >= maxb - minb )
         qsort(
               (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
               redcompare );
      else if ( maxg - ming >= maxb - minb )
         qsort(
               (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
               greencompare );
      else
         qsort(
               (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
               bluecompare );
#endif /*LARGE_NORM*/
#ifdef LARGE_LUM
      {
         apixel p;
         float rl, gl, bl, al;

         PAM_ASSIGN(p, maxr - minr, 0, 0, 0);
         rl = PPM_LUMIN(p);
         PAM_ASSIGN(p, 0, maxg - ming, 0, 0);
         gl = PPM_LUMIN(p);
         PAM_ASSIGN(p, 0, 0, maxb - minb, 0);
         bl = PPM_LUMIN(p);

         /*
GRR: treat alpha as grayscale and assign (maxa - mina) to each of R, G, B?
     assign (maxa - mina)/3 to each?
     use alpha-fractional luminosity?  (normalized_alpha * lum(r,g,b))
        al = dunno ...
     [probably should read Heckbert's paper to decide]
          */

         if ( al >= rl && al >= gl && al >= bl )
            qsort(
                  (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                  alphacompare );
         else if ( rl >= gl && rl >= bl )
            qsort(
                  (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                  redcompare );
         else if ( gl >= bl )
            qsort(
                  (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                  greencompare );
         else
            qsort(
                  (char*) &(achv[indx]), clrs, sizeof(struct acolorhist_item),
                  bluecompare );
      }
#endif /*LARGE_LUM*/

      /*
       ** Now find the median based on the counts, so that about half the
       ** pixels (not colors, pixels) are in each subdivision.
       */
      lowersum = achv[indx].value;
      halfsum = sm / 2;
      for ( i = 1; i < clrs - 1; ++i )
      {
         if ( lowersum >= halfsum )
            break;
         lowersum += achv[indx + i].value;
      }

      /*
       ** Split the box, and sort to bring the biggest boxes to the top.
       */
      bv[bi].colors = i;
      bv[bi].sum = lowersum;
      bv[boxes].ind = indx + i;
      bv[boxes].colors = clrs - i;
      bv[boxes].sum = sm - lowersum;
      ++boxes;
      qsort( (char*) bv, boxes, sizeof(struct box), sumcompare );
   }

   /*
    ** Ok, we've got enough boxes.  Now choose a representative color for
    ** each box.  There are a number of possible ways to make this choice.
    ** One would be to choose the center of the box; this ignores any structure
    ** within the boxes.  Another method would be to average all the colors in
    ** the box - this is the method specified in Heckbert's paper.  A third
    ** method is to average all the pixels in the box.  You can switch which
    ** method is used by switching the commenting on the REP_ defines at
    ** the beginning of this source file.
    */
   for ( bi = 0; bi < boxes; ++bi ) {
#ifdef REP_CENTER_BOX
      register int indx = bv[bi].ind;
      register int clrs = bv[bi].colors;
      register int minr, maxr, ming, maxg, minb, maxb, mina, maxa, v;

      minr = maxr = PAM_GETR( achv[indx].acolor );
      ming = maxg = PAM_GETG( achv[indx].acolor );
      minb = maxb = PAM_GETB( achv[indx].acolor );
      mina = maxa = PAM_GETA( achv[indx].acolor );
      for ( i = 1; i < clrs; ++i )
      {
         v = PAM_GETR( achv[indx + i].acolor );
         minr = min( minr, v );
         maxr = max( maxr, v );
         v = PAM_GETG( achv[indx + i].acolor );
         ming = min( ming, v );
         maxg = max( maxg, v );
         v = PAM_GETB( achv[indx + i].acolor );
         minb = min( minb, v );
         maxb = max( maxb, v );
         v = PAM_GETA( achv[indx + i].acolor );
         mina = min( mina, v );
         maxa = max( maxa, v );
      }
      PAM_ASSIGN(
            acolormap[bi].acolor, ( minr + maxr ) / 2, ( ming + maxg ) / 2,
            ( minb + maxb ) / 2, ( mina + maxa ) / 2 );
#endif /*REP_CENTER_BOX*/
#ifdef REP_AVERAGE_COLORS
      register int indx = bv[bi].ind;
      register int clrs = bv[bi].colors;
      register long r = 0, g = 0, b = 0, a = 0;

      for ( i = 0; i < clrs; ++i )
      {
         r += PAM_GETR( achv[indx + i].acolor );
         g += PAM_GETG( achv[indx + i].acolor );
         b += PAM_GETB( achv[indx + i].acolor );
         a += PAM_GETA( achv[indx + i].acolor );
      }
      r = r / clrs;
      g = g / clrs;
      b = b / clrs;
      a = a / clrs;
      PAM_ASSIGN( acolormap[bi].acolor, r, g, b, a );
#endif /*REP_AVERAGE_COLORS*/
#ifdef REP_AVERAGE_PIXELS
      register int indx = bv[bi].ind;
      register int clrs = bv[bi].colors;
      register long r = 0, g = 0, b = 0, a = 0, sum = 0;

      for ( i = 0; i < clrs; ++i )
      {
         r += PAM_GETR( achv[indx + i].acolor ) * achv[indx + i].value;
         g += PAM_GETG( achv[indx + i].acolor ) * achv[indx + i].value;
         b += PAM_GETB( achv[indx + i].acolor ) * achv[indx + i].value;
         a += PAM_GETA( achv[indx + i].acolor ) * achv[indx + i].value;
         sum += achv[indx + i].value;
      }
      r = r / sum;
      if ( r > maxval ) r = maxval;        /* avoid math errors */
      g = g / sum;
      if ( g > maxval ) g = maxval;
      b = b / sum;
      if ( b > maxval ) b = maxval;
      a = a / sum;
      if ( a > maxval ) a = maxval;
      /* GRR 20001228:  added casts to quiet warnings; 255 DEPENDENCY */
      PAM_ASSIGN( acolormap[bi].acolor, (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a );
#endif /*REP_AVERAGE_PIXELS*/
   }

   /*
    ** All done.
    */
   free(bv);
   return acolormap;
}

static int
redcompare( const void *ch1, const void *ch2 )
{
   return (int) PAM_GETR( ((acolorhist_vector)ch1)->acolor ) -
         (int) PAM_GETR( ((acolorhist_vector)ch2)->acolor );
}

static int
greencompare( const void *ch1, const void *ch2 )
{
   return (int) PAM_GETG( ((acolorhist_vector)ch1)->acolor ) -
         (int) PAM_GETG( ((acolorhist_vector)ch2)->acolor );
}

static int
bluecompare( const void *ch1, const void *ch2 )
{
   return (int) PAM_GETB( ((acolorhist_vector)ch1)->acolor ) -
         (int) PAM_GETB( ((acolorhist_vector)ch2)->acolor );
}

static int
alphacompare( const void *ch1, const void *ch2 )
{
   return (int) PAM_GETA( ((acolorhist_vector)ch1)->acolor ) -
         (int) PAM_GETA( ((acolorhist_vector)ch2)->acolor );
}

static int
sumcompare( const void *b1, const void *b2 )
{
   return ((box_vector)b2)->sum -
         ((box_vector)b1)->sum;
}


/*===========================================================================*/


/* libpam3.c - pam (portable alpha map) utility library part 3
 **
 ** Colormap routines.
 **
 ** Copyright (C) 1989, 1991 by Jef Poskanzer.
 ** Copyright (C) 1997 by Greg Roelofs.
 **
 ** Permission to use, copy, modify, and distribute this software and its
 ** documentation for any purpose and without fee is hereby granted, provided
 ** that the above copyright notice appear in all copies and that both that
 ** copyright notice and this permission notice appear in supporting
 ** documentation.  This software is provided "as is" without express or
 ** implied warranty.
 */

/*
#include "pam.h"
#include "pamcmap.h"
 */

#define HASH_SIZE 20023

#define pam_hashapixel(p) ( ( ( (long) PAM_GETR(p) * 33023 + \
      (long) PAM_GETG(p) * 30013 + \
      (long) PAM_GETB(p) * 27011 + \
      (long) PAM_GETA(p) * 24007 ) \
      & 0x7fffffff ) % HASH_SIZE )

static acolorhist_vector pam_computeacolorhist
(rgbaPixel **apixels, int cols, int rows, int maxacolors, int* acolorsP)
{
   acolorhash_table acht;
   acolorhist_vector achv;

   acht = pam_computeacolorhash( apixels, cols, rows, maxacolors, acolorsP );
   if ( acht == (acolorhash_table) 0 )
      return (acolorhist_vector) 0;
   achv = pam_acolorhashtoacolorhist( acht, maxacolors );
   pam_freeacolorhash( acht );
   return achv;
}



static acolorhash_table pam_computeacolorhash
(rgbaPixel** apixels, int cols, int rows, int maxacolors, int* acolorsP)
{
   acolorhash_table acht;
   register rgbaPixel* pP;
   acolorhist_list achl;
   int col, row, hash;

   acht = pam_allocacolorhash( );
   *acolorsP = 0;

   /* Go through the entire image, building a hash table of colors. */
   for ( row = 0; row < rows; ++row )
      for ( col = 0, pP = apixels[row]; col < cols; ++col, ++pP )
      {
         hash = pam_hashapixel( *pP );
         for ( achl = acht[hash]; achl != (acolorhist_list) 0; achl = achl->next )
            if ( PAM_EQUAL( achl->ch.acolor, *pP ) )
               break;
         if ( achl != (acolorhist_list) 0 )
            ++(achl->ch.value);
         else
         {
            if ( ++(*acolorsP) > maxacolors )
            {
               pam_freeacolorhash( acht );
               return (acolorhash_table) 0;
            }
            achl = (acolorhist_list) malloc( sizeof(struct acolorhist_list_item) );
            if ( achl == 0 ) {
               fprintf( stderr, "  out of memory computing hash table\n" );
               exit(7);
            }
            achl->ch.acolor = *pP;
            achl->ch.value = 1;
            achl->next = acht[hash];
            acht[hash] = achl;
         }
      }

   return acht;
}



static acolorhash_table
pam_allocacolorhash( )
{
   acolorhash_table acht;
   int i;

   acht = (acolorhash_table) malloc( HASH_SIZE * sizeof(acolorhist_list) );
   if ( acht == 0 ) {
      fprintf( stderr, "  out of memory allocating hash table\n" );
      exit(8);
   }

   for ( i = 0; i < HASH_SIZE; ++i )
      acht[i] = (acolorhist_list) 0;

   return acht;
}



static int pam_addtoacolorhash
(acolorhash_table acht, rgbaPixel *acolorP, int value)
{
   register int hash;
   register acolorhist_list achl;

   achl = (acolorhist_list) malloc( sizeof(struct acolorhist_list_item) );
   if ( achl == 0 )
      return -1;
   hash = pam_hashapixel( *acolorP );
   achl->ch.acolor = *acolorP;
   achl->ch.value = value;
   achl->next = acht[hash];
   acht[hash] = achl;
   return 0;
}



static acolorhist_vector pam_acolorhashtoacolorhist
(acolorhash_table acht, int maxacolors)
{
   acolorhist_vector achv;
   acolorhist_list achl;
   int i, j;

   /* Now collate the hash table into a simple acolorhist array. */
   achv = (acolorhist_vector) malloc( maxacolors * sizeof(struct acolorhist_item) );
   /* (Leave room for expansion by caller.) */
   if ( achv == (acolorhist_vector) 0 ) {
      fprintf( stderr, "  out of memory generating histogram\n" );
      exit(9);
   }

   /* Loop through the hash table. */
   j = 0;
   for ( i = 0; i < HASH_SIZE; ++i )
      for ( achl = acht[i]; achl != (acolorhist_list) 0; achl = achl->next )
      {
         /* Add the new entry. */
         achv[j] = achl->ch;
         ++j;
      }

   /* All done. */
   return achv;
}



static int pam_lookupacolor (acolorhash_table acht, rgbaPixel* acolorP)
{
   int hash;
   acolorhist_list achl;

   hash = pam_hashapixel( *acolorP );
   for ( achl = acht[hash]; achl != (acolorhist_list) 0; achl = achl->next )
      if ( PAM_EQUAL( achl->ch.acolor, *acolorP ) )
         return achl->ch.value;

   return -1;
}



static void pam_freeacolorhist (acolorhist_vector achv)
{
   free( (char*) achv );
}



static void pam_freeacolorhash (acolorhash_table acht)
{
   int i;
   acolorhist_list achl, achlnext;

   for ( i = 0; i < HASH_SIZE; ++i )
      for ( achl = acht[i]; achl != (acolorhist_list) 0; achl = achlnext )
      {
         achlnext = achl->next;
         free( (char*) achl );
      }
   free( (char*) acht );
}

#endif
