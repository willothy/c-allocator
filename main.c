#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define ALIGNMENT 8
#define WORD_SIZE (sizeof(size_t))
#define TAGS_SIZE (2 * WORD_SIZE)
#define MIN_BLOCK_SIZE (4 * WORD_SIZE)

typedef struct {
  // Size of the block
  // Assumed to be a multiple of 8.
  // The last bit is used to indicate whether the block is free or not.
  size_t size;

  // The size of the payload is given by size & ~1.
  // When a block is free, payload[0] is a pointer to the next free block,
  // and payload[1] is a pointer to the previous free block.
  size_t payload[];
} block_t;

enum result_t {
  Ok = 0,
  Err = -1,
};

static block_t *head = NULL;
static block_t *tail = NULL;

block_t *free_head = NULL;

static inline size_t align_up(size_t size) {
  return (size + WORD_SIZE - 1) & ~(WORD_SIZE - 1);
}

static inline size_t *block_end_tag(block_t *bp) {
  return (size_t *)((char *)bp + (bp->size & ~1) - WORD_SIZE);
}

static inline size_t block_size(block_t *bp) { return bp->size & ~1; }

static inline int block_allocated(block_t *bp) { return bp->size & 1; }

static inline void block_set_size(block_t *bp, size_t size) {
  int allocated = block_allocated(bp);
  bp->size = size | allocated;
  *block_end_tag(bp) = size | allocated;
}

static inline void block_set_allocated(block_t *bp, int allocated) {
  size_t size = block_size(bp);
  bp->size = size | allocated;
  *block_end_tag(bp) = size | allocated;
}

static inline void block_set_fprev(block_t *bp, block_t *prev) {
  bp->payload[1] = (size_t)prev;
}

static inline void block_set_fnext(block_t *bp, block_t *next) {
  bp->payload[0] = (size_t)next;
}

static inline block_t *block_fprev(block_t *bp) {
  return (block_t *)bp->payload[1];
}

static inline block_t *block_fnext(block_t *bp) {
  return (block_t *)bp->payload[0];
}

static inline size_t block_prev_size(block_t *bp) {
  size_t *prev_end_tag = (size_t *)((char *)bp - WORD_SIZE);
  return *prev_end_tag & ~1;
}

static inline size_t block_next_size(block_t *bp) {
  size_t *next_tag = (size_t *)((char *)bp + block_size(bp));
  return *next_tag & ~1;
}

static inline block_t *block_next(block_t *bp) {
  return (block_t *)((char *)bp + block_size(bp));
}

static inline block_t *block_prev(block_t *bp) {
  return (block_t *)((char *)bp - block_prev_size(bp));
}

int init() {
  head = (block_t *)sbrk(TAGS_SIZE);
  if (head == (void *)-1) {
    return Err;
  }
  block_set_size(head, TAGS_SIZE);
  block_set_allocated(head, 1);

  tail = (block_t *)sbrk(TAGS_SIZE);
  if (tail == (void *)-1) {
    return Err;
  }
  block_set_size(tail, TAGS_SIZE);
  block_set_allocated(tail, 1);

  free_head = NULL;

  return Ok;
}

void insert_free_block(block_t *bp) {
  block_t *head = free_head;
  if (head == NULL) {
    bp->payload[0] = (size_t)bp;
    bp->payload[1] = (size_t)bp;
  } else {
    block_t *tail = block_fprev(head);
    block_set_fnext(tail, bp);
    block_set_fprev(bp, tail);
    block_set_fnext(bp, head);
    block_set_fprev(head, bp);
  }

  free_head = bp;
}

void *alloc(size_t size) {
  if (size <= 0) {
    return NULL;
  }

  size_t aligned_size = align_up(size);
  size_t total_size = aligned_size + TAGS_SIZE;

  if (total_size < MIN_BLOCK_SIZE) {
    total_size = MIN_BLOCK_SIZE - TAGS_SIZE;
  }

  block_t *current = free_head;

  while (current != NULL) {
    size_t size = block_size(current);
    if (size >= total_size) {
      if (size - total_size >= MIN_BLOCK_SIZE) {
        size_t new_empty = size - total_size;
        block_set_size(current, new_empty);
        block_set_allocated(current, 0);
        insert_free_block(current);

        block_t *new_block = block_next(current);
        block_set_size(new_block, total_size);
        block_set_allocated(current, 1);
      }
      return current->payload;
    }
  }

  return NULL;
}

void *realloc(void *ptr, size_t size) {
  // TODO
  return NULL;
}

void dealloc(void *ptr) {
  // TODO
}

int main(int argc, char **argv) {
  //
  return 0;
}
