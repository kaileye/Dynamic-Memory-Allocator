#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4);
    void *y = sf_malloc(4);
    memset(y, 0xFF, 4);
    sf_free(x);
    cr_assert(freelist_head == x-8);
    sf_free_header *headofx = (sf_free_header*) (x-8);
    sf_footer *footofx = (sf_footer*) (x - 8 + (headofx->header.block_size << 4)) - 8;

    sf_blockprint((sf_free_header*)((void*)x-8));
    // All of the below should be true if there was no coalescing
    cr_assert(headofx->header.alloc == 0);
    cr_assert(headofx->header.block_size << 4 == 32);
    cr_assert(headofx->header.padding_size == 0);

    cr_assert(footofx->alloc == 0);
    cr_assert(footofx->block_size << 4 == 32);
}

/*
//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################
*/

//Test 1
Test(sf_memsuite, Malloc_max_value, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(16368);
	sf_varprint(ptr);
	cr_assert(freelist_head == NULL);
}
//Test 2
Test(sf_memsuite, Malloc_split, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(48);
	void *ptr2 = sf_malloc(32);
	sf_free(ptr);
	void *ptr3 = sf_malloc(1);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 32);
	sf_free(ptr2);
	sf_free(ptr3);
}

//Test 3
Test(sf_memsuite, Malloc_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(32);
	void *ptr2 = sf_malloc(16);
	sf_free(ptr);
	void *ptr3 = sf_malloc(1);
	ptr3 = (char*)ptr3-8;
	sf_free_header *sfh = (sf_free_header*)ptr3;
	cr_assert((sfh->header.block_size << 4) == 48);
	sf_free(ptr2);
}

//Test 4
Test(sf_memsuite, Free_coalesce_left, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(16);
	void *ptr2 = sf_malloc(32);
	void *ptr3 = sf_malloc(48);
	sf_free(ptr);
	sf_free(ptr2);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 80);
	sf_free(ptr3);
}

//Test 5
Test(sf_memsuite, Free_coalesce_right, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(16);
	void *ptr2 = sf_malloc(32);
	void *ptr3 = sf_malloc(48);
	void *ptr4 = sf_malloc(64);
	sf_free(ptr3);
	sf_free(ptr2);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 112);
	sf_free(ptr);
	sf_free(ptr4);
}
//Test 6
Test(sf_memsuite, Free_coalesce_both, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(16);
	void *ptr2 = sf_malloc(32);
	void *ptr3 = sf_malloc(48);
	void *ptr4 = sf_malloc(64);
	sf_free(ptr);
	sf_free(ptr3);
	sf_free(ptr2);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 144);
	sf_free(ptr4);
}

//Test 7
Test(sf_memsuite, Free_coalesce_all, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(16);
	void *ptr2 = sf_malloc(32);
	void *ptr3 = sf_malloc(48);
	void *ptr4 = sf_malloc(64);
	sf_free(ptr2);
	sf_free(ptr4);
	sf_free(ptr);
	sf_free(ptr3);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 4096);
}

//Test 8
Test(sf_memsuite, Realloc_same_size, .init = sf_mem_init, .fini = sf_mem_fini) {
	int *ptr = sf_malloc(5);
	*ptr = 123;
	int *ptr2 = sf_realloc(ptr, 5);
	cr_assert(*ptr2 == 123);
}

//Test 9
Test(sf_memsuite, Realloc_smaller_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(32);
	void *ptr2 = sf_realloc(ptr, 16);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 4064);
	sf_free(ptr2);
}

//Test 10
Test(sf_memsuite, Realloc_smaller_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(32);
	void *ptr2 = sf_malloc(32);
	void *ptr3 = sf_realloc(ptr, 16);
	ptr3 = (char*)ptr3-8;
	sf_free_header *sfh = (sf_free_header*)ptr3;
	cr_assert((sfh->header.block_size << 4) == 48);
	sf_free(ptr2);
}

//Test 11
Test(sf_memsuite, Realloc_smaller_create_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(48);
	void *ptr2 = sf_malloc(32);
	void *ptr3 = sf_realloc(ptr, 16);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 32);
	sf_free(ptr2);
	sf_free(ptr3);
}

//Test 12
Test(sf_memsuite, Realloc_bigger_available_space_right_freeblock, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *ptr = sf_malloc(16);
	void *ptr2 = sf_malloc(32);
	void *ptr3 = sf_malloc(48);
	sf_free(ptr2);
	void *ptr4 = sf_realloc(ptr, 32);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 32);	
	sf_free(ptr3);
	sf_free(ptr4);
}
//Test 13
Test(sf_memsuite, Realloc_bigger_malloc_memmove_free, .init = sf_mem_init, .fini = sf_mem_fini) {
	int *ptr = sf_malloc(8);
	*ptr = 123;
	void *ptr2 = sf_malloc(32);
	int *ptr3 = sf_realloc(ptr, 32);
	sf_free_header *sfh = (sf_free_header*)freelist_head;
	cr_assert((sfh->header.block_size << 4) == 32);	
	sfh = (sf_free_header*)freelist_head->next;
	cr_assert((sfh->header.block_size << 4) == 3968);
	cr_assert(*ptr3 == 123);	
	sf_free(ptr2);
}

