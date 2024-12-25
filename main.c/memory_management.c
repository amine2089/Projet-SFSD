
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// function prototypes
void intializeMemory(int num_blocks, int b_size);
void displayMemoryState();
int ContiguousAllocation(int file_id, int num_blocks);
int chainedAllocation(int file_id, int num_blocks);
void freeBlocks(int file_id);
void compactMemory();
void clearAllMemory();

typedef struct
{
  bool is_free;
  int file_id;
  int next_block;
} Block;

Block *memory;
int total_blocks;
int block_size;

void intializeMemory(int num_blocks, int b_size)
{
  total_blocks = num_blocks;
  block_size = b_size;
  memory = (Block *)malloc(num_blocks * sizeof(Block)); // idk what this line means

  if (memory == NULL)
  {
    printf("error");
    exit(1);
  }

  for (int i = 0; i < num_blocks; i++)
  {
    memory[i].is_free = true;
    memory[i].file_id = 0;
    memory[i].next_block = -1;
  }
  printf("secondary memory is initialized with %d blocks of %d bytes \n", num_blocks, b_size);
}

void displayMemoryState()
{
  printf("The state of tge memory is as follows : \n");
  for (int i = 0; i < total_blocks; i++)
  {
    if (memory[i].is_free)
    {
      printf("[FREE]");
    }
    else
    {
      printf("[FILE ID : %d]", memory[i].file_id);
    }
  }
}

int contiguousAllocation(int file_id, int num_blocks)
{
  int start = -1, count = 0;

  // searching for 'num_blocks' contiguous free blocks
  for (int i = 0; i < total_blocks; i++)
  {
    if (memory[i].is_free)
    {
      if (count == 0)
        start = i;
      count++;
      if (count == num_blocks)
        break;
    }
    else
    {
      count = 0;
    }
  }
  if (count < num_blocks)
  {
    printf("failed to allocate %d blocks for ile ID %d \n", num_blocks, file_id);
    return -1;
  }
  // alocating the found blocks
  for (int i = start; i < start + num_blocks; i++)
  {
    memory[i].is_free = false;
    memory[i].file_id = file_id;
    memory[i].next_block = -1;
  }
  printf("allocated %d blocks from block %d for file ID %d", num_blocks, start, file_id);
  return start;
}

// chained allocation
int chainedAllocation(int file_id, int num_blocks)
{
  int allocated = 0, previous = -1, first = -1;

  // searching for free blocks
  for (int i = 0; i < total_blocks && allocated < num_blocks; i++)
  {
    if (memory[i].is_free)
    {
      if (previous != -1)
      {
        memory[previous].next_block = i;
      }
      memory[i].is_free = false;
      memory[i].file_id = file_id;
      memory[i].next_block = -1;
      if (first == -1)
        first = i;
      previous = i;
      allocated++;
    }
  }
  // checking if allocation was successful
  if (allocated < num_blocks)
  {
    printf("Failed to allocate %d blocks for file ID %d \n", num_blocks, file_id);
    freeBlocks(file_id);
    return -1;
  }
  printf("allocated %d starting from block %d to file ID %d", num_blocks, first, file_id);
  return first;
}

// freeing all blocks associated with a certain file
void freeBlocks(int file_id)
{
  for (int i = 0; i < total_blocks; i++)
  {
    if (memory[i].file_id == file_id)
    {
      memory[i].is_free = true;
      memory[i].file_id = 0;
      memory[i].next_block = -1;
    }
  }
  printf("blocks associated to file %d free \n", file_id);
}

void compactMemory()
{
  int write_index = 0;
  for (int read_index = 0; read_index < total_blocks; read_index++)
  {
    if (!memory[read_index].is_free)
    {
      if (write_index != read_index)
      {
        memory[write_index] = memory[read_index];
        memory[read_index].is_free = true;
        memory[read_index].file_id = 0;
        memory[read_index].next_block = -1;
      }
      write_index++;
    }
  }
  for (int i = write_index; i < total_blocks; i++)
  {
    memory[i].is_free = true;
    memory[i].file_id = 0;
    memory[i].next_block = -1;
  }
  printf("memory compacted");
}

// clearing all memory
void clearAllMemory()
{
  free(memory);
  memory = NULL;
  printf("memory cleared \n");
}

// testing
int main()
{
  intializeMemory(10, 512);

  displayMemoryState();

  contiguousAllocation(1, 3);
  displayMemoryState();

  chainedAllocation(2, 4);
  displayMemoryState();

  freeBlocks(1);
  displayMemoryState();

  compactMemory();
  displayMemoryState();

  clearAllMemory();
  return 0;
}
