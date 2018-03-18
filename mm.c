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

static char *heap_listp; //Points to Prologue block
static char *free_listp; // Points to free list

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

/* compute address of next and prev free blocks */
#define NEXT_FREEBLK(bp) (*(char **)((char *)(bp) + DSIZE))
#define PREV_FREEBLK(bp) (*(char **)((char *)(bp)))


#define SET_NEXT_PTR(bp) ((char *)(bp) + DSIZE)
#define SET_PREV_PTR(bp) (char *)(bp)
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* helper function prototypes */
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);
static void *extend_heap(size_t words);

/* add/remove from free list */
static void add_freeblk(char *bp);
static void rmv_freeblk(void *bp);



static void add_freeblk(char *bp){
  //(void *) copy = free_listp;
  /*PREV_FREEBLK(free_listp) = bp;
  NEXT_FREEBLK(bp) = free_listp;
  PREV_FREEBLK(bp) = NULL;*/
  /*
  NEXT_FREEBLK(bp) = free_listp;
  PREV_FREEBLK(bp) = NULL;
  PREV_FREEBLK(free_listp) = bp;
  free_listp = bp;*/
  char *copy = HDRP(bp);
  bp = SET_PREV_PTR(bp);
  NEXT_FREEBLK(bp) = free_listp;
  PREV_FREEBLK(bp) = NULL;
  if(free_listp != NULL){
    PREV_FREEBLK(free_listp) = bp;
  }
  free_listp = bp;
  if(HDRP(bp) != copy){
    printf("FUCK");
  }
}

static void rmv_freeblk(void *bp){
  // if at the front of the free list
  if(GET_ALLOC(HDRP(bp)) == 1){
    printf("BAD RMV CALL");
  }
  // remove first block
  if((PREV_FREEBLK(bp) == NULL) && (NEXT_FREEBLK(bp))) {
    free_listp = NEXT_FREEBLK(bp);
    PREV_FREEBLK(NEXT_FREEBLK(bp)) = NULL;
  }
  // remove only block in the list
  else if((PREV_FREEBLK(bp) == NULL) && (NEXT_FREEBLK(bp) == NULL)){
    free_listp = NULL;
  }
  // remove last block
  else if((PREV_FREEBLK(bp)) && (NEXT_FREEBLK(bp) == NULL)){
    NEXT_FREEBLK(PREV_FREEBLK(bp)) = NULL;
  }
  // remove block in middle
  else if((PREV_FREEBLK(bp)) && (NEXT_FREEBLK(bp))) {
    PREV_FREEBLK(NEXT_FREEBLK(bp)) = PREV_FREEBLK(bp);
    NEXT_FREEBLK(PREV_FREEBLK(bp)) = NEXT_FREEBLK(bp);
  }
}



static void *find_fit(size_t asize){ //first fit, BAD
  void *bp;
  //JOSE: CHANGING CONDITIONAL
  for (bp = free_listp; bp != NULL; bp = NEXT_FREEBLK(bp)){
    if(GET_SIZE(HDRP(bp)) >= asize){
      return HDRP(bp);
    }
  }
  return NULL;
}

/*
static void *find_fit(size_t asize){
void *bp;
char *minbp;
size_t min_size = GET_SIZE(HDRP(bp));

// if they're equal, return
for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
if(!GET_ALLOC(HDRP(bp)) && (asize == GET_SIZE(HDRP(bp)))){
return bp;
}
// if not equal, find the smallest spot
if(!GET_ALLOC(HDRP(bp)) && (asize < GET_SIZE(HDRP(bp)))){
if (min_size > GET_SIZE(HDRP(bp))) {
min_size = GET_SIZE(HDRP(bp));
*minbp = bp;
}
}
}
if(min_size != GET_SIZE(HDRP(bp))){
*(char *)bp = *minbp;
return bp;
}

return NULL;
}
*/

