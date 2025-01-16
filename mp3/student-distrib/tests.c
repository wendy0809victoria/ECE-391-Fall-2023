#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "rtc.h"
#include "keyboard.h"
#include "terminal.h"
#include "cursor.h"
#include "filesystem.h"    
#include "system_calls.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}
/*
 * Divide by zero test
 * Inputs: None
 * Outputs: FAIL
 * Side Effects: None
 * Coverage: Divide by zero
 * Files: idt.h/c and assembly_linkage.h/S 
*/
int divide_zero_test(){
	TEST_HEADER;
	int a = 0;
	int b;
	b = 1/a;
	return FAIL;
}

/* Page Dereferencing Test
 * Dereferences each address in video memory and kernel space.
 * Returns PASS if all addresses are successfully dereferenced.
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Paging
 * Files: page.c/h
 */
int page_dereferencing_test() 
{
	TEST_HEADER;
	unsigned char* video_mem_start = (unsigned char*) 0xB8000;
	unsigned char* video_mem_end   = (unsigned char*) 0xB9000;
	unsigned char* kernel_start    = (unsigned char*) 0x400000;
	unsigned char* kernel_end      = (unsigned char*) 0x800000;
	unsigned char  temp;
	unsigned char* i;
	for (i = video_mem_start; i < video_mem_end; i++) 
	{
		temp = (*i);
	}
	for (i = kernel_start; i < kernel_end; i++) 
	{
		temp = (*i);
	}
	return PASS;
}
/*
*page_derefer_inaccessible_test
* Dereferences an address that is not present in the page.
* Returns FAIL if the address is successfully dereferenced.
* Inputs: None
* Outputs: FAIL
* Side Effects: None
*/
int page_derefer_inaccessible_test()
{
	TEST_HEADER;
	unsigned char temp;
	unsigned char* inaccessible_addr = (unsigned char*) 0x800001;
	temp = (*inaccessible_addr);
	return FAIL;
}

/*
*deref_null_test
* Dereferences a null pointer.
* Returns FAIL if the address is successfully dereferenced.
* Inputs: None
* Outputs: FAIL
* Side Effects: None
*/
int deref_null_test()
{
	TEST_HEADER;
	unsigned char temp;
	unsigned char* null_addr = NULL;
	temp = (*null_addr);
	return FAIL;
}

/*
* pic_test
* Tests the pic by enable and disable the IRQs.
* Returns PASS if the enable and disable functions work.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int pic_test(){
    TEST_HEADER;
    int result = PASS;
    enable_irq(1);
    enable_irq(8);
   if(inb(MASTER_8259_PORT+1) != 0xF9 && inb(SLAVE_8259_PORT+1) != 0xFE)
   {
	   result = FAIL;
   }           
    disable_irq(1);
    disable_irq(8);
	if(inb(MASTER_8259_PORT+1) != 0xFB && inb(SLAVE_8259_PORT+1) != 0xFF)
	{
		result = FAIL;
	}
    return result;
}
/*
* idt_test2
* Tests the idt by calling the exceptions, interrupts and system calls.
* Returns FAIL if the interrupt handlers are not called.
*/
int idt_test2()
{
	TEST_HEADER;
	// one at a time to test
	// debug exception
	asm volatile("int $1");
	// breakpoint exception
	// asm volatile("int $3");
	// nmi interrupt
	// asm volatile("int $2");
	// system call
	// asm volatile("int $128");
	return PASS;
}
// add more tests here

/* Checkpoint 2 tests */

/*
* rtc_driver_test
* Tests RTC_read and RTC_write functions by directly calling them.
* Changing RTC frequencies, receiving RTC interrupts at each possible frequency
* and counting the number of interrupts received.
* Inputs: none
* Outputs: PASS/FAIL
* Side Effects: None
*/
int rtc_driver_test() {
    TEST_HEADER;
	printf("RTC DRIVER TEST");

    int freq, rate, count;
    for (freq = 2, rate = 0; freq <= 1024; freq <<= 1, rate++) {
        // Test RTC interrupt continuity
        RTC_write(0, &freq, 4);
        for (count = 0; count < freq; count++) {
            RTC_read(0, NULL, 0);
            printf("0");
        }

        printf(" - RTC interrupt count for frequency %d: ", freq);

        // Test RTC interrupt count
        RTC_write(0, &freq, 4);
        int interrupt_count = 0;
        for (count = 0; count < freq; count++) {
            RTC_read(0, NULL, 0);
            interrupt_count++;
        }
        printf("%d\n", interrupt_count);
    }

    return PASS;
}

