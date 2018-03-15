/*
* mm-naive.c - The fastest, least memory-efficient malloc package.
*
* In this naive approach, a block is allocated by simply incrementing
* the brk pointer.  A block is pure payload. There are no headers or
* footers.  Blocks are never coalesced or reused. Realloc is
* implemented directly using mm_malloc and mm_free.
*
* NOTE TO STUDENTS: Replace this header comment with your own header
* comment that gives a high level description of your solution.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
* NOTE TO STUDENTS: Before you do anything else, please
* provide your team information in the following struct.
********************************************************/

team_t team = {
  /* Team name */
  "Group 6",
  /* First member's full name */
  "Chloe Mueller",
  /* First member's email address */
  "chloemueller2020@u.northwestern.edu",
  /* Second member's full name (leave blank if none) */
  "Jose Trejos",
  /* Second member's email address (leave blank if none) */
  "josetrejos2019@u.northwestern.edu"
};


static char *mem_heap; // Points to first byte of heap
static char *mem_brk; //Points to last byte of heap +1
static char *mem_max_addr; //Max Legal Address +1
static char *heap_listp; //Points to Prologue block

/* basic constants and macros*/
#define WSIZE 4   /* word header/footer size (bytes) */
#define DSIZE 8   /* double word size (bytes) */
#define CHUNKSIZE (1<<12) /* extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* pack a size and allocated bit into word*/
#define PACK(size, alloc) ((size) | (alloc))

/* read and write a word at address p*/
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & -0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address on its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* given block ptr bp, compute address of next and prev blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *find_fit(size_t asize){ //first fit, BAD
  void *bp;

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
    if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
      return bp;
    }
  }
  return NULL;
}
/*
static void *find_best_fit(size_t asize){
void *bp;


for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
if(!GET_ALLOC(HDRP(bp)) && (asize == GET_SIZE(HDRP(bp)))){
return bp;
}
else {

}
}
}

*/

static void place(void *bp, size_t asize){
  size_t csize = GET_SIZE(HDRP(bp));
  if((csize - asize) >= (2*DSIZE)){
    PUT(HDRP(bp), PACK(asize,1));
    PUT(FTRP(bp), PACK(asize,1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(FTRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if(prev_alloc && next_alloc){
    return bp;
  }

  else if (prev_alloc && !next_alloc){
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
  }
  else if (!prev_alloc && next_alloc){
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size,0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
    bp = PREV_BLKP(bp);
  }

  else{
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  return bp;
}
/*
void *mem_sbrk(int incr)
{
char *old_brk = mem_brk;

if((incr < 0) || ((mem_brk + incr) > mem_max_addr)){
errno = ENOMEM;
fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
return (void *)-1;
}
mem_brk += incr;
return (void *)old_brk;
}
*/
static void *extend_heap(size_t words)
{
  char *bp;
  size_t size;

  //Allocate even number of words for ALIGNMENT
  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
  if((long)(bp = mem_sbrk(size)) == -1){
    return NULL;
  }
  PUT(HDRP(bp), PACK(size, 0)); //Free block header
  PUT(FTRP(bp), PACK(size,0)); //free block footer
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); //new epilogue header

  //Coalesce if previous block was Free
  return coalesce(bp);
}
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/*
* mm_init - initialize the malloc package.
*/
int mm_init(void)
{
  //create initial empty heap
  if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
    return -1;
  }
  PUT(heap_listp, 0); //alignment padding
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); //Prologue padding
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); //Prologue footer
  PUT(heap_listp + (3*WSIZE), PACK(0,1)); //Prologue header
  heap_listp += (2*WSIZE);
  //Extend empty heap with CHUNKSIZE bytes
  if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
    return -1;
  }
  mm_check();
  return 0;
}


/*
* mm_malloc - Allocate a block by incrementing the brk pointer.
*     Always allocate a block whose size is a multiple of the alignment.
*/
void *mm_malloc(size_t size)
{
  mm_check();
  size_t asize; /* Adjusted block Size */
  size_t extendsize; /* Amount to extend heap if no fit */
  char *bp;

  /*Ignore spurious requests */
  if(size == 0)
  return NULL;

  /* adjust block size to include overhead and alignment range */
  if (size <= DSIZE)
  {
    asize = 2*DSIZE;
  }
  else
  {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
  }

  /* search the free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* no fit found. get more memory and place the block */
  extendsize = MAX(asize,CHUNKSIZE);
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
  return NULL;
  place(bp, asize);
  mm_check();
  return bp;

}

/*
* mm_free - Freeing a block does nothing.
*/
void mm_free(void *bp)
{
  size_t size = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(size,0));
  PUT(FTRP(bp), PACK(size,0));
  coalesce(bp);
}

/*
mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *ptr, size_t size)
{
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
  return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize)
  copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

/*
void mm_check(void)
{
  void *iterator = mem_heap_lo();
  while (iterator != mem_heap_hi()){
    //What happens whenever we find an allocated block
    if(GET_ALLOC(iterator) == 1){
      iterator = FTRP(iterator);

      if(GET_ALLOC(iterator) != 1){
        printf("Error: Footer and header unequal");
        printf("%s\n", __func__);
        return;
      }
      if(iterator > mem_heap_hi()){
        printf("Error: Max Memory exceeded");
        printf("%s\n", __func__);
        return;
      }
      iterator += 4;
    }

    if(GET_ALLOC(iterator) == 0){
      iterator = FTRP(iterator);
      if(GET_ALLOC(iterator) != 0){
        printf("Error: Footer and header unequal");
        printf("%s\n", __func__);
        return;
      }
      if(GET_ALLOC(PREV_BLKP(iterator) == 0 ||
      GET_ALLOC(NEXT_BLKP(iterator)) == 0){
        printf("Error: Coalesce failed");
        printf("%s\n", __func__);
        return;
      }
      if(iterator > mem_heap_hi()){
        printf("Error: Max Memory exceeded");
        printf("%s\n", __func__);
        return;
      }
      iterator += 4;
    }
  }
  printf("all good");
  printf("%s\n", __func__);

}
*/
