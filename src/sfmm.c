#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "sfmm.h"

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;
void *heapstart;
void *heapend;
static size_t internal = 0;
static size_t external = 0;
static size_t allocations = 0;
static size_t frees = 0;
static size_t coalesce = 0;

void *sf_malloc(size_t size){
	void *ptr = NULL;
	sf_free_header *sfh, *split;
	sf_footer *footer;
	size_t padding, splinter, blocksize;
	
	if (size == 0) {
		return NULL;
	}
	if (NULL == freelist_head) {
		ptr = sf_sbrk(0);
		if (allocations == 0) {
			heapstart = ptr;
		}
		if ((heapend = sf_sbrk(1)) != (void *)-1) {
			external += 4096;
			sfh = ptr;
			sfh->header.alloc = 0;
			sfh->header.block_size = 4096 >> 4;
			sfh->header.padding_size = 0;
			sfh->next = freelist_head;
			sfh->prev = NULL;
			freelist_head = sfh;
			ptr = (char*) sfh + 4096 - 8;
			footer = (sf_footer*) ptr;
			footer->alloc = 0;
			footer->block_size = 4096 >> 4;
		} else {
			errno = ENOMEM;
			return NULL;
		}
	}
	sfh = freelist_head;
	while ((sfh->header.block_size << 4) - 16 < size) {
		sfh = sfh->next;
		if (NULL == sfh) {
			ptr = sf_sbrk(0);
			if ((heapend = sf_sbrk(1)) != (void *)-1) {
				external += 4096;
				ptr = (char*)ptr-8;
				footer = ptr;
				if (footer->alloc == 1) {
					ptr = (char*)ptr+8;
					sfh = ptr;
					sfh->header.alloc = 0;
					sfh->header.block_size = 4096 >> 4;
					sfh->header.padding_size = 0;
					sfh->next = freelist_head;
					sfh->prev = NULL;
					if (NULL != freelist_head) {
						freelist_head->prev = sfh;	
					}
					freelist_head = sfh;
					ptr = (char*)sfh + 4096 - 8;
					footer = (sf_footer*) ptr;
					footer->alloc = 0;
					footer->block_size = sfh->header.block_size;
				} else { 
					coalesce++;
					ptr = (char*)ptr-(footer->block_size << 4)+8;
					sfh = ptr;
					sfh->header.block_size += (4096 >> 4);
					ptr = (char*)sfh+(sfh->header.block_size << 4)-8;
					footer = ptr;
					footer->alloc = 0;
					footer->block_size = sfh->header.block_size;
					sfh = freelist_head;
				}
			} else {
				return NULL;
			}
		}
	}
	blocksize = sfh->header.block_size << 4;
	padding = 16 - (size % 16);
	if (padding == 16) {
		padding = 0;
	}
	size += padding;
	splinter = (sfh->header.block_size << 4) - (size + 16);
	if (splinter == 16) {
		size += splinter;
	}
	external -= (size + 16);

	sfh->header.alloc = 1;
	sfh->header.block_size = (size + 16) >> 4;
	sfh->header.padding_size = padding;
	internal += padding;
	if (sfh->prev != NULL) {
		sfh->prev->next = sfh->next;
	} else {
		freelist_head = sfh->next;
	}
	if (sfh->next != NULL) {
		sfh->next->prev = sfh->prev;		
	}
	if (splinter != 0 && splinter != 16) {
		
		blocksize = blocksize - (size + 16);
		ptr = (char*)sfh+size+16;
		split = ptr;
		split->header.alloc = 0;
		split->header.block_size = blocksize >> 4;
		split->header.padding_size = 0;
		split->next = freelist_head;
		split->prev = NULL;
		if (freelist_head != NULL) {
			freelist_head->prev = split;
		}
		freelist_head = split;
		ptr = (char*)split+blocksize-8;
		footer = ptr;
		footer->alloc = 0;
		footer->block_size = blocksize >> 4;
	}
	ptr = (char*)sfh+size+8;
	footer = ptr;
	footer->alloc = 1;
	footer->block_size = (size + 16) >> 4;
	
	ptr = sfh;
	allocations++;
	internal += 16;
	return (char*)ptr+8;
}