/*
* rtc_open_and_close
* Tests RTC_open and RTC_close functions by directly calling them.
* Inputs: none
* Outputs: PASS/FAIL
* Side Effects: None
*/
int rtc_open_and_close() {
	TEST_HEADER;
	printf("RTC OPEN AND CLOSE TEST");

	// 0 - currently we are not using the arguments passed to RTC
	RTC_open(NULL);
	RTC_close(0);

	return PASS;
}

/*
* terminal_function
* Tests terminal_open, terminal_read, terminal_write, and terminal_close functions by directly calling them.
* Tests scrolling and handling overflow functionalities.
* Inputs: none
* Outputs: PASS/FAIL
* Side Effects: None
*/
int terminal_function() {
	TEST_HEADER;
	printf("TERMINAL OPEN AND CLOSE TEST\n");

    terminal_open(NULL);

	// 128 - maximum character number in a buffer (including '\n')
	char buf[128];
	int i;
	int32_t byte;
	int32_t byte_1;
	int32_t byte_2;

	while (1) {

		// 128 - maximum character number in a buffer (including '\n')
		byte = terminal_read(0, buf, 128);

		// If user types 'q', the return byte number should be 2 including '\n'
		if (buf[0] == 'q' && byte == 2) {
			clear();
			break;
		}
	}

	printf("Scrolling test\n");

	// 5000 - exceeding NUM_COLS * NUM_ROWS
	for (i = 0; i < 5000; i++){
		putc('1', 1);
	}

	putc('\n', 1);

	while (1) {

		// 128 - maximum character number in a buffer (including '\n')
		byte = terminal_read(0, buf, 128);

		// If user types 'q', the return byte number should be 2 including '\n'
		if (buf[0] == 'q' && byte == 2) {
			clear();
			break;
		}
	}

	printf("Overflow test\n");
	while (1) {

		// 129 - exceeding maximum character number in a buffer (including '\n')
		byte = terminal_read(0, buf, 129);

		// 128 - maximum character number in a buffer (including '\n')
		if (byte == 128) {
			clear();
			break;
		}
	}

	printf("Input test\n");
	while (1) {

		// 5 - char number in a buffer (including '\n')
		byte = terminal_read(0, buf, 5);

		// 5 - char number in a buffer (including '\n')
		if (byte == 5) {
			clear();
			break;
		}
	}

	printf("Keyboard test\n");

	// 3 - number of characters read including '\n'
	byte_1 = terminal_read(0, buf, 3);

	while (1) {

		// 3 - number of characters written
		byte_2 = terminal_write(0, buf, 3);
		putc('\n', 1);
		break;
	}

	printf("Writing test 1\n");

	// 3 - number of characters read including '\n'
	byte_1 = terminal_read(0, buf, 3);

	while (1) {

		// 4 - number of characters written
		byte_2 = terminal_write(0, buf, 4);
		putc('\n', 1);
		break;
	}

	printf("Writing test 2\n");

	// 3 - number of characters read including '\n'
	byte_1 = terminal_read(0, buf, 3);

	while (1) {

		// 128 - number of characters written
		byte_2 = terminal_write(0, buf, 128);
		putc('\n', 1);
		break;
	}

	terminal_close(0);
    return PASS;
}

/*
* terminal_function
* Tests terminal_read and terminal_write functions by directly calling them.
* Tests reading, writing and clearing functionalities.
* Inputs: none
* Outputs: PASS/FAIL
* Side Effects: None
*/
int terminal_driver_test(void) {
    clear();

	// 0 - the start of the screen
    update_cursor(0, 0);

    TEST_HEADER;
	printf("TERMINAL DRIVER TEST");

    uint8_t buffer[128];					// 128 - character(kb)

	// 128 - maximum character number in a buffer (including '\n')
	// 0 - return value for success but no bytes read
    if (terminal_read(0, buffer, 128) == 0){
		printf("\nReading fails\n");
		return FAIL;
	}

	// 128 - maximum character number in a buffer (including '\n')
	// < 0 - return value for unsuccessful writing
    if ((terminal_write(0, buffer, 128)) < 0) {
		printf("\nWriting fails\n");
        return FAIL;
    }

	// 0 - terminal_close should return 0 for success
    if (terminal_close(0) != 0) {
        return FAIL;
		printf("\nClosing fails\n");
    }

    return PASS;
}

