#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	int i;
	for (i = 0; i < seg_table->size; i++) {
		// Enter your code here
		
		if(seg_table->table[i].v_index == index) 
			return seg_table->table[i].pages;
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}
	
	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			
			*physical_addr = (page_table->table[i].p_index << 10) + offset;
			return 1;
		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = (size % PAGE_SIZE == 0) ? size / PAGE_SIZE :
		size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

	//todo
	int physical_indexs[num_pages];
	if (proc->bp + num_pages * PAGE_SIZE <= RAM_SIZE){		
		int i, count = 0;		
		for( i = 0; i < NUM_PAGES; i++){
		  	if (_mem_stat[i].proc == 0) {
				physical_indexs[count] = i;
				count++;
			}

		  	if (count >= num_pages) {
				mem_avail = 1;
				break;
			}
		}
	}
	//end-todo

	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		if(ret_mem == PAGE_SIZE) proc->seg_table->size = 0;
		int j, k;
		for( j = 0; j < num_pages; j++){
			_mem_stat[physical_indexs[j]].proc = proc->pid;
			_mem_stat[physical_indexs[j]].index = j;
			if(j == num_pages - 1) 
	   		      _mem_stat[physical_indexs[j]].next = -1; 				
			else  _mem_stat[physical_indexs[j]].next = physical_indexs[j + 1];
			/* The first layer index */
			addr_t first_lv = get_first_lv(ret_mem + j * PAGE_SIZE);
			/* The second layer index */
			addr_t second_lv = get_second_lv(ret_mem + j * PAGE_SIZE);		
			int find = -1;
			for(k = 0; k < proc->seg_table->size; k++){
	           	    if(first_lv == proc->seg_table->table[k].v_index)
					{find = k; break;}			
			}
			if(find == -1){ 
				find = proc->seg_table->size;
				proc->seg_table->size++;
				proc->seg_table->table[find].pages = (struct page_table_t*)malloc(sizeof(struct page_table_t));
				proc->seg_table->table[find].pages->size = 0;	
				proc->seg_table->table[find].v_index = first_lv;
			}
			int count = proc->seg_table->table[find].pages->size;
			
			   proc->seg_table->table[find].pages->table[count].v_index = second_lv;
			   proc->seg_table->table[find].pages->table[count].p_index = physical_indexs[j];		
			   count++;			
			proc->seg_table->table[find].pages->size = count;
			
		}
		
	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);
	addr_t physical_addr;
	if (!translate(address, &physical_addr, proc)) {	
		pthread_mutex_unlock(&mem_lock);	
		return -1;
	}
	
	int find = physical_addr >> 10;
	int i, j, indexs_to_free = 0;	
	
	while(find != -1){
		_mem_stat[find].proc = 0;
		find = _mem_stat[find].next;
		indexs_to_free++;			
	}
	
	/* The first layer index */
	addr_t first_lv = get_first_lv(address);
	/* The second layer index */
	addr_t second_lv = get_second_lv(address);
	
	struct seg_table_t* seg_table = proc->seg_table;
	struct page_table_t* page_table;
	//loop for each page_table in seg_table
	for(i=0; i < seg_table->size; i++){
	    if(first_lv == seg_table->table[i].v_index){
			page_table = seg_table->table[i].pages;
			//loop for each entry in page_table
			for(j = 0; j < page_table->size; j++){
				if(page_table->table[j].v_index == second_lv) { 
					//if top is larger than size, there are indexs to be freed from the next table
					while (indexs_to_free > 0) {
						int top = j + indexs_to_free;
						if (top >= page_table->size) {
							int already_free = page_table->size - j;
							page_table->size = j;
							indexs_to_free -= already_free;
							if (page_table->size == 0) {
								void* src_addr = &seg_table->table[i + 1];
								void* dest_addr = &seg_table->table[i];
								int n_bytes_to_copy = ((void*)&seg_table->table[seg_table->size]) - src_addr;
								memcpy(dest_addr, src_addr, n_bytes_to_copy);
								seg_table->size -= 1;
								--i;
							}
						} else {
							void* src_addr = &page_table->table[top];
							void* dest_addr = &page_table->table[j];
							int n_bytes_to_copy = ((void*)&page_table->table[page_table->size]) - src_addr;
							memcpy(dest_addr,src_addr,n_bytes_to_copy);
							page_table->size -= indexs_to_free;
							indexs_to_free = 0;
						}
						page_table = seg_table->table[++i].pages;
						j = 0;

					}
					break;
				}
			}
			break;
	    }
	}
	pthread_mutex_unlock(&mem_lock);
	return 0;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


