//Nicholas Delli Carpini

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>


typedef struct pteStruct { //struct for pte
	u_int8_t present;   // 0 if on disk | 1 if in mem
	u_int8_t valid;     // 0 if no value | 1 if value
	u_int8_t rw;        // 0 if rw | 1 if read only
	u_int8_t physical;  // physical frame number
} PTE;

typedef struct pidregStruct { // register struct
	int valid; // 0 if not allocated | 1 if allocated
	int offset;// location of the page table (if -1 on disk)
	int called; // last called
} reg;

unsigned char *memory; // memory
int *freeList; // free frames
reg *regPID; // register
FILE *pagingFile;

void createReg() { // initializes the register
	for (int i = 0; i < 4; i++) {
		regPID[i].valid = 0;
		regPID[i].offset = 69;
		regPID[i].called = 0;
	}
}
int findFree() { // finds a free frame
	for (int i = 0; i < 4; i++) {
		if (freeList[i] == 0) {
			return i;
		}
	}
	return -1;
}

int freeCheck() { // returns the number of free frames
	int x = 4;
	for (int i = 0; i < 4; i++) {
		if (freeList[i] == 1) {
			x--;
		}
	}
	return x;
}

unsigned char *clean() { // cleans an array
	unsigned char *temp = malloc(sizeof(unsigned char) * 16);
	for (int i = 0; i < 16; i++) {
		temp[i] = 0;
	}
	return temp;
}

