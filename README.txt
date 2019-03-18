Nicholas Delli Carpini

----- Running -----
The program runs as expected in the syllabus. The only thing is make sure there are no spaces in the input or it won't work. Also,
they were incorporated for my convenience, but there are 2 additional commands named 'mem' and 'file' that spit out the data of memory
and the paging file respectively.

----- Global Structs -----
unsigned char *memory; // memory
int *freeList; // free frames
reg *regPID; // register
FILE *pagingFile;

----- Functions -----
void createReg() { // initializes the register
int findFree() { // finds a free frame
int freeCheck() { // returns the number of free frames
unsigned char *clean() { // cleans an array
void createPagingFile() { // creates the paging file
PTE *createTable() { // creates the page table
PTE *editTable(PTE *pageTable, int page, int valid, int value, int physical, int present) { // edits the table by keeping old values and changing new ones
void swapOut(u_int8_t pid) { // swap out all pages and page tables for least used allocated pid
void swapOutPage(u_int8_t pid, int pageNum) { // swaps out page of same pid if more memory is needed
void swapInPage(u_int8_t pid, int pageNum) { //swaps in a requested page if pagetable is already in memory
void swapIn(u_int8_t pid, int pageNum) { // swaps in requested page table and page for pid

----- Issues -----
No issues were observed

----- Notes -----
There wasn't a lot of time to clean this code up, so its pretty messy; however, it worked throughout all of my testing so I was not
going to touch it with such little time remaining.