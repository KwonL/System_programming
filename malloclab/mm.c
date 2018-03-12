/*
 *
 * My mm_alloc() func use Explicit Free Lists.
 * with LIFO free Policy
 * x86-64 processor uses 64-bit address, so each block seems like this
 *
 * -----------------------------------------------------------------------
 * Header : size of block and alloc mark - 4bytes
 * next addr : addr of next freed block (only in free block) - 8bytes   <- bp is here
 * prev addr : addr of previous freed block (only in free block) - 8bytes
 * -----------------------------------------------------------------------
 *                  blank
 *
 * -----------------------------------------------------------------------
 * Footer : size of block and alloc mark - 4bytes
 * -----------------------------------------------------------------------
 *
 * so, one block needs at least (24 + alpha) bytes
 * if block is assigned, then (next addr + prev addr + blank) = payload
 *
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "lkh116",
    /* First member's full name */
    "Lee KwonHyeong",
    /* First member's email address */
    "lkh116@snu.ac.kr",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//my var

static size_t* root_freeList;
#define PSIZE_BLOCK(bolck) (SIZE_BLOCK(block) - 8)
#define SIZE_BLOCK(block) (HEADER(block) & (-2))
#define HEAD_S 24
#define HEADER(block) *(int *)((char*)block - 4)
#define NEXT_BLOCK(block) *((size_t *)block)
#define PREV_BLOCK(block) *(size_t *)((char *)block + 8)
#define FOOTER(block) *(int *)((char *)&HEADER(block) + (SIZE_BLOCK(block) - 4))

//end of myvar

//my func

void* coalecse(size_t *block)
{
    //front block...
    size_t *front_block;

    if (block == (size_t *)((char *)mem_heap_lo() + 8))
        front_block = NULL;
    else
    {
        int sizeofFront = (*(int *)((char *)&FOOTER(block) - SIZE_BLOCK(block))) & (-2);
        front_block = (size_t *)((char *)&FOOTER(block) - SIZE_BLOCK(block) - sizeofFront + 8);
    }
    //freed block at front
    if ((front_block != NULL) && !(HEADER(front_block) & 1))
    {
        //before recieve PREV and NEXT, front pass it
        NEXT_BLOCK(PREV_BLOCK(front_block)) = NEXT_BLOCK(front_block);
        if (NEXT_BLOCK(front_block) != NULL)
            PREV_BLOCK(NEXT_BLOCK(front_block)) = PREV_BLOCK(front_block);
        else
            NEXT_BLOCK(PREV_BLOCK(front_block)) = NULL;

        //pass it to front
        PREV_BLOCK(front_block) = PREV_BLOCK(block);
        NEXT_BLOCK(front_block) = NEXT_BLOCK(block);

        int newsize = SIZE_BLOCK(front_block) + SIZE_BLOCK(block);
        HEADER(front_block) = newsize & (-2);
        FOOTER(front_block) = newsize & (-2);

        if (NEXT_BLOCK(block) != NULL)
            PREV_BLOCK(NEXT_BLOCK(block)) = front_block;
        //complete coalecse with front

        block = front_block;
    }

    //freed block at behind
    size_t *behind_block = (size_t *)((char *)&HEADER(block) + SIZE_BLOCK(block) + 4);
    if (!(HEADER(behind_block) & 1) && (behind_block <= mem_heap_hi()))
    {

        // printf("behind block sum... %p\n", block);
        // printf("PREV behind : %p, NEXT behind : %p", PREV_BLOCK(behind_block), NEXT_BLOCK(behind_block));

        //before recieve PREV and NEXT, behind pass it
        NEXT_BLOCK(PREV_BLOCK(behind_block)) = NEXT_BLOCK(behind_block);
        if (NEXT_BLOCK(behind_block) != NULL)
            PREV_BLOCK(NEXT_BLOCK(behind_block)) = PREV_BLOCK(behind_block);
        else
            NEXT_BLOCK(PREV_BLOCK(behind_block)) = NULL;

        // //pass it to block
        //nope!
        // PREV_BLOCK(block) = PREV_BLOCK(behind_block);
        // NEXT_BLOCK(block) = NEXT_BLOCK(behind_block);

        int newsize = SIZE_BLOCK(behind_block) + SIZE_BLOCK(block);
        HEADER(block) = newsize & (-2);
        FOOTER(block) = newsize & (-2);
    }

    return block;
}

