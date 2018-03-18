/*
This allocator uses an implicit free list to allocate memory in the heap.

In this approach, each block has a header and footer describing its size and
whether it is allocated ot free. Aside from this, blocks only have payloads
if they are allocated or reassignable junk data if they are not.

A block is allocated by searching through the heap for the smallest free block
large enough to fit the requested size payload, then allocating the payload
to that block. If the block is too big, then the leftover data is reassigned
as a free block. If no free blocks of an adequate size are available, the
heap is simply grown by an amount large enough to hold the requested data size,
and that new area is used to allocate the data.
Blocks are freed by changing their header and footer to describe them as free
blocks. This allows the malloc function to use them to assign memory. In
addition, if a block is freed alongside another block, they are joined together
as one larger free block, so as to be able to fit larger blocks of memory.

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
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address on its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* given block ptr bp, compute address of next and prev blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define ALIGNMENT 8

#define ALIGNMENT_CHECK(p)  ((((unsigned int)(p)) % ALIGNMENT) == 0)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* helper function prototypes */
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static int mm_check_block(void *bp, int iterator);
static void cheese();
int mm_check();

//Searches for the smallest possible block to satisfy a malloc call
static void *find_fit(size_t asize){ //first fit, BAD
  void *bp;
  void *tiny = NULL;
  //Searches for the first free block that can satisfy the allocation
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
    if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
      tiny = bp;
      break;
    }
  }
  //Searches for blocks that could still satisfy the allocation while being
  //smaller
  while(GET_SIZE(HDRP(bp)) != 0){
    if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))
       && GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(tiny))){
         tiny = bp;
       }
     bp = NEXT_BLKP(bp);
  }
  //Returns the found block, or null if none are appropriate
  return tiny;
}

