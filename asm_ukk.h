#ifndef ASM_UKK_H
#define ASM_UKK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASM_UKK_MISMATCH 3
#define ASM_UKK_GAP 2

typedef struct asm_ukk_type {
  char *SeqA, *SeqB;
  char *AlignA, *AlignB;
  int Threshold;
  int Score;
  int *W;
  int w_len;
  int n_r, n_c;

  int MismatchCost, GapCost;
} asm_ukk_t;

asm_ukk_t AsmUkkDefaultContext;

int asm_ukk_score(char *, char *);
int asm_ukk_score2(char *, char *, int, int);

int asm_ukk_align(char **, char **, char *, char *);
int asm_ukk_align2(char **X, char **Y, char *a, char *b, int mismatch, int gap, char gap_char);


int sa_align_ukk(char **, char **, char *, char *, int);
int sa_align_ukk2(char **, char **, char *, char *, int, int, int, char);

#endif