int isPoss(size_t size, size_t* block)
{
    size_t tot_size = SIZE_BLOCK(block);
    int sizeofBehind = *((int *)((char *)&HEADER(block) + SIZE_BLOCK(block)));
    //behind block is empty..?
    if (!(sizeofBehind & 1))
        tot_size += sizeofBehind & (-2);

    if ((tot_size < size) || ((tot_size - size) < 24) || (((char *)block + SIZE_BLOCK(block)) > mem_heap_hi()))
        return 0;
    else
        return 1;
}

//end of my func

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    //heap is empty initially, so add initial block with 0 blank
    size_t* init_block = (size_t *)((char *)mem_sbrk(HEAD_S + 4) + 8);
    NEXT_BLOCK(init_block) = NULL;
    PREV_BLOCK(init_block) = NULL;
    HEADER(init_block) = 24 & (-2);
    FOOTER(init_block) = 24 & (-2);

    root_freeList = init_block;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // basic...
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }

    //my code..wtf..
    size_t* cur_block = root_freeList;
    size_t newsize = ALIGN(size) + 8;

    //if size is too small, return NULL
    if (!(size > 0))
        return NULL;

    // //print freed List
    // printf("mm_malloc called! size is : %d\n", size);

    // size_t* cur = root_freeList;
    // printf("Freed List start!\n");
    // int i;
    // while(cur != NULL)
    // {
    //     printf("%p : prev block is %p, next block is %p  : ", cur, PREV_BLOCK(cur), NEXT_BLOCK(cur));
    //     printf("%d\n", HEADER(cur));

    //     if (root_freeList != NULL)
    //     {
    //         if (NEXT_BLOCK(cur) == root_freeList)
    //             scanf("%d", &i);
    //     }

    //     cur = NEXT_BLOCK(cur);
    // }
    // printf("Freed List end!\n");
    // ////////////////////////


    //searching for freed block
    while (cur_block != NULL)
    {
        //sufficient space, allocate!
        if (newsize <= SIZE_BLOCK(cur_block))
        {
            size_t old_free_size = SIZE_BLOCK(cur_block);
            //fragmentation
            if ((old_free_size - newsize) > 23)
            {
                //new_block is new fragmented block after allocated block
                size_t* new_block = (size_t *)((char *)cur_block + newsize);
                HEADER(new_block) = (old_free_size - newsize) & (-2);
                FOOTER(new_block) = (old_free_size - newsize) & (-2);

                //add to freeList
                NEXT_BLOCK(new_block) = NEXT_BLOCK(cur_block);
                PREV_BLOCK(new_block) = PREV_BLOCK(cur_block);
                
                if (NEXT_BLOCK(cur_block) == NULL && PREV_BLOCK(cur_block) == NULL)
                {
                    root_freeList = new_block;
                }
                else
                {
                    if (NEXT_BLOCK(cur_block) != NULL)
                        PREV_BLOCK(NEXT_BLOCK(cur_block)) = new_block;
                    else
                        NEXT_BLOCK(PREV_BLOCK(cur_block)) = NULL;
                    if (PREV_BLOCK(cur_block) != NULL)
                        NEXT_BLOCK(PREV_BLOCK(cur_block)) = new_block;
                    else
                    {
                        root_freeList = new_block;
                        PREV_BLOCK(root_freeList) = NULL;
                    }
                }

                //allocating
                HEADER(cur_block) = newsize | 1;
                FOOTER(cur_block) = newsize | 1;

                // /////////////////////////////////////
                // printf("new block is : %p, prev is : %p, next is : %p\n", new_block, PREV_BLOCK(new_block), NEXT_BLOCK(new_block));
                // /////////////////////////////////////

                return cur_block;
            }
            else
            //no sufficient space for headers, so no fragmentation.
            {
                if ((NEXT_BLOCK(cur_block) == NULL) && (PREV_BLOCK(cur_block) == NULL))
                    root_freeList = NULL;
                else
                {
                    if (NEXT_BLOCK(cur_block) != NULL)
                        PREV_BLOCK(NEXT_BLOCK(cur_block)) = PREV_BLOCK(cur_block);
                    else
                        NEXT_BLOCK(PREV_BLOCK(cur_block)) = NULL;
                    if (PREV_BLOCK(cur_block) != NULL)
                        NEXT_BLOCK(PREV_BLOCK(cur_block)) = NEXT_BLOCK(cur_block);
                    else
                    {
                        root_freeList = NEXT_BLOCK(cur_block);
                        PREV_BLOCK(root_freeList) = NULL;
                    }
                }

                //allocating
                HEADER(cur_block) = old_free_size | 1;
                FOOTER(cur_block) = old_free_size | 1;
            }

            return cur_block;
        }

        cur_block = NEXT_BLOCK(cur_block);
    }

    //if ther is no sufficient block, extend heap and add block
    int sizeofTop = *(int *)((char *)mem_heap_hi() - 3) & (-2);
    size_t* top_block = (size_t *)((char *)mem_heap_hi() + 1 - sizeofTop + 4);

    // printf("allocated in extended heap, top is : %p\n", top_block);
    // printf("memory hi is : %p\n", mem_heap_hi());

    if ((HEADER(top_block) & 1) == 0)
    {
        mem_sbrk(newsize - sizeofTop);
        sizeofTop = newsize;

        //allocate from extended top block
        HEADER(top_block) = sizeofTop;
        FOOTER(top_block) = sizeofTop;

        cur_block = top_block;

        HEADER(cur_block) = newsize | 1;
        FOOTER(cur_block) = newsize | 1;

        if ((NEXT_BLOCK(cur_block) == NULL) && (PREV_BLOCK(cur_block) == NULL))
            root_freeList = NULL;
        else
        {
            if (NEXT_BLOCK(cur_block) != NULL)
                PREV_BLOCK(NEXT_BLOCK(cur_block)) = PREV_BLOCK(cur_block);
            else
                NEXT_BLOCK(PREV_BLOCK(cur_block)) = NULL;
            if (PREV_BLOCK(cur_block) != NULL)
                NEXT_BLOCK(PREV_BLOCK(cur_block)) = NEXT_BLOCK(cur_block);
            else
            {
                root_freeList = NEXT_BLOCK(cur_block);
                PREV_BLOCK(root_freeList) = NULL;
            }
        }
    }
    else
    {
        cur_block = (size_t *)((char *)mem_sbrk(newsize) + 4);
        HEADER(cur_block) = newsize | 1;
        FOOTER(cur_block) = newsize | 1;
    }
    return cur_block;
}