/*
Place is used to allocate blocks by malloc. It first ensures that the allocated
block isn't using more memory than it has to, splitting it into two if it is.
It then allocates the block with the needed memory, and, if applicable,
makes the remainder into a free block that can still be used.
*/
static void place(void *bp, size_t asize){
  size_t csize = GET_SIZE(HDRP(bp));
  //Checks whether all of the free block is needed for allocation
  if((csize - asize) >= (2*DSIZE)){
    //allocates the block
    PUT(HDRP(bp), PACK(asize,1));
    PUT(FTRP(bp), PACK(asize,1));
    //frees the remainder for other use
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  }
  //If all the memory is needed, uses it all
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

/*
Coalesce is used to ensure that the memory is available in the largest
blocks possible. It is used whenever a free block is created, and checks
whether adjacent blocks are free. If they are, it merges them with the newly
freed block to make a bigger free block.
*/
static void *coalesce(void *bp)
{
  //retrieve data
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  //If both neighboring blocks are allocated, does nothing
  if(prev_alloc && next_alloc){
    return bp;
  }
 //If only the following block is free, merges with it
  else if (prev_alloc && !next_alloc){
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
  }
  //If only the previous block is free, merges with it
  else if (!prev_alloc && next_alloc){
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size,0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
    bp = PREV_BLKP(bp);
  }
  //if both previous blocks are free, merges with both.
  else{
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  return bp;
}

static void *extend_heap(size_t words)
{
  char *bp;
  size_t size;

  //Allocate even number of words for ALIGNMENT
  size = (words % 2) ? ((words+1) * WSIZE) : (words * WSIZE);
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
    printf("initialization fails \n");
    return -1;
  }
  return 0;
}


/*
* mm_malloc - Allocate a block by searching for an appropriate free block
with find_fit, or making a new one if none is available using extend_heap
*/
void *mm_malloc(size_t size)
{
  size_t asize; /* Adjusted block Size */
  size_t extendsize; /* Amount to extend heap if no fit */
  char *bp;

//Initialize heap if not done so yet
  if(heap_listp == 0){
    mm_init();
  }

  /*Ignore spurious requests */
  if(size == 0){
    return NULL;
  }

  /* adjust block size to include overhead and alignment range */
  if (size <= DSIZE){
    asize = 2*DSIZE;
  }
  else{
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
  }

  /* search the free list for a fit */
  bp = find_fit(asize);
  if ((bp) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* no fit found. get more memory and place the block */
  extendsize = MAX(asize, CHUNKSIZE);
  bp = extend_heap(extendsize/WSIZE);
  if (bp == NULL){
    return NULL;
  }
  place(bp, asize);
  /* Uncomment to use checker.
  if(!mm_check){
    bp = NULL;
  }
  */
  return bp;

}

/*
* mm_free - Freeing a block will make the block header/footer reflect the
block's allocation as free, which allows malloc and realloc to use it as memory
*/
void mm_free(void *bp)
{
  if(bp == 0)
    return;

  size_t size = GET_SIZE(HDRP(bp));

  if (heap_listp == 0){
    mm_init();
  }
//re-label the data as free
  PUT(HDRP(bp), PACK(size,0));
  PUT(FTRP(bp), PACK(size,0));
  //coalesces in case the newly freed data has free data adjacent to it
  coalesce(bp);
}

/*
mm_realloc - Implemented largely in terms of mm_malloc and mm_free
Allocates the memory requested using mm_malloc, then copies the memory from the
designated pointed to the malloc-allocated pointer, and frees the original copy
using mm_free. If no pointer is given, the function simplifies to mm_malloc;
if no size is given, the function simply frees the given pointer.
*/

void *mm_realloc(void *ptr, size_t size)
{
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;
//If no size is requested, simply frees the original pointer
  if(size == 0){
    mm_free(ptr);
    return 0;
  }
//If the given pointer is null, simply allocates a block of the requested size
  if (ptr == NULL) {
    return mm_malloc(size);
  }
//If no pointer can be allocated for the requested size, returns zero
  newptr = mm_malloc(size);
  if (!newptr) {
    return 0;
  }
//Given a valid pointer and available size, copies the data stored in the
//area designated by the pointer to the malloc-allocated area, then frees
//the original copy of the data.
  copySize = GET_SIZE(HDRP(ptr));
  if (size < copySize)
  copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}
/*
static void cheese(){
  void *bp = NEXT_BLKP(heap_listp);
  void *other;
  while(GET_SIZE(HDRP(bp)) != 0){

    if(GET_ALLOC(HDRP(bp)) == 0){
      other = HDRP(NEXT_BLKP(bp));
      while(GET_SIZE(other) != 0){
        if(GET_ALLOC(other) = 1 && GET_SIZE(HDRP(other) <= GET_SIZE(HDRP(bp)))){
          mm_realloc(other, GET_SIZE(other));
        }
      }
    }
  }
}
*/
/*
//void mm_check;
mm_check goes through every block in the heap starting at the heap pointer.
It checks through every block in the heap by using the NEXT_BLKP sequentially
through each block, and detecting the beginning and end of the heap using its
prologue and epilogue, with the unique property that only they have a size of
zero. In each block, it checks for compliance with our invariants; if any
fail to comply, the return value is changed to 0, otherwise it returns one.
The function also prints a full description of our heap block-by-block, which
is helpful when debugging with small samples such as short1,2-bal.rep.
*/
int mm_check(void)
{
  void *bp = heap_listp;
  int iterator = 1;
  int works = 1;
  //Checks whether prologue is correctly defined
  if(GET_ALLOC(bp) != 0){
    printf("Prologue error");
    works = 0;
  }
  bp = NEXT_BLKP(bp);
//Search through each block
  while (GET_SIZE(HDRP(bp)) != 0){
    //checks whether block complies with invariants, see below
    if(mm_check_block(bp, iterator)){
      works = 0;
    }
    //Incrementor to check the following block
    bp = HDRP(NEXT_BLKP(bp));
    ++iterator;
  }
  return works;
}

//Checks whether single blocks follow invariants
//Helper function for mm_check_block
static int mm_check_block(void *bp, int iterator){
  int works = 1;
  printf("Block number %d\n", iterator);
  printf("Block size: %d\n", GET_SIZE(HDRP(bp)));
  printf("Block allocation: %d\n", GET_ALLOC(HDRP(bp)));
  //Check footer/header matching
  if(GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))){
    printf("ERROR: Non-matching footer/header, block %d\n", iterator);
    works = 0;
  }
  //Check for adjacent free blocks, as we immediate coalesce
  if(GET_ALLOC(HDRP(bp)) == 0 && GET_ALLOC(HDRP(NEXT_BLKP(bp))) == 0){
    printf("Lack of coalescing, block %d\n", iterator);
    works = 0;
  }
  //Check for alignment issues
  if(!ALIGNMENT_CHECK(HDRP(bp)) || !ALIGNMENT_CHECK(HDRP(bp))){
    printf("Unaligned block %d\n", iterator);
    works = 0;
  }
  //Check whether data leaks from heap or epilogue not implemented well
  if (mem_heap_hi() < bp) {
    printf("Memory max exceeded block %d\n", iterator);
    works = 0;
  }
  return works;
}
