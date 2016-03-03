#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h>
#define INITIAL_PAGE_VALUE 5

uint32_t ssd[100];
uint32_t ram[25];
uint32_t hdd[1000];
sem_t binsem;

typedef struct {
	int memflag; // 1 for ram, 2 for ssd and 3 for disk
	int modified;
	int valid;
	int index;
	int newflag;

}page_table;



typedef struct {
	signed short add;
}vAddr;
page_table ptable[1000];



vAddr create_page();
uint32_t * get_value(vAddr address);
void store_value (vAddr address, uint32_t *value);
void bring_from_ssd(vAddr address, uint32_t *value);
void bring_from_hdd (vAddr address, uint32_t *value);
void memoryMaxer();
int evict_from_ram();
page_table *findentry(int index, int mem);
int evict_from_ssd();
int evict_from_hdd();
int page_eviction_algorithm1(int memflag);
int page_eviction_algorithm2(int memflag);
int page_eviction_algorithm3(int memflag);
//void updatenflag(int addr);
//Creates a page and stores it in the ram. If ram is full, calls evict_from_ram() to have pages from ram evicted to lower heiarchies
void init();
void free_page(vAddr address);
void updatenflag(int addr);


void updatenflag(int addr) {
	
	int i = 0;
	for (i = 0; i < 1000; i++) {
		if ((i!=addr)&& (ptable[i].valid==1)) {
			ptable[i].newflag++;
		}
	}

}



void init() {
	
	int i;
	for(i = 0; i < 1000 ; i++) {
		ptable[i].index = -1;
		ptable[i].memflag = 0;
		ptable[i].modified = 0;
		ptable[i].valid = 0;
		ptable[i].newflag = 0;
	}

	for(i = 0; i < 25; i++) {
		ram[i] = -1;
	}
	for(i = 0; i < 100; i++) {
		ssd[i] = -1;
	}
	for(i = 0; i < 1000; i++) {
		hdd[i] = -1;
	}
}
			
vAddr create_page() {
	
	sem_wait(&binsem);
	printf("\n\n CREATION OF A NEW PAGE BEGAN!\n");
	int index = -1;
	int flag = 0;
	int addr = -1;
	int test = 0;
	int i = 0;
	//Finding an empty spot in the page table
	for(i = 0; i < 1000; i++) {
		if(ptable[i].index == -1) {
				addr = i;
				test = 1;
				break;
		}
	}

	if (test == 0) {
		printf("No space left in the page table, hardware issue! ECEEE!!!\n");
		exit(1);  //error: page entry exceeded
	}


	//Finding an empty spot in ram
	for(i = 0; i < 25; i++) {
		if(ram[i] == -1) {
			index = i;
			flag = 1;
			break;
		}
	}
	
	//if not found, call evict_from_ram which will empty a position in the ram and return the index
	if(flag == 0) {
		index = evict_from_ram(); //no space in ram
	}

	//index is the position where the new page entry should be stored;
	ram[index] = INITIAL_PAGE_VALUE;  
	printf("New page entry stored in ptable address at %d\n", addr);
	printf("New page entry stored in ram at index value %d\n", index);
	ptable[addr].memflag = 1;
	ptable[addr].valid = 1;
	ptable[addr].index = index;
	ptable[addr].newflag = 1;
	updatenflag(addr); //updates all flag values but addr
	ptable[addr].modified = 1;
	vAddr adr;
	adr.add = addr;
	sem_post(&binsem);
	return adr;

}

uint32_t * get_value(vAddr address) {
	
	sem_wait(&binsem);
	int address1 = address.add;
	//Are you kidding me? Check the page table's size
	if (address1 > 999 && address1 < 0) {
		printf("Address out of bound!\n");
		sem_post(&binsem);
		return NULL;

	}
	
	//Page table not initialized
	if(ptable[address1].valid == 0) {
		printf("Page doesn't even exist! What do you want?\n");
		sem_post(&binsem);
		return NULL;
	}

	printf("Trying to get the value of address %d\n ", address1);

	ptable[address1].modified++;

	//If stored in ram, getting the value
	if(ptable[address1].memflag == 1) {
		sem_post(&binsem);
		return &ram[ptable[address1].index];	
	}

	//If stored in ssd, getting the value
	if(ptable[address1].memflag == 2) {
		sem_post(&binsem);
		return &ssd[ptable[address1].index];
	}
	
	//If stored in hdd, getting the value
	if(ptable[address1].memflag == 3) {
		sem_post(&binsem);
		return &hdd[ptable[address1].index];
	}

}

