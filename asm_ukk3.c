/*
    Copyright (C) 2015 Curoverse, Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm_ukk.h"

static int g_verbose=0;
static int g_debug=0;

int align_W3(char **X, char **Y, char *a, char *b, int *W, int m_r, int n_c, int w_len, int (*score_func)(char, char), char gap_char) {
  int i;
  int dr, dc;
  int r, c, w;
  int pos00, pos01, pos10, pos11;
  int w_offset;
  int mm;
  int xy_pos=0;
  char ch;

  char *tx, *ty;

  i = ((n_c>m_r)?n_c:m_r);

  *X = NULL;
  *Y = NULL;

  tx = (char *)malloc(sizeof(char)*2*i);
  ty = (char *)malloc(sizeof(char)*2*i);

  tx[2*i-1] = '\0';
  ty[2*i-1] = '\0';

  w_offset = w_len/2;

  r = m_r-1;
  c = n_c-1;
  while ((r>0) || (c>0)) {
    dr = 0;
    dc = 0;


    w = c - (r-w_offset);
    pos11 = r*w_len + w;

    // The preference is for straight alignment, followed by column
    // alignment followed by row alignment.
    // The precedence is last to first
    //

    if (r>0) {
      w = c - ((r-1)-w_offset);
      if ((w>=0) && (w<w_len)) {
        pos01 = (r-1)*w_len + w;
        //if ((W[pos01]+gap) == W[pos11]) { dr=-1; dc=0; }
        if ((W[pos01]+score_func(0,b[r-1])) == W[pos11]) { dr=-1; dc=0; }
        //if ((W[pos01]+score_func(a[c-1],0)) == W[pos11]) { dr=-1; dc=0; }
      }
    }

    if (c>0) {
      w = (c-1) - (r-w_offset);
      if ((w>=0) && (w<w_len)) {
        pos10 = r*w_len + w;
        //if ((W[pos10]+gap) == W[pos11]) { dr=0; dc=-1; }
        if ((W[pos10]+score_func(a[c-1],0)) == W[pos11]) { dr=0; dc=-1; }
        //if ((W[pos10]+score_func(0,b[r-1])) == W[pos11]) { dr=0; dc=-1; }
      }
    }

    if ((r>0) && (c>0)) {
      w = (c-1) - ((r-1)-w_offset);
      if ((w>=0) && (w<w_len)) {
        pos00 = (r-1)*w_len + w;
        //mm = ((a[c-1]==b[r-1])?0:mismatch);
        mm = score_func(a[c-1], b[r-1]);
        if ((W[pos00]+mm) == W[pos11]) { dr=-1; dc=-1; }
      }
    }

    if ((dr==-1) && (dc==-1)) {
      tx[xy_pos] = a[c-1];
      ty[xy_pos] = b[r-1];
    } else if ((dr==-1) && (dc==0)) {
      tx[xy_pos] = gap_char;
      ty[xy_pos] = b[r-1];
    } else if ((dr==0) && (dc==-1)) {
      tx[xy_pos] = a[c-1];
      ty[xy_pos] = gap_char;
    } else {
      free(tx);
      free(ty);

      //DEBUG
      //printf("BANG!\n");
      //fflush(stdout);

      return -1;
    }

    xy_pos++;
    r+=dr;
    c+=dc;
  }

  tx[xy_pos]='\0';
  ty[xy_pos]='\0';

  for (i=0; i<(xy_pos/2); i++) {
    ch = tx[i]; tx[i] = tx[xy_pos-i-1]; tx[xy_pos-i-1] = ch;
    ch = ty[i]; ty[i] = ty[xy_pos-i-1]; ty[xy_pos-i-1] = ch;
  }

  *X = tx;
  *Y = ty;

  return 0;
}

int asm_ukk_score3(char *a, char *b, int (*score_func)(char, char)) {
  int threshold = (1<<2);
  int it, max_it=(1<<(32-2-1));
  int sc = -2;

  for (it=0; (it<max_it) && (sc<0); it++) {
    sc = sa_align_ukk3(NULL, NULL, a, b, threshold, score_func, '-');
    threshold*=2;
  }

  return sc;
}

int asm_ukk_align3(char **X, char **Y, char *a, char *b, int (*score_func)(char, char), char gap_char) {
  int threshold = (1<<2);
  int it, max_it=(1<<(32-2-1));
  int sc = -2;

  if ((X!=NULL) && (Y!=NULL)) {
    *X = NULL;
    *Y = NULL;
  }

  for (it=0; (it<max_it) && (sc<0); it++) {

    if (g_verbose) { printf("# threshold %d\n", threshold); }

    sc = sa_align_ukk3(X, Y, a, b, threshold, score_func, gap_char);
    threshold*=2;

    if (sc<0) {
      if ((X!=NULL) && (Y!=NULL)) {
        if (*X) free(*X);
        if (*Y) free(*Y);
        *X = NULL;
        *Y = NULL;
      }
    }
  }

  return sc;
}

int sa_align_ukk3(char **X, char **Y, char *a, char *b, int T, int (*score_func)(char, char), char gap_char) {
  int ret;
  int r,c, n_c, m_r, len_ovf;
  int *W, w, w_offset, w_len;
  int p, del, m;
  int create_align_seq = 0;

  n_c = strlen(a)+1;
  m_r = strlen(b)+1;

  // Find minimum non-zero score for
  // window band space allocation.
  //
  del = score_func(-1,-1);
  if (del<=0) { return -1; }

  if (X && Y) { create_align_seq = 1; }

  if (create_align_seq) {
    *X = NULL;
    *Y = NULL;
  }

  // t/del < |n-m| -> reject
  //
  len_ovf = ((n_c>m_r) ? (n_c-m_r) : (m_r-n_c));
  if ((T/del) < len_ovf) {
    if (create_align_seq) {
      if (!(*X)) free(*X);
      if (!(*Y)) free(*Y);
    }
    return -1;
  }

  p = (T/del) - len_ovf;
  p /= 2;

  w_offset = ((n_c>m_r) ? (n_c-m_r+p) : p);
  w_len = 2*w_offset+1;

  // our window isn't big enough to hold calculated values
  //
  w = (n_c-1) - ((m_r-1)-w_offset);
  if ((w<0) || (w>=w_len)) {
    if (create_align_seq) {
      if (!(*X)) free(*X);
      if (!(*Y)) free(*Y);
    }
    return -1;
  }

  //DEBUG
  //int i;
  //printf("!!!!! m_r %d, w_len %d\n", m_r, w_len);
  //fflush(stdout);


  W = (int *)malloc(sizeof(int)*m_r*w_len);

  for (w=0; w<w_len; w++) {
    c = w-w_offset;

    if (w<w_offset) { W[w] = -1; }
    else { W[w] = 2*(w-w_offset); }

    /*
    else if (w==w_offset) { W[w] = 0; }
    else if (c<n_c) { W[w] = W[w-1] + score_func(a[c],0); }
    else { W[w] = W[w-1] + score_func(0,0); }
    */

  }

  /*
  for (c=1; c<n_c; c++) {
    w = c - (1-w_offset);
    if (c==1) { W[w]=0; }
    else if (c<n_c) { W[w] = W[w-1] + score_func(a[c-1],0); }
    else { W[w] = W[w-1] + score_func(0,0); }
  }
  */

  //DEBUG
  //printf(">>>INIT\n");
  //for (i=0; i<w_len; i++) { printf("%d ", W[i]); }
  //printf("\n");
  //fflush(stdout);

  for (r=1; r<m_r; r++) {

    // For conceptual simplicity, enumerate columns
    //
    for (c=(r-w_offset); c<=(r+w_offset); c++) {


      // Window position
      //
      w = c - (r-w_offset);
      int w_rmm = c - ((r-1)-w_offset);

      if (c<0) { W[r*w_len + w] = -1; }

      //else if (c==0) { W[r*w_len + w] = r*2; }
      //else if (c==0) { W[r*w_len + w] = W[(r-1)*w_len + w_rmm] + 2; }
      else if (c==0) { W[r*w_len + w] = W[(r-1)*w_len + w_rmm] + score_func(0,b[r-1]); }
      //else if (c==0) { W[r*w_len + w] = W[(r-1)*w_len + w] + score_func(0, b[r-1]); }

      else if (c>=n_c) { W[r*w_len + w] = -1; }
      else {

        // diagonal value
        //
        //m = W[(r-1)*w_len + w] + ((a[c-1]==b[r-1]) ? 0 : mismatch);
        m = W[(r-1)*w_len + w] + score_func(a[c-1],b[r-1]) ;


        // left to right transition
        //
        //if ((w>0) && ((W[r*w_len+w-1] + gap) < m)) { m = W[r*w_len+w-1] + gap; }
        //if ((w>0) && ((W[r*w_len+w-1] + score_func(a[c-1], 0)) < m)) { m = W[r*w_len+w-1] + score_func(a[c-1], 0); }
        if ((w>0) && ((W[r*w_len+w-1] + score_func(0,b[r-1])) < m)) { m = W[r*w_len+w-1] + score_func(0,b[r-1]); }


        // top to bottom transition
        //
        if ((w+1)!=w_len) {
          //if ((W[(r-1)*w_len+w+1] + gap) < m) { m = W[(r-1)*w_len+w+1] + gap; }
          //if ((W[(r-1)*w_len+w+1] + score_func(0,b[r-1])) < m) { m = W[(r-1)*w_len+w+1] + score_func(0,b[r-1]); }
          if ((W[(r-1)*w_len+w+1] + score_func(a[c-1],0)) < m) { m = W[(r-1)*w_len+w+1] + score_func(a[c-1],0); }
        }

        W[r*w_len+w] = m;
      }

    }
  }

  w = (n_c-1) - ((m_r-1)-w_offset);
  m = W[(m_r-1)*w_len + w];

  if (create_align_seq) {
    //ret = align_W2(X, Y, a, b, W, m_r, n_c, w_len, mismatch, gap, gap_char);
    ret = align_W3(X, Y, a, b, W, m_r, n_c, w_len, score_func, gap_char);
    if (ret<0) { return ret; }
  }

  free(W);

  if (m>T) {
    if (create_align_seq) {
      if (!(*X)) free(*X);
      if (!(*Y)) free(*Y);
    }
    return -1;
  }

  return m;
}