void createPagingFile() { // creates the paging file
	unsigned char *temp = malloc(sizeof(unsigned char) * 324);
	for (int i = 0; i < 20; i++) {
		memcpy(temp + (i * 16), clean(), 16);
	}
	memcpy((temp + 320), clean(), 4);
	pagingFile = fopen("pagingFile", "w+");
	fwrite(temp, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
}

PTE *createTable() { // creates the page table
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	for (int i = 0; i < 4; i++) {
		pageTable[i].present = 69;
		pageTable[i].valid = 0;
		pageTable[i].rw = 69;
		pageTable[i].physical = 69;
	}
	return pageTable;
}

PTE *editTable(PTE *pageTable, int page, int valid, int value, int physical, int present) { // edits the table by keeping old values and changing new ones
	PTE *editedTable = malloc(sizeof(PTE) * 4);
	for(int i = 0; i < 4; i++) {
		editedTable[i].present = pageTable[i].present;
		editedTable[i].valid = pageTable[i].valid;
		editedTable[i].physical = pageTable[i].physical;
		editedTable[i].rw = pageTable[i].rw;
	}
	editedTable[page].present = present;
	editedTable[page].valid = valid;
	editedTable[page].physical = physical;
	editedTable[page].rw = value;
	return editedTable;
}


void swapOut(u_int8_t pid) { // swap out all pages and page tables for least used allocated pid
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	unsigned char *pid2swap = malloc(sizeof(unsigned char));
	unsigned char *table = malloc(sizeof(unsigned char) * 16);
	unsigned char *pages = malloc(sizeof(unsigned char) * 64);
	unsigned char *buffer = malloc(sizeof(unsigned char) * 324);
	unsigned char *cleaner = malloc(sizeof(unsigned char) * 16);
	int temp = 255;
	cleaner = clean();
	memcpy(table, cleaner, 16);
	for (int i = 0; i < 4; i++) {
		memcpy(pages + (i * 16), cleaner, 16);
	}
	for (int i = 0; i < 20; i++) {
		memcpy(buffer + (i * 16), cleaner, 16);
	}
	memcpy((buffer + 320), cleaner, 4);
	memcpy(pid2swap, cleaner, 1);
	for (int i = 0; i < 4; i++) {
		if ((regPID[i].valid == 1) && (regPID[i].called < temp) && (regPID[i].offset != -1) && (i != pid)) {
			pid2swap[0] = i;
			temp = regPID[i].called;
		}
	}
	memcpy(pageTable, (memory + regPID[pid2swap[0]].offset), 16);
	for (int i = 0; i < 4; i++) {
		if (pageTable[i].valid == 1 && pageTable[i].present == 1) {
			memcpy((pages + (i * 16)), (memory + (pageTable[i].physical * 16)), 16);
		}
	}
	freeList[(regPID[pid2swap[0]].offset) / 16] = 0;
	for (int i = 0; i < 4; i++) {
		if (pageTable[i].physical != 69) {
			freeList[pageTable[i].physical] = 0;
		}
	}
	memcpy((memory + regPID[pid2swap[0]].offset), cleaner, 16);
	for (int i = 0; i < 4; i++) {
		if (pageTable[i].valid == 1) {
			memcpy((memory + (pageTable[i].physical * 16)), cleaner, 16);
		}
	}
	pagingFile = fopen("pagingFile", "r");
	fread(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	for (int i = 0; i < 4; i++) {
		if (pageTable[i].present == 1) {
			memcpy((buffer + (pid2swap[0] * 80) + 16 + (i * 16)), (pages + (i * 16)), 16);
		}
	}
	for (int i = 0; i < 4; i++) {
		pageTable = editTable(pageTable, i, pageTable[i].valid, pageTable[i].rw, 69, 0);
	}
	memcpy((buffer + (pid2swap[0] * 80)), pid2swap, 1);
	memcpy(table, pageTable, 16);
	memcpy((buffer + (pid2swap[0] * 80)), table, 16);
	pagingFile = fopen("pagingFile", "w");
	fwrite(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	regPID[pid2swap[0]].offset = -1;
	printf("Swapped Out All Pages for Process %d\n", pid2swap[0]);
}

void swapOutPage(u_int8_t pid, int pageNum) { // swaps out page of same pid if more memory is needed
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	unsigned char *page = malloc(sizeof(unsigned char) * 16);
	unsigned char *buffer = malloc(sizeof(unsigned char) * 324);
	unsigned char *cleaner = malloc(sizeof(unsigned char));
	cleaner = clean();
	memcpy(page, cleaner, 16);
	for(int i = 0; i < 20; i++) {
		memcpy(buffer + (i * 16), cleaner, 16);
	}
	memcpy((buffer + 320), cleaner, 4);
	memcpy(pageTable, (memory + regPID[pid].offset), 16);
	if (pageTable[pageNum].present == 1) {
		memcpy(page, (memory + (pageTable[pageNum].physical * 16)), 16);
		freeList[pageTable[pageNum].physical] = 0;
		memcpy((memory + (pageTable[pageNum].physical * 16)), cleaner, 16);
	}
	pageTable = editTable(pageTable, pageNum, pageTable[pageNum].valid, pageTable[pageNum].rw, 69, 0);
	pagingFile = fopen("pagingFile", "r");
	fread(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	memcpy((buffer + (pid * 80) + 16 + (pageNum * 16)), page, 16);
	pagingFile = fopen("pagingFile", "w");
	fwrite(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	memcpy((memory + regPID[pid].offset), pageTable, 16);
	printf("Swapped Out Page %d of Process %d\n", pageNum, pid);
}

void swapInPage(u_int8_t pid, int pageNum) { //swaps in a requested page if pagetable is already in memory
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	unsigned char *page = malloc(sizeof(unsigned char) * 16);
	unsigned char *buffer = malloc(sizeof(unsigned char) * 324);
	unsigned char *cleaner = malloc(sizeof(unsigned char));
	int freePage;
	cleaner = clean();
	memcpy(page, cleaner, 16);
	for(int i = 0; i < 20; i++) {
		memcpy(buffer + (i * 16), cleaner, 16);
	}
	memcpy((buffer + 320), cleaner, 4);
	memcpy(pageTable, (memory + regPID[pid].offset), 16);
	if (pageTable[3].valid == 1  && freeCheck() < 1) {
		if (pageTable[3].present == 1 && pageNum < 2) {
			swapOutPage(pid, 3);
		} else if (pageTable[0].present == 1 && pageNum > 1){
			swapOutPage(pid, 0);
		}
	}
	while (freeCheck() < 1) {
		swapOut(pid);
	}
	pagingFile = fopen("pagingFile", "r");
	fread(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	freePage = findFree();
	freeList[freePage] = 1;
	memcpy(pageTable, (memory + regPID[pid].offset), 16);
	pageTable = editTable(pageTable, pageNum, pageTable[pageNum].valid, pageTable[pageNum].rw, freePage, 1);
	memcpy((memory + regPID[pid].offset), pageTable, 16);
	memcpy(page, (buffer + (pid * 80) + 16 + (pageNum * 16)), 16);
	memcpy((memory + (freePage * 16)), page, 16);
	pagingFile = fopen("pagingFile", "w");
	fwrite(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	printf("Swapped In Page %d of Process %d (Physical Frame %d)\n", pageNum, pid, freePage);
}

void swapIn(u_int8_t pid, int pageNum) { // swaps in requested page table and page for pid
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	int freePTE;
	int freePage;
	unsigned char *page = malloc(sizeof(unsigned char) * 16);
	unsigned char *buffer = malloc(sizeof(unsigned char) * 324);
	unsigned char *cleaner = malloc(sizeof(unsigned char) * 16);
	cleaner = clean();
	for (int i = 0; i < 4; i++) {
		memcpy(page + (i * 16), cleaner, 16);
	}
	for (int i = 0; i < 20; i++) {
		memcpy(buffer + (i * 16), cleaner, 16);
	}
	memcpy((buffer + 320), cleaner, 4);
	while(freeCheck() < 2) {
		swapOut(pid);
	}
	pagingFile = fopen("pagingFile", "r");
	fread(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	freePTE = findFree();
	freeList[freePTE] = 1;
	regPID[pid].offset = (freePTE * 16);
	freePage = findFree();
	freeList[freePage] = 1;
	memcpy(pageTable, (buffer + (pid * 80)), 16);
	pageTable = editTable(pageTable, pageNum, pageTable[pageNum].valid, pageTable[pageNum].rw, freePage, 1);
	memcpy((memory + regPID[pid].offset), pageTable, 16);
	memcpy(page, (buffer + (pid * 80) + 16 + (pageNum * 16)), 16);
	memcpy((memory + (freePage * 16)), page, 16);
	pagingFile = fopen("pagingFile", "w");
	fwrite(buffer, sizeof(unsigned char), 324, pagingFile);
	fclose(pagingFile);
	printf("Swapped In Page Table for Process %d (Physical Frame %d)\n", pid, freePTE);
	printf("   Swapped In Page %d (Physical Frame %d)\n", pageNum, freePage);
}

int map(int pid, int va, int rw) {
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	int freePTE;
	int freeMem;
	int pageNum = (va / 16) + 1;
	int offset;
	int curPages;
	int startPage = 0;
	memcpy(pageTable, (memory + regPID[pid].offset), 16);
	if (regPID[pid].valid == 1) {
		if (regPID[pid].offset == -1) {
			swapIn((u_int8_t) pid, (va / 16));
			memcpy(pageTable, (memory + regPID[pid].offset), 16);
		}
		if (regPID[pid].offset > -1 && pageTable[va / 16].valid == 1) {
			if (pageTable[va / 16].present == 0) {
				swapInPage((u_int8_t) pid, (va / 16));
				memcpy(pageTable, (memory + regPID[pid].offset), 16);
			}
			if (pageTable[va / 16].rw == rw) {
				if (rw == 1) {
					printf("Page %d of PID %d is Already Read-Only\n", (va / 16), pid);
				} else {
					printf("Page %d of PID %d is Already Read/Write\n", (va / 16), pid);
				}
				return -1;
			} else {
				pageTable = editTable(pageTable, (va / 16), 1, rw, pageTable[va / 16].physical, 1);
				memcpy((memory + regPID[pid].offset), pageTable, 16);
				if (rw == 1) {
					printf("Page %d of PID %d is Now Read-Only\n", (va / 16), pid);
				} else {
					printf("Page %d of PID %d is Now Read/Write\n", (va / 16), pid);
				}
				regPID[pid].called++;
				return 0;
			}
		} else {
			for (int i = 0; i < 4; i++) {
				if (pageTable[i].valid == 1) {
					curPages++;
				}
			}
			if (pageNum < 4) {
				while (pageNum > (freeCheck() + curPages + 1)) {
					swapOut((u_int8_t) pid);
				}
			} else {
				while (pageNum > (freeCheck() + curPages + 1)) {
					swapOut((u_int8_t) pid);
				} if (regPID[pid].offset > -1) {
					swapOutPage((u_int8_t) pid, 0);
				}
				memcpy(pageTable, (memory + regPID[pid].offset), 16);
				printf("Adding Pages to PID %d (Table Physical Frame %d)\n", pid, (regPID[pid].offset / 16));
				for (int i = curPages; i < 4; i++) {
					freeMem = findFree();
					freeList[freeMem] = 1;
					printf("   Page %d has been Added to Physical Frame %d\n", i, freeMem);
					pageTable = editTable(pageTable, i, 1, rw, freeMem, 1);
				}
				memcpy((memory + regPID[pid].offset), pageTable, 16);
				regPID[pid].called++;
				return 0;
			}
			printf("Adding Pages to PID %d (Table Physical Frame %d)\n", pid, (regPID[pid].offset / 16));
			for (int i = 0; i < 4; i++) {
				if (pageTable[i].valid == 1) {
					startPage++;
				}
			}
			for (int i = startPage;i < pageNum; i++) {
				freeMem = findFree();
				freeList[freeMem] = 1;
				printf("   Page %d has been Added to Physical Frame %d\n", i, freeMem);
				pageTable = editTable(pageTable, i, 1, rw, freeMem, 1);
			}
			printf("   Virtual Address %d has been Mapped to Physical Frame %d (Page %d)\n", va, freeMem, (va / 16));
			memcpy((memory + regPID[pid].offset), pageTable, 16);
			regPID[pid].called++;
			return 0;
		}
	}
	if (pageNum == 4) {
		while (pageNum > freeCheck()) {
			swapOut((u_int8_t) pid);
		}
		freePTE = findFree();
		freeList[freePTE] = 1;
		pageTable = createTable();
		printf("Page Table for PID %d has been Mapped to Physical Frame %d\n", pid, freePTE);
		for (int i = 1; i < pageNum; i++) {
			freeMem = findFree();
			freeList[freeMem] = 1;
			printf("   Page %d has been Mapped to Physical Frame %d\n", i, freeMem);
			pageTable = editTable(pageTable, i, 1, rw, freeMem, 1);
		}
		printf("   Virtual Address %d has been Mapped to Physical Frame %d (Page %d)\n", va, freeMem, (va / 16));
		offset = freePTE * 16;
		pageTable = editTable(pageTable, 0, 1, rw, 69, 0);
		swapOutPage((u_int8_t) pid, 0);
		memcpy((memory + offset), pageTable, 16);
		regPID[pid].valid = 1;
		regPID[pid].offset = offset;
		regPID[pid].called++;
		return 0;
	} else {
		while ((pageNum + 1) > freeCheck()) {
			swapOut((u_int8_t) pid);
		}
	}
	freePTE = findFree();
	freeList[freePTE] = 1;
	pageTable = createTable();
	printf("Page Table for PID %d has been Mapped to Physical Frame %d\n", pid, freePTE);
	for (int i = 0; i < pageNum; i++) {
		freeMem = findFree();
		freeList[freeMem] = 1;
		printf("   Page %d has been Mapped to Physical Frame %d\n", i, freeMem);
		pageTable = editTable(pageTable, i, 1, rw, freeMem, 1);
	}
	printf("   Virtual Address %d has been Mapped to Physical Frame %d (Page %d)\n", va, freeMem, (va / 16));
	offset = freePTE * 16;
	memcpy((memory + offset), pageTable, 16);
	regPID[pid].valid = 1;
	regPID[pid].offset = offset;
	regPID[pid].called++;
	return 0;
}

int store(int pid, int va, u_int8_t value) {
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	int pa;
	if (regPID[pid].valid == 0) {
		printf("Error: PID does not have a page\n");
		return -1;
	}
	if (regPID[pid].offset == -1) {
		swapIn((u_int8_t) pid, (va / 16));
		memcpy(pageTable, (memory + regPID[pid].offset), 16);
	}
	memcpy(pageTable, (memory + regPID[pid].offset), 16);
	if (pageTable[va / 16].present != 1) {
		swapInPage((u_int8_t) pid, (va / 16));
		memcpy(pageTable, (memory + regPID[pid].offset), 16);
	}
	if (pageTable[va / 16].valid == 0) {
		printf("Error: Invalid Virtual Address\n");
		return -1;
	}
	if (pageTable[va / 16].rw == 1) {
		printf("Error: Virtual Address is Read-Only\n");
		return -1;
	}
	pa = (va % 16) + (pageTable[va / 16].physical * 16);
	memcpy((memory + pa), &value, 1);
	printf("Stored Value %d at Virtual Address %d (Physical Address %d)\n", value, va, pa);
	regPID[pid].called++;
	return 0;
}

int load(int pid, int va, int value) {
	PTE *pageTable = malloc(sizeof(PTE) * 4);
	int pa;
	if (regPID[pid].valid == 0) {
		printf("Error: PID does not have a page\n");
		return -1;
	}
	if (regPID[pid].offset == -1) {
		swapIn((u_int8_t) pid, (va / 16));
		memcpy(pageTable, (memory + regPID[pid].offset), 16);
	}
	memcpy(pageTable, (memory + regPID[pid].offset), 16);
	if (pageTable[va / 16].present != 1) {
		swapInPage((u_int8_t) pid, (va / 16));
		memcpy(pageTable, (memory + regPID[pid].offset), 16);
	}
	if (pageTable[va / 16].valid == 0) {
		printf("Error: Invalid Virtual Address\n");
		return -1;
	}
	pa = (va % 16) + (pageTable[va / 16].physical * 16);
	int val = memory[pa];
	printf("The Value %d was Loaded from Virtual Address %d (Physical Address %d)\n", val, va, pa);
	regPID[pid].called++;
	return 0;
}

int main () {
	char *input = malloc(sizeof(char) * 32);
	memory = malloc(sizeof(unsigned char) * 64);
	regPID = malloc(sizeof(reg) * 4);
	freeList = malloc(sizeof(int) * 4);
	for (int i = 0; i < 4; i++) {
		freeList[i] = 0;
	}
	char *exit = "exit\n";
	char *mapCheck = "map";
	char *storeCheck = "store";
	char *loadCheck = "load";
	char *debug = "mem\n";
	char *debug2 = "file\n";
	int PID;
	int VA;
	int value;
	createReg();
	createPagingFile();
	printf("\nPlease Enter Instructions");
	while(1) {
		printf("\nFormat: PID,Instruction,Virtual Address,Value\n");
		printf("To Quit, Type 'Exit'\n");
		printf("Input: ");
		fgets(input, 32, stdin);
		printf("\n");
		if (strcasecmp(input, exit) == 0 || feof(stdin)) {
			printf("Exiting...\n");
			remove("pagingFile");
			free(input);
			return 0;
		}
		if (strcasecmp(input, debug) == 0) {
			for (int i = 0; i < 64; i++) {
				printf("%d\n", memory[i]);
			}
			continue;
		}
		if (strcasecmp(input, debug2) == 0) {
			unsigned char *buffer = malloc(sizeof(unsigned char) * 324);
			pagingFile = fopen("pagingFile", "r");
			fread(buffer, sizeof(unsigned char), 324, pagingFile);
			for (int i = 0; i < 324; i++) {
				printf("%d\n", buffer[i]);
			}
			fclose(pagingFile);
			continue;
		}
		char *cleanInput[32];
		char *temp;
		char **i = cleanInput;
		temp = strtok(input, ",");
		while (temp != NULL) {
			*i++ = temp;
			temp = strtok(NULL, ",");
		}
		cleanInput[3][strcspn(cleanInput[3], "\n")] = '\0';
		if (atoi(cleanInput[0]) == 0) {
			if (strcmp(cleanInput[0], "0") == 0) {
				PID = atoi(cleanInput[0]);
				if (PID < 0 || PID > 3) {
					printf("Error: PID is Out of Bounds [0-3]\n");
					continue;
				}
			} else {
				printf("Error: Invalid Input for PID\n");
				continue;
			}
		} else {
			PID = atoi(cleanInput[0]);
			if (PID < 0 || PID > 3) {
				printf("Error: PID is Out of Bounds [0-3]\n");
				continue;
			}
		}
		if (atoi(cleanInput[2]) == 0) {
			if (strcmp(cleanInput[2], "0") == 0) {
				VA = atoi(cleanInput[2]);
				if (VA < 0 || VA > 63) {
					printf("Error: Virtual Address is Out of Bounds [0-63]\n");
					continue;
				}
			} else {
				printf("Error: Invalid Input for Virtual Address\n");
				continue;
			}
		} else {
			VA = atoi(cleanInput[2]);
			if (VA < 0 || VA > 63) {
				printf("Error: Virtual Address is Out of Bounds [0-63]\n");
				continue;
			}
		}
		if (atoi(cleanInput[3]) == 0) {
			if (strcmp(cleanInput[3], "0") == 0) {
				value = atoi(cleanInput[3]);
				if (value < 0 || value > 255) {
					printf("Error: Value is Invalid [0-255]\n");
					continue;
				}
			} else {
				printf("Error: Invalid Input for Value\n");
				continue;
			}
		} else {
			value = atoi(cleanInput[3]);
			if (value < 0 || value > 255) {
				printf("Error: Value is Invalid [0-255]\n");
				continue;
			}

		}
		if (strcasecmp(cleanInput[1], mapCheck) == 0) {
			if (value > 1) {
				printf("Error: When Mapping, Value [0 or 1]\n");
				continue;
			}
			map(PID, VA, value);
			continue;
		} else if (strcasecmp(cleanInput[1], storeCheck) == 0) {
			store(PID, VA, (u_int8_t) value);
			continue;
		} else if (strcasecmp(cleanInput[1], loadCheck) == 0) {
			load(PID, VA, 0);
			continue;
		} else {
			printf("Error: Invalid Input for Instruction [map, store, or load]\n");
			continue;
		}
	}
	free(input);
	return 0;
}