void store_value (vAddr address, uint32_t *value) {

	sem_wait(&binsem);
	//Out of bound
	int address1 = address.add;
	if (address1 > 999 && address1 < 0) {
		printf("Error: cannot find the address");
		sem_post(&binsem);
		return;
	}

	//Page doesnt exist
	if (ptable[address1].valid == 0) {
		printf("Error: cannot find the address");
		sem_post(&binsem);
		return;
	}

	printf("Trying to store the value %d at address %d in the page table\n", *value, address1);

	ptable[address1].modified++;

	//The page is in the ram, so just store the value
	if (ptable[address1].memflag == 1) {
	
		usleep(0.01*1000000);	
		printf("The page request at address %d seems to be in ram, changing the value!!\n", address1);
		ram[ptable[address1].index] = *value;
		sem_post(&binsem);
		return;
	}

	//The page is in the ssd, so calling bring_from_ssd, which will store the value by finding a place in ssd
	if (ptable[address1].memflag == 2) {
		bring_from_ssd(address, value);
		//This function will find the value in ssd and store the value after bringing it from ssd
		sem_post(&binsem);
		return;
	}

	//The page is in the hdd, so calling bring_from_hdd, which will store the value by finding a place in hdd
	if (ptable[address1].memflag == 3) {
		bring_from_hdd(address, value);
		//This function will find the value in hdd and store the value after bringing it from ssd
		sem_post(&binsem);
		return;
	}
	
	//if the function is still running at this point, something's wrong
	printf("Error occured while storing value");
	sem_post(&binsem);
	return;
}


void free_page (vAddr address) {
	
	sem_wait(&binsem);
	int address1 = address.add;
	printf("freeing page\n");
	if(ptable[address1].memflag == 1) {

		ram[ptable[address1].index] = -1;
	}

	if(ptable[address1].memflag == 2) {
		
		ssd[ptable[address1].index] = -1;

	}

	if(ptable[address1].memflag == 3) {
		
		hdd[ptable[address1].index] = -1;
	}

	ptable[address1].index = -1;
	ptable[address1].memflag = 0;
	ptable[address1].modified = 0;
	ptable[address1].valid = 0;
	ptable[address1].newflag = 0;
	sem_post(&binsem);
}



//This function will store the value of the page entry at address address. It might evict an entry from ram to create space for it 
void bring_from_ssd(vAddr address, uint32_t *value) {

	usleep(0.1*1000000);	
	int address1 = address.add;
	printf("The page request at address %d seems to be in ssd, bringing from ssd step by step\n", address1);
	int i = 0;
	int flag = 0;
	int index = -1;
	for( i = 0; i < 25; i++) {
		if (ram[i] == -1) {
			flag = 1;
			index = i;
			break;
		}
	}

	if (flag  == 0) {
		index = evict_from_ram();   //ram will be evicted and the address will be stored in the ssd
	}
	
	printf("Storing the value in RAM now that it has been brought from ssd\n");
	ram[index] = *value;   //index will be the position where ram is freed
	ptable[address1].memflag = 1;
	ptable[address1].index = index;
}


//This function will bring the page entry from the hard disk and then store it in ssd. If ssd is not free, it will call evict_from_ssd and find the freed index. The function will then store in the 
// ssd and then call bring_from_ssd to send the page entry from ssd to ram
void bring_from_hdd (vAddr address, uint32_t *value) {

	usleep(2.5*1000000);	
	int address1 = address.add;
	printf("The page request at address %d seems to be in hdd, bringing from hdd step by step\n", address1);
	int i = 0;
	int flag = 0;
	int index = -1;
	for (i = 0; i < 100 ; i++) {
		if (ssd[i] == -1) {
			flag = 1;
			index = i;
			break;
		}
	}

	if (flag == 0) {
		index = evict_from_ssd( ); //evict a page entry from ssd and then store the evicted page table in hdd
	}

	//TODO: ssd[index] = *value;

	printf("Sending the value to ssd so that it can proceed\n");
	ptable[address1].memflag = 2;
	ptable[address1].index = index;
	bring_from_ssd(address, value);  //now that it is stored in ssd, bringing the page table entry from ssd to ram by calling the function
}