/*
* read_dentry_by_name_test
* Tests the read_dentry_by_name function by printing out the file name, type, and inode number.
* Returns PASS if the function works.
* Inputs: filename - the file name
* Outputs: PASS/FAIL
* Side Effects: None
*/
int read_dentry_by_name_test(const uint8_t* filename) 
{
    TEST_HEADER;
    dentry_t test;

    if (read_dentry_by_name(filename, &test) == -1) 
	{
        return FAIL; // File not found
    }

    printf("READ FILE TEST\n");
    printf("The file's name is %s\n", test.file_name);
	printf("The file's type is %d\n", test.file_type);
	printf("The file's inode number is %d\n", test.inode_num);
    if (strncmp((int8_t*)test.file_name, (int8_t*)filename, 32) != 0) 
	{
        return FAIL; // File names don't match
    }
    return PASS;
}

/*
* print_all_files_test
* Tests the file system by printing out a list of all files in the file system.
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS
* Side Effects: None
*/
int read_dirt_test()
{
	TEST_HEADER;
	int i;
	// the file system can hold up to 63 files
	dentry_t temp[63];
	// 4096 is the size of one block
	uint8_t buf[4096];
	for (i = 0; i < 63; i++) 
	{
		if (read_dentry_by_index((uint32_t)i,&temp[i]) == -1)
		{
			break;
		}
		strncpy((int8_t*)buf, (int8_t*)temp[i].file_name, 32);
		buf[32] = '\0';
		printf("file_name: %s  ,file_type: %d  ,file_size: %d\n", (int8_t*)buf, temp[i].file_type, get_length(temp[i].inode_num));
	}
	return PASS;
}

/*
* read_data_test
* Tests the read_data function by printing out the contents of a file.
* Returns PASS if the function works.
* Inputs: filename - the file name
* Outputs: PASS/FAIL
* Side Effects: None
*/
int read_data_test(uint8_t* filename) 
{
    TEST_HEADER;
	int i;
	char buff[40000] = {'\0'};
    dentry_t test_dentry;
    if (read_dentry_by_name(filename, &test_dentry) == -1) 
	{
        return FAIL;
    }
    uint32_t inode_index = test_dentry.inode_num;
    int32_t file_length = get_length(inode_index);
    int32_t num_bytes_copied = read_data(test_dentry.inode_num, 0, (uint8_t*)buff, file_length);
    if (num_bytes_copied == -1) 
	{
        return FAIL;
    } 
	for (i = 0; i < num_bytes_copied; i++) 
	{
		if (buff[i] != '\0')
		{
			putc(buff[i], 1);
		}
	}
	return PASS;
}
/*
* file_open_test
* Tests the file_open function by opening a file.
* Returns PASS if the function works.
* Inputs: filename - the file name
* Outputs: PASS/FAIL
* Side Effects: None
*/
int file_open_test(uint8_t* filename)
{
	TEST_HEADER;
	if (file_open(filename) == -1) { return FAIL; }
	return PASS;
}

/*
* file_close_test
* Tests the file_close function by closing a file.
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int file_close_test()
{
	TEST_HEADER;
	int fd = 0;
	if (file_close(fd) == 0) { return PASS; }
	return FAIL;
}

/*
* dir_open_test
* Tests the dir_open function by opening a directory.
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int dir_open_test()
{
	TEST_HEADER;
	if (!dir_open((uint8_t*)".")) 
	{
		return PASS;
	}
	return FAIL;
}

/*
* dir_close_test
* Tests the dir_close function by closing a directory.
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int dir_close_test()
{
	TEST_HEADER;
	if (!dir_close(0)) {
		return PASS;
	}
	return FAIL;
}


/* Checkpoint 3 tests */