/*
 * mm_free - Freeing a block.
 *
 */
void mm_free(void *ptr)
{
    // printf("free called... : %p, size if : %d\n", ptr, HEADER(ptr));

    // //print freed List
    // size_t* cur = root_freeList;
    // printf("Freed List start!\n");
    // int i;
    // while(cur != NULL)
    // {
    //     printf("%p : prev block is %p, next block is %p  : ", cur, PREV_BLOCK(cur), NEXT_BLOCK(cur));
    //     printf("%d\n", HEADER(cur));

    //     if (root_freeList != NULL)
    //     {
    //         if (NEXT_BLOCK(cur) == root_freeList)
    //             scanf("%d", &i);
    //     }

    //     cur = NEXT_BLOCK(cur);
    // }
    // printf("Freed List end!\n");
    // ////////////////////

    ptr = (size_t *) ptr;

    //turn off alloc mark
    HEADER(ptr) = HEADER(ptr) & (-2);
    FOOTER(ptr) = FOOTER(ptr) & (-2);

    if (root_freeList == NULL)
    {
        root_freeList = ptr;
        NEXT_BLOCK(ptr) = NULL;
        PREV_BLOCK(ptr) = NULL;
    }
    else
    {
        NEXT_BLOCK(ptr) = root_freeList;
        PREV_BLOCK(root_freeList) = ptr;
        root_freeList = ptr;
        PREV_BLOCK(ptr) = NULL;
    }

    ptr = (size_t *)coalecse(ptr);

    root_freeList = ptr;
}

