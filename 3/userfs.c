#include <stdio.h>
#include "userfs.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum {
    BLOCK_SIZE = 512,
    MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
    /** Block memory. */
    char *memory;
    /** How many bytes are occupied. */
    size_t occupied;
    /** Next block in the file. */
    struct block *next;
    /** Previous block in the file. */
    struct block *prev;
};

struct file {
    /** Double-linked list of file blocks. */
    struct block *block_list;
    /**
     * Last block in the list above for fast access to the end
     * of file.
     */
    struct block *last_block;
    /** How many file descriptors are opened on the file. */
    int refs;
    /** File name. */
    char *name;
    /** Files are stored in a double-linked list. */
    struct file *next;
    struct file *prev;

    int deleted;
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
    struct file *file;
    // Additional members can be added here if needed
    size_t block;
    size_t pos;
    size_t rules_w;
    size_t rules_r;
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;

enum ufs_error_code
ufs_errno()
{
    return ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{

    // Check if the filename is valid
    if (filename == NULL || filename[0] == '\0') {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    // // fprintf(stderr, "AAA1");
	struct file *file_ptr = file_list;
    while (file_ptr != NULL) {

        // // fprintf(stderr, "AAA2 %s deleted ?? %d\n", file_ptr->name, file_ptr->deleted);
        if (strcmp(file_ptr->name, filename) == 0 && file_ptr->deleted == 0) {
            
            // // fprintf(stderr, "AAA3 %d -- %d", file_descriptor_count, file_descriptor_capacity);
			if ( file_descriptor_count == file_descriptor_capacity){
                
                // // fprintf(stderr, "AAA3111111111111");
				// // fprintf(stderr, "CAL2");
				if ( file_descriptors == NULL ){
					// // fprintf(stderr, "CAL3");
					file_descriptor_capacity++;
					file_descriptors = (struct filedesc **)malloc(sizeof(struct filedesc*));
					// // fprintf(stderr, "CAL5");
					
				} else {
					// // fprintf(stderr, "CAL4");
					file_descriptor_capacity*=2;
					file_descriptors = (struct filedesc **)realloc(file_descriptors, file_descriptor_capacity * 2 * sizeof(struct filedesc*));
					// // fprintf(stderr, "CAL6");
				};
				// // fprintf(stderr, "CAL82");
				struct filedesc *filedesc_ = (struct filedesc *)malloc(sizeof(struct filedesc));
				// // fprintf(stderr, "CAL83");

                file_ptr->refs++;

				filedesc_->file = file_ptr;
                filedesc_->block = 0;
                filedesc_->pos = 0;
                filedesc_->rules_w = 1;
                filedesc_->rules_r = 1;

                if (flags & UFS_READ_ONLY){
                    filedesc_->rules_w = 0;
                } else if (flags & UFS_WRITE_ONLY){
                    filedesc_->rules_r = 0;
                }

                file_descriptors[file_descriptor_count] = filedesc_;
				file_descriptor_count++;
				// // fprintf(stderr, "CAL8");
				return file_descriptor_count;
			} else {

                // // fprintf(stderr, "AAA32222222222");
				// // fprintf(stderr, "CAL10");
				for (int i = 0; i < file_descriptor_capacity; ++i) {
					if (file_descriptors[i] == NULL) {
						// // fprintf(stderr, "CAL7");
						struct filedesc *filedesc_ = (struct filedesc *)malloc(sizeof(struct filedesc));
						filedesc_->file = file_ptr;
						file_descriptors[i] = filedesc_;
                        filedesc_->block = 0;
                        filedesc_->pos = 0;
						file_descriptor_count++;

                        filedesc_->rules_w = 1;
                        filedesc_->rules_r = 1;

                        if (flags & UFS_READ_ONLY){
                            filedesc_->rules_w = 0;
                        } else if (flags & UFS_WRITE_ONLY){
                            filedesc_->rules_r = 0;
                        }
						// // fprintf(stderr, "CAL12");

						return i+1;
					};
				}
			}
        }
        file_ptr = file_ptr->next;
    }

    
	if ( flags && UFS_CREATE ){

                // // fprintf(stderr, "AAA324444444444444444444");

		struct file *new_file = (struct file *)malloc(sizeof(struct file));
		if (new_file == NULL) {
			// Memory allocation failed
			ufs_error_code = UFS_ERR_NO_MEM;
			return -1;
		}
		// Initialize the new fil

		new_file->name = strdup(filename); // Duplicate the filename
		if (new_file->name == NULL) {
			// Memory allocation failed
			free(new_file);
			ufs_error_code = UFS_ERR_NO_MEM;
			return -1;
		}

		new_file->block_list = NULL;
		new_file->last_block = NULL;
		new_file->refs = 1;
        new_file->deleted = 0;

        if ( file_list == NULL){
            // file_list = new_file;
            new_file->next = NULL;
		    new_file->prev = NULL;
        } else {
            new_file->next = file_list;
		    new_file->prev = NULL;
            file_list->prev = new_file;
        }

		file_list = new_file;

		// new_file->next = file_list;
		// new_file->prev = NULL;
		// if (file_list != NULL) {
		// 	file_list->prev = new_file;
		// }
		// // // fprintf(stderr, "CAL");

		if ( file_descriptor_count == file_descriptor_capacity){

			// // fprintf(stderr, "CAL2");
			if ( file_descriptors == NULL ){
				// // fprintf(stderr, "CAL3");
				file_descriptor_capacity++;
				file_descriptors = (struct filedesc **)malloc(sizeof(struct filedesc*));
				// // fprintf(stderr, "CAL5");
				
			} else {
				// // fprintf(stderr, "CAL4");
				file_descriptor_capacity*=2;
				file_descriptors = (struct filedesc **)realloc(file_descriptors, file_descriptor_capacity * 2 * sizeof(struct filedesc*));
				// // fprintf(stderr, "CAL6");
			};
			// // fprintf(stderr, "CAL82");
			struct filedesc *filedesc_ = (struct filedesc *)malloc(sizeof(struct filedesc));
			// // fprintf(stderr, "CAL83");
			filedesc_->file = new_file;
			// // fprintf(stderr, "CAL84");
			file_descriptors[file_descriptor_count] = filedesc_;
			// // fprintf(stderr, "CAL3");
			file_descriptor_count++;
            filedesc_->block = 0;
            filedesc_->rules_w = 1;
                filedesc_->rules_r = 1;

                if (flags & UFS_READ_ONLY){
                    filedesc_->rules_w = 0;
                } else if (flags & UFS_WRITE_ONLY){
                    filedesc_->rules_r = 0;
                }
            filedesc_->pos = 0;
			// // fprintf(stderr, "CAL8");
			return file_descriptor_count;
		} else {
			// // fprintf(stderr, "CAL10");
			for (int i = 0; i < file_descriptor_capacity; ++i) {
				if (file_descriptors[i] == NULL) {
					// // fprintf(stderr, "CAL7");
					struct filedesc *filedesc_ = (struct filedesc *)malloc(sizeof(struct filedesc));
					filedesc_->file = new_file;
					file_descriptors[i] = filedesc_;
					file_descriptor_count++;
					// // fprintf(stderr, "CAL12");
                    filedesc_->block = 0;
                    filedesc_->pos = 0;
                    filedesc_->rules_w = 1;
                    filedesc_->rules_r = 1;

                    if (flags & UFS_READ_ONLY){
                        filedesc_->rules_w = 0;
                    } else if (flags & UFS_WRITE_ONLY){
                        filedesc_->rules_r = 0;
                    }

                    return i+1;
                };
    		}
		}

	}


	// // fprintf(stderr, "CAL9");
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
}


size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

ssize_t
ufs_write(int fd, const char *buf, size_t size)
{
    // Check if the file descriptor is valid
    if (fd <= 0 || fd > file_descriptor_capacity+1 || file_descriptors[fd-1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    // Get file descriptor
    struct filedesc *fdesc = file_descriptors[fd-1];
    struct file *file = fdesc->file;
    
    if ( fdesc->rules_w == 0 ){
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }

    // Check if the file has been opened in write mode
    // (UFS_READ_ONLY flag not set)
    // If not, return an error
    // if (!(file->flags & UFS_READ_ONLY)) {
    //     ufs_error_code = UFS_ERR_NO_PERMISSION;
    //     return -1;
    // }
    
    if ( fdesc->block >= (MAX_FILE_SIZE / BLOCK_SIZE) ){
        
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }

    size_t left = size;
    size_t count = 0;
    size_t state = fdesc->pos + size;
    size_t caLLL = 0;
    // Allocate memory for the data buffer

    struct block *current_block = file->block_list;

    // fprintf(stderr, "\n CUR_WR_STEP_! %ld \n", fdesc->pos);
    // // fprintf(stderr, "\n MEMORY_WR_STEP %s \n", current_block->memory);
    // // fprintf(stderr, "\n OCCUPIED_WR %ld\n", current_block->occupied);
    // // fprintf(stderr, "\n TI_BE_WR %s\n", buf);
    // fprintf(stderr, "\n LEFT_WR_! %ld\n", left);
    // fprintf(stderr, "\n block_! %ld \n", fdesc->block);
    // // fprintf(stderr, "\n CALL1 \n");
    int null_flag = 0;

    for (size_t i = 0; i < fdesc->block; i++){
            // // fprintf(stderr, "\n CALL1IT %ld\n", i);
            current_block = current_block->next;
    }

    if ( current_block == NULL ){
        // // fprintf(stderr, "\n CALL1NULL \n");
        null_flag = 1;
    } 

    // fprintf(stderr, "\n CUR_BLOCK %p\n", current_block);
    // // fprintf(stderr, "\n CALL2 \n");

    for (size_t i = 0; i < ( state + BLOCK_SIZE - 1 ) / BLOCK_SIZE; i++)
    {
        struct block *new_block;
        if ( fdesc->pos == 0 && (null_flag == 1 || (current_block->next == NULL && current_block->occupied == BLOCK_SIZE )) ){ // првоерятть что нет блока
            // Allocate memory for the new block
            // fprintf(stderr, "\n WR NO BLOCK %ld \n", fdesc->block);

            new_block = (struct block *)malloc(sizeof(struct block));
            if (new_block == NULL) {
                ufs_error_code = UFS_ERR_NO_MEM;
                return -1;
            }

            new_block->next = NULL;
            new_block->occupied = 0;
            new_block->memory = (char *)malloc(BLOCK_SIZE);
            // fprintf(stderr, "\n CALL1 \n");

            new_block->prev = file->last_block;
    // fprintf(stderr, "\n CALL2 \n");
            
            if ( null_flag == 0 ){
    // fprintf(stderr, "\n NULL_GLAG!!! == 0 \n");

                current_block->next = new_block;
                
                current_block = current_block->next;
            } else {
    // fprintf(stderr, "\n CALL2 \n");

                current_block = new_block;
                current_block->next = NULL;
    // fprintf(stderr, "\n CALL3 \n");

    // fprintf(stderr, "\n CALL4 \n");

                if ( !file->block_list ){
                    file->block_list = new_block;                 
                }

                if ( file->last_block != NULL ){
                file->last_block->next = new_block;

                }
    // fprintf(stderr, "\n CALL5 \n");

            }
            
            file->last_block = new_block;
            null_flag = 0;
            
    // // fprintf(stderr, "\n CALL3 \n");

            
            // fdesc->block = 0; // &&
        } else {
            // fprintf(stderr, "\n WR YA BLOCK %ld \n", fdesc->block);
            new_block = current_block;
        };

        // 
        // // fprintf(stderr, "\n CUR_WR_STEP %ld \n", fdesc->pos);
        // // fprintf(stderr, "\n MEMORY_WR_STEP %s \n", new_block->memory);
        // // fprintf(stderr, "\n OCCUPIED_WR %ld\n", new_block->occupied);
        // // fprintf(stderr, "\n TI_BE_WR %s\n", buf);
        // // fprintf(stderr, "\n LEFT_WR %ld\n", left);

        size_t HMTW = min(BLOCK_SIZE-fdesc->pos, left);
        // // fprintf(stderr, "\n HMTW %ld\n", HMTW);

        // new_block->memory = (char *)realloc(new_block->memory, strlen(new_block->memory) + HMTW);
        if (new_block->memory == NULL) {
            free(new_block);
            ufs_error_code = UFS_ERR_NO_MEM;
            return -1;
        }

        // // fprintf(stderr, "BBB1");
        // сместить буфер!!

        // // fprintf(stderr, "\n buff %s \n", buf);           caLLL = 
        // // fprintf(stderr, "\n hmtw %ld \n", HMTW);
        memcpy(new_block->memory+fdesc->pos, buf+caLLL, HMTW);
        // // fprintf(stderr, "\n mem %s \n", new_block->memory);
    
        count += HMTW;
        fdesc->pos += HMTW;

        if ( fdesc->pos > new_block->occupied ){
            new_block->occupied = fdesc->pos;
        }

        
        
        // // fprintf(stderr, "BBB2");
        // Update file's block list
        if (file->block_list == NULL) {
             // fprintf(stderr, "\n BREDIK =======================\n");
            file->block_list = new_block;
        } 
        
        if ( fdesc->pos == BLOCK_SIZE ){
            // // fprintf(stderr, "\n BREDIK =======================\n");
            //  // fprintf(stderr, "\n MEMORY %s\n", current_block->memory);
            caLLL = count;
            fdesc->pos = 0;
            fdesc->block++;
            // fdesc->file->block_list+fdesc->block = NULL;
        } 

        // // fprintf(stderr, "BBB3");
        left -= HMTW;
        
        // // fprintf(stderr, "\n CUR_WR_STEP_2 %ld \n", fdesc->pos);
        // // fprintf(stderr, "\n MEMORY_WR_STEP_2 %s \n", new_block->memory);
        // // fprintf(stderr, "\n OCCUPIED_WR_2 %ld\n", new_block->occupied);
        // // fprintf(stderr, "\n TI_BE_WR_2 %s\n", buf);
        // // fprintf(stderr, "\n LEFT_WR_2 %ld\n", left);
        // // fprintf(stderr, "\n COUNT_2 %ld\n", count);
        // // fprintf(stderr, "BBB4");
    }

    // Return the number of bytes written

    // fprintf(stderr, "\n CUR_WR_STEP %ld \n", fdesc->pos);
    // // fprintf(stderr, "\n MEMORY_WR_STEP %s \n", current_block->memory);
    // // fprintf(stderr, "\n OCCUPIED_WR %ld\n", current_block->occupied);
    // // fprintf(stderr, "\n TI_BE_WR %s\n", buf);
    // fprintf(stderr, "\n LEFT_WR %ld\n", left);
    // fprintf(stderr, "\n block %ld \n", fdesc->block);
    return count;
}


ssize_t
ufs_read(int fd, char *buf, size_t size)
{
    // Check if the file descriptor is valid
    if (fd <= 0 || fd > file_descriptor_capacity+1 || file_descriptors[fd-1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    // // fprintf(stderr, "CCC1 %d\n", fd);
    // Get file descriptor
    struct filedesc *fdesc = file_descriptors[fd-1];
    // // fprintf(stderr, "CCC1-1 %s\n", fdesc->file->name);
    struct file *file = fdesc->file;
    
    if ( fdesc->rules_r == 0 ){
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }
    // // fprintf(stderr, "CCC1-2");
    // Initialize bytes read counter
    size_t bytes_read = 0;
    // // fprintf(stderr, "CCC1-3");
    // Iterate through the blocks of the file
    struct block *current_block = file->block_list;
    for (size_t i = 0; i < fdesc->block; i++){
        // // fprintf(stderr, "\n CALL1IT \n");
        current_block = current_block->next;
    }
    // // fprintf(stderr, "CCC2");

    // // fprintf(stderr, "\n POS_WRITE %ld\n", fdesc->pos);
    // // fprintf(stderr, "\n OCCUPIED_WRITE %ld\n", current_block->occupied);
    // // // fprintf(stderr, "\n fgd %d \n", fd);
    // // fprintf(stderr, "\n mem3 %s \n", file->last_block->memory);
    // // fprintf(stderr, "\n block? %d \n", fdesc->block == 0);
    // // fprintf(stderr, "\n  1 = 1 %d \n", 1 == 1);
    
    size_t left = size;
    int flag = 0;

    while (current_block != NULL && bytes_read < size && flag == 0) {
        // Calculate the number of bytes to read from this block
        //  // fprintf(stderr, "\n size %ld bytes_read %ld current_block->occupied %ld pos %ld\n", size, bytes_read, current_block->occupied, fdesc->pos);
        // size_t bytes_to_read = (size - bytes_read <= ((size_t)current_block->occupied - fdesc->pos)) 
        // ? size - bytes_read 
        // : ((size_t)current_block->occupied - fdesc->pos);

        size_t HMTR = min(current_block->occupied-fdesc->pos, left);

        // fprintf(stderr, "\n HMTR %ld \n", HMTR);
        // Copy data from the block to the buffer
        memcpy(buf + bytes_read, current_block->memory+fdesc->pos, HMTR);
        // // fprintf(stderr, "\n buf2 %s \n", buf);

        // // fprintf(stderr, "CCC3");
        // Update counters
        bytes_read += HMTR;
        fdesc->pos += HMTR;
        left -= HMTR;

        if ( current_block->next == NULL && current_block->occupied == fdesc->pos ){
            flag = 1;
        }

        // if ( fdesc->pos == current_block->occupied ){
        //     flag = 1;
        // }

        if ( BLOCK_SIZE == fdesc->pos){
            fdesc->block++;
            current_block = current_block->next;
            fdesc->pos = 0;
        }

        // // fprintf(stderr, "CCC4");
    }

    // // fprintf(stderr, "CCC5 %ld\n", bytes_read);
    // Return the number of bytes read
    // // fprintf(stderr, "\n block %ld \n", fdesc->block);
    // // fprintf(stderr, "\n MEMORY_RE %s \n", current_block->memory);
    // // fprintf(stderr, "\n OCCUPIED_RE %ld\n", current_block->occupied);
    return bytes_read;
}

int
ufs_close(int fd)
{
    // Check if the file descriptor is valid
    if (fd <= 0 || fd > file_descriptor_capacity+1 || file_descriptors[fd-1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    // Free the file descriptor
    free(file_descriptors[fd-1]);
    file_descriptors[fd-1] = NULL;
    file_descriptor_count--;

    return 0;
}

int
ufs_delete(const char *filename)
{
    // Check if the filename is valid
    if (filename == NULL || filename[0] == '\0') {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }

    // Find the file by name and remove it from the list
    struct file *prev = NULL;
    struct file *curr = file_list;
    while (curr != NULL) {
        if (strcmp(curr->name, filename) == 0) {


            if ( curr->refs > 0 ){
                
                curr->deleted = 1;

                return 0;
            } 

                // Found the file to delete
            if (prev != NULL) {
                prev->next = curr->next;
            } else {
                file_list = curr->next;
            }

            // СДЕЛАТЬ ФРИ
            free(curr->name);

            // Free blocks
            struct block *block_ptr = curr->block_list;
            while (block_ptr != NULL) {
                struct block *temp = block_ptr;
                block_ptr = block_ptr->next;
                free(temp->memory);
                free(temp);
            }
            free(curr);
            return 0;
            
            
        }
        prev = curr;
        curr = curr->next;
    }
    // File not found
    ufs_error_code = UFS_ERR_NO_FILE;
    return -1;
}

void
ufs_destroy(void) {
    // Free memory used by file descriptors
    	// fprintf(stderr, "\n BREDIK 1 =======================\n");

    if (file_descriptors != NULL) {
        for (int i = 0; i < file_descriptor_capacity; ++i) {
            if (file_descriptors[i] != NULL) {
                free(file_descriptors[i]);
            }
        }
        free(file_descriptors);
    }
    	// fprintf(stderr, "\n BREDIK 2 =======================\n");

    // Free memory used by files and their blocks
    struct file *curr_file = file_list;
    while (curr_file != NULL) {
    	// fprintf(stderr, "\n BREDIK 2-2 =======================\n");

        struct file *temp_file = curr_file;
        curr_file = curr_file->next;

        // Free file name
        free(temp_file->name);

        // Free file blocks
        struct block *curr_block = temp_file->last_block;
        while (curr_block != NULL) {

            struct block *temp_block = curr_block;
    	    // fprintf(stderr, "\n BREDIK 2-3 %s=======================\n", temp_block->memory);

            curr_block = curr_block->prev;

            // Free block memory
            free(temp_block->memory);

            // Free block structure
            free(temp_block);
        }

        // Free file structure
        free(temp_file);
    }
    	// fprintf(stderr, "\n BREDIK 3 =======================\n");

    // Reset global variables
    file_list = NULL;
    file_descriptors = NULL;
    file_descriptor_count = 0;
    file_descriptor_capacity = 0;
    ufs_error_code = UFS_ERR_NO_ERR;
}