void memoryMaxer() {
	vAddr indexes[1000];
	int i = 0;
	for (i = 0; i < 128; ++i) {
		indexes[i] = create_page();
		int *value = get_value(indexes[i]);
		*value = (i * 3);
		store_value(indexes[i], value);
	}
	
	for (i = 0; i < 128; ++i) {
		free_page(indexes[i]);
	}
}


//This function will find the index to be evicted by calling page_eviction_algorithm and then store it in sdd by clearing out ssd if needed. 
int evict_from_ram() {


	usleep(0.01*1000000);	
	printf("No space in ram, evicting a page!\n");
	int index = page_eviction_algorithm1(1);
	int i = 0;
	int nindex;
	int flag = 0;
	for(i = 0; i < 100; i++) {

		if (ssd[i] == -1) {
			nindex = i;
			flag = 1;
			break;
		}
	}

	if (flag == 0) {

		nindex = evict_from_ssd();
	}
	
	usleep(0.1*1000000);	
	//contents of ram at index is being transferred to ssd at nindex
	ssd[nindex] = ram[index];
	page_table *st;
	
	//finding the page entry that was transferred
	st = findentry(index, 1);

	//changing the values of the page entry that was transferred
	st->memflag = 2;
	st->index = nindex;
	ram[index] = -1;
	return index;
}


//finds the page table entry which is stored in mem at index and returning the page table entry
page_table *findentry(int index, int mem){

	int i = 0; 
//	printf ("index = %d, mem = %d\n", index, mem);
	for(i = 0; i < 1000; i++) {
		if((ptable[i].index == index) && (ptable[i].memflag == mem))
		{
			return &ptable[i];
		}
	//	printf("i1 = %d , me1 = %d\n", ptable[i].index, ptable[i].memflag);
	}

	printf( "Cannot find value, exiting \n");
	exit(1);
}



int evict_from_ssd() {

	printf("No space in ssd, evicting a page!\n");

	int index = page_eviction_algorithm1(2);
	int i = 0;
	int nindex;
	int flag = 0;
	for (i = 0; i < 1000; i++) {

		if (hdd[i] == -1) {
			nindex = i;
			flag = 1;
			break;
		}
	}

	if (flag == 0) {

		nindex = evict_from_hdd();
	}

	usleep(2.5*1000000);
	hdd[nindex] = ssd[index];
	page_table *st;
	st = findentry(index, 2);
	st->memflag = 3;
	st->index = nindex;
	ssd[index] = -1;
	return index;
}


int evict_from_hdd() {

	printf("No space in hdd, discarding a page! \n");
	
	int index = page_eviction_algorithm1(3);
	printf("Discarding page with value %d at position %d in hdd\n", hdd[index], index);
	hdd[index] = -1;
	return index;
}


//Random page eviction algorithm
int page_eviction_algorithm1(int memflag) {
	
	//returns the index of the page evicted

	if ( memflag == 1) {
		int r = rand()%25;
		printf("Removing a random page, random page eviction algorithm says remove %d index\n", r);
		return r;
	}

	if (memflag == 2) {
		int r = rand()%100;
		printf("Removing a random page, random page eviction algorithm says remove %d index\n", r);
		return r;
	}

	if (memflag == 3) {
		int r = rand()%1000;
		printf("Removing a random page, random page eviction algorithm says remove %d index\n", r);
		return r;
	}

	printf("Error!, proper memory not found!\n");
	exit(1);

}

int main () {
	
	if ( (sem_init (&binsem, 0, 1)) != 0) {
		printf("\n Error creating semaphore\n");
		exit(1);
	}

	srand(time(NULL));
	clock_t start, end;
	double cpu_time_used;

	start = clock();

	
	init();
	memoryMaxer();

	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	

	printf("\n\n TOTAL TIME TAKEN IS: %g \n\n", cpu_time_used);
	sem_destroy(&binsem);	
	return 0;

}




