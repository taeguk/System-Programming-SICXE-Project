#include "memory.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct memory_manager
  {
    void *memory;
    uint32_t memory_size;
  };

/* memory manager를 생성하는 factory 함수 */
struct memory_manager *memory_manager_construct (uint32_t memory_size)
{
  struct memory_manager *manager = malloc (sizeof(*manager));
  
  manager->memory = malloc (memory_size);
  manager->memory_size = memory_size;

  return manager;
}

/* memory manager를 소멸시키는 함수 */
void memory_manager_destroy (struct memory_manager *manager)
{
  free (manager->memory);
  free (manager);
}

/* 특정 offset의 memory의 값을 val로 바꿔주는 함수 */
bool memory_edit (struct memory_manager *manager, uint32_t offset, uint8_t val)
{
  if (offset >= manager->memory_size)
    return false;

  ((uint8_t*) manager->memory)[offset] = val;
  return true;
}

/* [start, end] offset 범위의 memory에 대하여 val 값으로 채워주는 함수 */
bool memory_fill (struct memory_manager *manager, uint32_t start, uint32_t end, uint8_t val)
{
  if (start > end)
    return false;

  memset (manager->memory + start, val, end-start+1);

  return true;
}

/* memory를 0으로 초기화하는 함수 */
void memory_reset (struct memory_manager *manager)
{
  memset (manager->memory, 0, manager->memory_size);
}

/* [start, end] offset 범위의 memory를 dump해서 출력해주는 함수
 * 만약 enable_max_end가 true라면, end가 memory 범위를 초과하는 offset일 경우
 * end를 가능한 최대 memory offset으로 세팅해준다.
 */
bool memory_dump (struct memory_manager *manager, uint32_t start, uint32_t end, bool enable_max_end)
{
  if (enable_max_end && end >= manager->memory_size) 
    end = manager->memory_size - 1;

  if (start > end)
    return false;

  uint32_t aligned_start = start / 16 * 16,
           aligned_end = (end + 15 + 1) / 16 * 16;

  for (uint32_t base = aligned_start;
       base < aligned_end;
       base += 16)
    {
      printf ("%05X ", base);

      for (int i = 0; i < 16; ++i)
        {
          uint32_t offset = base + i;

          if (offset < start || offset > end)
            printf ("   ");
          else
            printf ("%02X ", ((uint8_t*) manager->memory)[offset]);
        }
      printf ("; ");

      for (int i = 0; i < 16; ++i)
        {
          uint32_t offset = base + i;

          if (offset < start || offset > end)
            printf (".");
          else
            {
              uint8_t val = ((uint8_t*) manager->memory)[offset];
              if (0x20 <= val && val <= 0x7E)
                printf ("%c", (char) val);
              else
                printf (".");
            }
        }
      printf("\n");
    }

  return true;
}

/* memory 크기를 반환하는 함수 */
uint32_t memory_get_memory_size (struct memory_manager *manager)
{
  return manager->memory_size;
}