/*
 * mm_realloc -
 */
void *mm_realloc(void *ptr, size_t size)
{
    // // // basic version
    // void *oldptr = ptr;
    // void *newptr;
    // size_t copySize;

    // newptr = mm_malloc(size);
    // if (newptr == NULL)
    //   return NULL;
    // copySize = SIZE_BLOCK(oldptr) - 8;
    // if (size < copySize)
    //   copySize = size;
    // memcpy(newptr, oldptr, copySize);
    // mm_free(oldptr);

    // return newptr;

    size_t oldsize = SIZE_BLOCK(ptr);
    size_t* oldptr = (size_t *)ptr;
    size_t* newptr = oldptr;
    size_t newsize = ALIGN(size) + 8;

    if (!(size > 0) || ptr == NULL)
        return NULL;

    //Free remainder memory and sum up to behind block
    if (newsize < oldsize)
    {
        if (oldsize - newsize > 23)
        {
            size_t* new_block = (size_t *)((char *)newptr + newsize);

            HEADER(new_block) = oldsize - newsize;
            FOOTER(new_block) = oldsize - newsize;

            HEADER(newptr) = newsize | 1;
            FOOTER(newptr) = newsize | 1;

            mm_free(new_block);

        }

        return (void *)newptr;
    }

    //if there is sufficeint memory with behind memory

    if (isPoss(newsize, oldptr))
    {
        // printf("newsize is %d, ptr is %p\n", newsize, oldptr);
        size_t* new_block = (size_t *)((char *)newptr + newsize);
        size_t* behind_block = (size_t *)((char *)&HEADER(oldptr) + SIZE_BLOCK(oldptr) + 4);
        size_t tot_size = SIZE_BLOCK(newptr) + SIZE_BLOCK(behind_block);
        
        HEADER(new_block) = tot_size - newsize;
        FOOTER(new_block) = tot_size - newsize;

        //remove behind block from freed_List
        if (NEXT_BLOCK(behind_block) == NULL && PREV_BLOCK(behind_block) == NULL)
        {
            root_freeList = NULL;
        }
        else
        {
            if (NEXT_BLOCK(behind_block) != NULL)
                PREV_BLOCK(NEXT_BLOCK(behind_block)) = PREV_BLOCK(behind_block);
            else
                NEXT_BLOCK(PREV_BLOCK(behind_block)) = NULL;
            if (PREV_BLOCK(behind_block) != NULL)
                NEXT_BLOCK(PREV_BLOCK(behind_block)) = NEXT_BLOCK(behind_block);
            else
            {
                root_freeList = NEXT_BLOCK(behind_block);
                PREV_BLOCK(NEXT_BLOCK(behind_block)) = NULL;
            }
        }

       //allocate
        HEADER(newptr) = newsize | 1;
        FOOTER(newptr) = newsize | 1;

        mm_free(new_block);

        return newptr;
    }
    
    newptr = mm_malloc(newsize);
    memcpy(newptr, oldptr, newsize);
    mm_free(oldptr);

    return newptr;
}