void sf_free(void *ptr){
	sf_free_header *sfh, *sfh2;
	sf_footer *footer;
	size_t size;

	if (NULL == ptr) {
		return;
	}
	ptr = (char*)ptr-8;
	sfh = ptr;
	ptr = (char*)ptr-8;
	footer = ptr;
	ptr = (char*)sfh+(sfh->header.block_size << 4);
	sfh2 = ptr;
	internal -= sfh->header.padding_size;
	external += sfh->header.block_size;
	sfh->header.padding_size = 0;
	if (footer->alloc == 0 && sfh2->header.alloc == 0 && sfh != heapstart && sfh2 != heapend) {
		coalesce++;
		if (sfh2->prev != NULL) {
			sfh2->prev->next = sfh2->next;
		} else {
			freelist_head = sfh2->next;
		}
		if (sfh2->next != NULL) {
			sfh2->next->prev = sfh2->prev;		
		}
		size = sfh2->header.block_size;
		ptr = (char*)footer-(footer->block_size << 4)+8;
		sfh2 = ptr;
		if (sfh2->prev != NULL) {
			sfh2->prev->next = sfh2->next;
		} else {
			freelist_head = sfh2->next;
		}
		if (sfh2->next != NULL) {
			sfh2->next->prev = sfh2->prev;		
		}
		sfh2->header.block_size += sfh->header.block_size;
		sfh2->header.block_size += size;
		sfh2->next = freelist_head;
		sfh2->prev = NULL;
		if (freelist_head != NULL) {
			freelist_head->prev = sfh2;
		}
		freelist_head = sfh2;
		ptr = (char*)sfh2+(sfh2->header.block_size << 4)-8;
		footer = ptr;
		footer->block_size = sfh2->header.block_size;	
	} else if (footer->alloc == 0 && sfh != heapstart) {
		coalesce++;
		ptr = (char*)footer-(footer->block_size << 4)+8;
		sfh2 = ptr;
		if (sfh2->prev != NULL) {
			sfh2->prev->next = sfh2->next;
		} else {
			freelist_head = sfh2->next;
		}
		if (sfh2->next != NULL) {
			sfh2->next->prev = sfh2->prev;		
		}
		sfh2->header.block_size += sfh->header.block_size;
		sfh2->next = freelist_head;
		sfh2->prev = NULL;
		if (freelist_head != NULL) {
			freelist_head->prev = sfh2;
		}
		freelist_head = sfh2;
		ptr = (char*)sfh2+(sfh2->header.block_size << 4)-8;
		footer = ptr;
		footer->block_size = sfh2->header.block_size;	
	} else if (sfh2->header.alloc == 0 && sfh2 != heapend) {
		coalesce++;
		if (sfh2->prev != NULL) {
			sfh2->prev->next = sfh2->next;
		}
		if (sfh2->next != NULL) {
			sfh2->next->prev = sfh2->prev;		
		}
		sfh->header.alloc = 0;
		sfh->header.block_size += sfh2->header.block_size;
		sfh->header.padding_size = 0;
		sfh->next = freelist_head;
		sfh->prev = NULL;
		if (freelist_head != NULL) {
			freelist_head->prev = sfh;
		}
		freelist_head = sfh;
		ptr = (char*)sfh+(sfh->header.block_size << 4)-8;
		footer = ptr;
		footer->block_size = sfh->header.block_size;
	} else {
		sfh->header.alloc = 0;
		sfh->header.padding_size = 0;
		sfh->next = freelist_head;
		sfh->prev = NULL;
		if (freelist_head != NULL) {
			freelist_head->prev = sfh;
		}
		freelist_head = sfh;
		ptr = (char*)sfh+(sfh->header.block_size << 4)-8;
		footer = ptr;
		footer->alloc = 0;
	}

	internal -= 16;
	frees++;
}