/*
* bad_execute
* Tests the execute function passed bad arguments
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int bad_execute()
{
	TEST_HEADER; 

	// -1 - fail with bad input
	if (execute((uint8_t*)"") == -1) {
		return PASS;
	}

	// Should never be called
	return FAIL;
}

/*
* bad_halt
* Tests the halt function passed bad arguments
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int bad_halt()
{
	TEST_HEADER;

	// 1 - exception
	halt(1);

	// Should never be called
	return FAIL;
}

/*
* bad_read
* Tests the read function passed bad arguments
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int bad_read()
{
	TEST_HEADER;
	int32_t fd = 0;		// 0 - stdin
	uint8_t arg2[128] = "Readtest\n";
	int32_t nbytes = 9;
	if (read(-1, arg2, nbytes) == -1) {
		if (read(fd, NULL, nbytes) == -1) {
			if (read(fd, arg2, -1) == -1) {
				return PASS;
			} 
		}
	}
	return FAIL;
}

/*
* bad_write
* Tests the write function passed bad arguments
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int bad_write()
{
	TEST_HEADER;
	int32_t fd = 0;
	uint8_t arg2[128] = "Writetest\n";
	int32_t nbytes = 9;
	if (write(-1, arg2, nbytes) == -1) {
		if (write(fd, NULL, nbytes) == -1) {
			if (write(fd, arg2, -1) == -1) {
				return PASS;
			} 
		}
	}
	return FAIL;
}

/*
* bad_open
* Tests the open function passed bad arguments
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int bad_open()
{
	TEST_HEADER;
	if (open((uint8_t*)"") == -1) {
		if (open((uint8_t*)"hi") == -1) {
			return PASS;
		}
	}
	return FAIL;
}

/*
* bad_close
* Tests the close function passed bad arguments
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int bad_close()
{
	TEST_HEADER;
	if (((close(0) == -1) && (close(1) == -1) && (close(3) == -1) && (close(9) == -1)) == 1) {
		return PASS;
	}
	return FAIL;
}

/*
* run_shell
* Tests execution of function shell 
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int run_shell() 
{
	TEST_HEADER;
	execute((uint8_t*)"shell");
	return FAIL;
}

/*
* run_testprint
* Tests execution of function testprint
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int run_testprint() 
{
	TEST_HEADER;
	execute((uint8_t*)"testprint");
	return FAIL;
}

/*
* halt_shell
* Tests halting function shell 
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int halt_shell() 
{
	TEST_HEADER;
	halt(0);
	return FAIL;
}

/*
* read_test
* Tests execution of function read
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int read_test()
{
	TEST_HEADER;
	int32_t byte;
	uint8_t arg[128];
	byte = read(0, arg, 3);
	return PASS;
}

/*
* write_test
* Tests execution of function write
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int write_test()
{
	TEST_HEADER;
	int32_t byte;
	uint8_t arg[128] = "Writetest\n";
	byte = write(1, arg, 3);
	return PASS;
}

/*
* open_test
* Tests execution of function open
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int open_test()
{
	TEST_HEADER;
	uint8_t arg[128] = ".";
	open(arg);
	return PASS;
}

/*
* close_test
* Tests execution of function close
* Returns PASS if the function works.
* Inputs: None
* Outputs: PASS/FAIL
* Side Effects: None
*/
int close_test()
{
	TEST_HEADER;
	close(0);
	return PASS;
}

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	/* checkpoint 1 tests */
	// uncomment to test one each time
	// TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
	// TEST_OUTPUT("page_dereferencing_test", page_dereferencing_test());
	// TEST_OUTPUT("pic_test", pic_test());
	// TEST_OUTPUT("divide_zero_test", divide_zero_test());
	// TEST_OUTPUT("page_derefer_inaccessible_test", page_derefer_inaccessible_test());
	// TEST_OUTPUT("deref_null_test", deref_null_test());
	// TEST_OUTPUT("idt_test2", idt_test2());

	/* checkpoint 2 tests */
	// TEST_OUTPUT("RTC_open_and_close_test\n", rtc_open_and_close());
	// TEST_OUTPUT("RTC_driver_test\n", rtc_driver_test());
	// TEST_OUTPUT("terminal_driver_test\n", terminal_driver_test());
	// TEST_OUTPUT("terminal_function_test\n", terminal_function());
	// TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test((uint8_t*)"frame0.txt"));
	// TEST_OUTPUT("read_dirt_test", read_dirt_test());
	// TEST_OUTPUT("file_open_test", file_open_test((uint8_t*)"frame0.txt"));
	// TEST_OUTPUT("file_close_test", file_close_test());
	// TEST_OUTPUT("dir_open_test", dir_open_test());
	// TEST_OUTPUT("dir_close_test", dir_close_test());
	// TEST_OUTPUT("read_data_test", read_data_test((uint8_t*)"frame0.txt")); // for small file
	// TEST_OUTPUT("read_data_test", read_data_test((uint8_t*)"hello")); // for executables
	// TEST_OUTPUT("read_data_test", read_data_test((uint8_t*)"verylargetextwithverylongname.tx")); // for large file

	/* checkpoint 3 tests */
	// TEST_OUTPUT("bad_execute_test", bad_execute());
	// TEST_OUTPUT("bad_halt_test", bad_halt());
	// TEST_OUTPUT("bad_read_test", bad_read());
	// TEST_OUTPUT("bad_write_test", bad_write());
	// TEST_OUTPUT("bad_open_test", bad_open());
	// TEST_OUTPUT("bad_close_test", bad_close());
	// TEST_OUTPUT("running_shell_test", run_shell());
	// TEST_OUTPUT("running_testprint_test", run_testprint());
	// TEST_OUTPUT("halting_shell_test", halt_shell());
	// TEST_OUTPUT("read_test", read_test());
	// TEST_OUTPUT("write_test", write_test());
	// TEST_OUTPUT("open_test", open_test());
	// TEST_OUTPUT("close_test", close_test());
}