static void place(void *bp, size_t asize){
  size_t csize = GET_SIZE(HDRP(bp));
  if((csize - asize) >= (2*DSIZE)){
    if(GET_ALLOC(HDRP(bp)) == 0){
      rmv_freeblk(bp);
    }
    PUT(HDRP(bp), PACK(asize,1));
    PUT(FTRP(bp), PACK(asize,1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  }
  else {
    if(GET_ALLOC(HDRP(bp)) == 0){
      rmv_freeblk(bp);
    }
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if(prev_alloc && next_alloc){
    return bp;
  }

  else if (prev_alloc && !next_alloc){
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    rmv_freeblk(NEXT_BLKP(bp));
  }
  else if (!prev_alloc && next_alloc){
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size,0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
    rmv_freeblk(bp);
    bp = PREV_BLKP(bp);
  }

  else{
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    rmv_freeblk(PREV_BLKP(bp));
    rmv_freeblk(NEXT_BLKP(bp));
    bp = PREV_BLKP(bp);
     // chloe has this below, above return
  }
  add_freeblk(bp);
  return bp;
}
/*
void *mem_sbrk(int incr)
{
char *old_brk = mem_brk;

if((incr < 0) || ((mem_brk + incr) > mem_heap_hi())){
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
  size = (words % 2) ? ((words+1) * WSIZE) : (words * WSIZE);
  size = MAX(size, 24);
  if((long)(bp = mem_sbrk(size)) == -1){
    return NULL;
  }
  PUT(HDRP(bp), PACK(size, 0)); //Free block header
  PUT(FTRP(bp), PACK(size,0)); //free block footer
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); //new epilogue header
  add_freeblk(bp);
  //Coalesce if previous block was Free
  return SET_PREV_PTR(coalesce(bp));
}
/* single word (4) or double word (8) alignment */



/*
* mm_init - initialize the malloc package.
*/
int mm_init(void)
{
  //create initial empty heap
  if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
    printf("HERE???");
    return -1;
  }

  PUT(heap_listp, 0); //alignment padding
  PUT(heap_listp + WSIZE, PACK(DSIZE, 1)); //Prologue padding
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); //Prologue footer
  PUT(heap_listp + (3*WSIZE), PACK(0,1)); //Prologue header
  heap_listp += (2*WSIZE);
  free_listp = NULL;
  //free_listp = (heap_listp + DSIZE);
  //Extend empty heap with CHUNKSIZE bytes
  if((free_listp = extend_heap(CHUNKSIZE/WSIZE)) == NULL){ // ???
    printf("initialization fails \n");
    return -1;
  }
  //print_heap();
  print_heap();
  return 0;
}


/*
* mm_malloc - Allocate a block by incrementing the brk pointer.
*     Always allocate a block whose size is a multiple of the alignment.
*/
void *mm_malloc(size_t size)
{
  size_t asize; /* Adjusted block Size */
  size_t extendsize; /* Amount to extend heap if no fit */
  char *bp;


  if(heap_listp == 0){
    mm_init();
  }

  /*Ignore spurious requests */
  if(size == 0){
    return NULL;
  }

  /* adjust block size to include overhead and alignment range */
  if (size <= DSIZE){
    asize = 4*DSIZE;
  }
  else{
    asize = MAX(DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE), 4*DSIZE);
  }

  /* search the free list for a fit */
  bp = find_fit(asize);
  if (bp != NULL) {
    place(bp, asize);
    return bp;
  }

  /* no fit found. get more memory and place the block */
  extendsize = MAX(asize, CHUNKSIZE);
  bp = (extend_heap(extendsize/WSIZE));
  if (bp == NULL){
    return NULL;
  }
  place(bp, asize);
  return bp;
}

/*
* mm_free - Freeing a block does nothing.
*/
void mm_free(void *bp)
{
  /*if(bp == 0)
  return;

  size_t size = GET_SIZE(HDRP(bp));

  if (heap_listp == 0){
  mm_init();
}

PUT(HDRP(bp), PACK(size,0));
PUT(FTRP(bp), PACK(size,0));
//printf("%s\n", __func__);
coalesce(bp);*/
if(bp == 0)
return;

void *oldfree = free_listp;
free_listp = bp;
size_t size = GET_SIZE(HDRP(bp));



NEXT_FREEBLK(free_listp) = oldfree;
PUT(HDRP(bp), PACK(size,0));
PUT(FTRP(bp), PACK(size,0));
coalesce(bp);
print_heap(heap_listp);
}

/*
mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/

void *mm_realloc(void *ptr, size_t size)
{
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  /* if size is 0, return NULL */
  if(size == 0){
    mm_free(ptr);
    return 0;
  }

  if (ptr == NULL) {
    return mm_malloc(size);
  }

  newptr = mm_malloc(size);
  if (!newptr) {
    return 0;
  }

  copySize = GET_SIZE(HDRP(ptr));
  if (size < copySize)
  copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

/*
void m_check(void *bp){
if (bp > mem_heap_hi())
printf("Found:");
};
*/


/* FAKE CHECKER */

static void print_block(void *bp) {
printf("\tp: %p; ", bp);
printf("allocated: %s; ", GET_ALLOC(HDRP(bp))? "yes": "no" );
printf("hsize: %d; ", GET_SIZE(HDRP(bp)));
printf("fsize: %d; ", GET_SIZE(FTRP(bp)));
printf("pred: %p, succ: %p\n", (void *) (PREV_FREEBLK(bp)), (void *) (NEXT_FREEBLK(bp)));
}

void check_block(void *bp) {
if (GET_SIZE(HDRP(bp)) % DSIZE)
printf("\terror: not doubly aligned\n");
if (GET(HDRP(bp)) != GET(FTRP(bp)))
printf("\terror: header & foot do not match\n");
}

void print_heap() {
printf("heap\n");
void *bp;
for (bp = heap_listp+DSIZE; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
check_block(bp);
print_block(bp);
}
printf("heap-end\n");
}

//void mm_check(){};
/*
/ASK: What should we do?
MOST IMPORTANT: IS THE BASIC CONCEPT OK
All linked lists connected?
How do we align?
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