void *sf_realloc(void *ptr, size_t size){
	sf_free_header *sfh, *sfh2;
	sf_footer *footer;
	void *ptr2;
	size_t leftover, addition, padding;
	
	if (size == 0) {
		errno = EINVAL;
		return NULL;
	}
	if (NULL == ptr) {
		return sf_malloc(size);
	} 
	ptr2 = (char*)ptr-8;
	sfh = ptr2;
	ptr2 = (char*)sfh+(sfh->header.block_size << 4);
	sfh2 = ptr2;
	if ((sfh->header.block_size << 4) == (size + 16)) {
		return ptr;
	}
	padding = 16 - (size % 16);
	if (padding == 16) {
		padding = 0;
	}
	size += padding;
	if ((sfh->header.block_size << 4) == (size + 16)) {
		internal -= sfh->header.padding_size;		
		sfh->header.padding_size = padding;
		internal += sfh->header.padding_size;	
		allocations++;	
		return ptr;
	} else if ((sfh->header.block_size << 4) > (size + 16)) {
		leftover = (sfh->header.block_size << 4) - (size + 16);
		if (sfh2->header.alloc == 0 && sfh2 != heapend) {
			internal -= sfh->header.padding_size;		
			sfh->header.padding_size = padding;
			internal += sfh->header.padding_size;	
			allocations++;
			sfh->header.block_size = ((size + 16) >> 4);
			ptr2 = (char*)sfh+(sfh->header.block_size << 4)-8;
			footer = ptr2;
			footer->block_size = sfh->header.block_size;
			footer->alloc = 1;
			coalesce++;
			if (sfh2->prev != NULL) {
				sfh2->prev->next = sfh2->next;
			} else {
				freelist_head = sfh2->next;
			}
			if (sfh2->next != NULL) {
				sfh2->next->prev = sfh2->prev;		
			}
			ptr2 = (char*)ptr2+8;
			sfh = ptr2;
			sfh->header.alloc = 0;
			sfh->header.block_size = (leftover + (sfh2->header.block_size << 4)) >> 4;
			sfh->header.padding_size = 0;
			sfh->next = freelist_head;
			sfh->prev = NULL;
			if (freelist_head != NULL) {
				freelist_head->prev = sfh;
			}
			freelist_head = sfh;
			ptr2 = (char*)sfh+(sfh->header.block_size << 4)-8;
			footer = ptr2;
			footer->block_size = sfh->header.block_size;
			external += leftover;
			return ptr;
		} else if (leftover == 16) {
			internal -= sfh->header.padding_size;		
			sfh->header.padding_size = padding;
			internal += sfh->header.padding_size;	
			allocations++;	
			return ptr;
		} else {
			internal -= sfh->header.padding_size;		
			sfh->header.padding_size = padding;
			internal += sfh->header.padding_size;	
			allocations++;
			sfh->header.block_size = ((size + 16) >> 4);
			ptr2 = (char*)sfh+(sfh->header.block_size << 4)-8;
			footer = ptr2;
			footer->block_size = sfh->header.block_size;
			footer->alloc = 1;
			ptr2 = (char*)ptr2+8;
			sfh = ptr2;
			sfh->header.alloc = 0;
			sfh->header.block_size = leftover >> 4;
			sfh->header.padding_size = 0;
			sfh->next = freelist_head;
			sfh->prev = NULL;
			if (freelist_head != NULL) {
				freelist_head->prev = sfh;
			}
			freelist_head = sfh;
			ptr2 = (char*)sfh+(sfh->header.block_size << 4)-8;
			footer = ptr2;
			footer->alloc = 0;
			footer->block_size = sfh->header.block_size;
			external += leftover;
			return ptr;
		}
	} else {
		addition = (size + 16) - (sfh->header.block_size << 4);
		if (sfh2->header.alloc == 0 && sfh2 != heapend && (sfh2->header.block_size << 4) >= addition) {
			if (sfh2->prev != NULL) {
				sfh2->prev->next = sfh2->next;
			} else {
				freelist_head = sfh2->next;
			}
			if (sfh2->next != NULL) {
				sfh2->next->prev = sfh2->prev;		
			}
			leftover = (sfh2->header.block_size << 4) - addition;
			if (leftover == 0 || leftover == 16) {
				size += leftover;
				external -= leftover;
			}
			external -= addition;
			internal -= sfh->header.padding_size;		
			sfh->header.padding_size = padding;
			internal += sfh->header.padding_size;	
			allocations++;
			sfh->header.block_size = ((size + 16) >> 4);
			ptr2 = (char*)sfh+(sfh->header.block_size << 4)-8;
			footer = ptr2;
			footer->block_size = sfh->header.block_size;
			footer->alloc = 1;	
			if (leftover != 0 && leftover != 16) {
				ptr2 = (char*)ptr2+8;
				sfh = ptr2;
				sfh->header.alloc = 0;
				sfh->header.block_size = leftover >> 4;
				sfh->header.padding_size = 0;
				sfh->next = freelist_head;
				sfh->prev = NULL;
				if (freelist_head != NULL) {
					freelist_head->prev = sfh;
				}
				freelist_head = sfh;
				ptr2 = (char*)sfh+(sfh->header.block_size << 4)-8;
				footer = ptr2;
				footer->alloc = 0;
				footer->block_size = sfh->header.block_size;
			}
			return ptr;
		} else {
			size -= padding;
			ptr2 = sf_malloc(size);
			if (NULL == ptr2) {
				errno = ENOMEM;
				return NULL;
			}
			memmove(ptr2, ptr, (sfh->header.block_size << 4) - (sfh->header.padding_size) - 16);
			sf_free(ptr);
			return ptr2;
		}
	}

	return NULL;
}

int sf_info(info* meminfo){
	meminfo->internal = internal;
	meminfo->external = external; 
	meminfo->allocations = allocations;
	meminfo->frees = frees;
	meminfo->coalesce = coalesce;
	return 0;
}
