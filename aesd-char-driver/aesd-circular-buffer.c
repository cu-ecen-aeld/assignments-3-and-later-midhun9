/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{

	uint8_t out_offs = buffer->out_offs;
	size_t size = 0;
	size_t check_size = 0;
	uint8_t temp_count = 0;
	uint8_t i = 0;

	if ((out_offs == buffer->in_offs) && !(buffer->full)) 
	{
        	return NULL;
    	}
	// check each entry
	for(i = out_offs; temp_count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i = (i+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
	{
		temp_count++;
		size += buffer->entry[i].size;

                if(size > char_offset)
                {
                        check_size = size - buffer->entry[i].size;
                        *entry_offset_byte_rtn = char_offset - check_size;
                        return &(buffer->entry[i]);
                }

	}


    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
	const char* extra_byte = NULL;
	
	if(buffer->full)
	{
		extra_byte = buffer->entry[buffer->out_offs].buffptr; // add additional value to extra_byte if buffer is full
	}

	//add entry to buffer
	buffer->entry[buffer->in_offs] = *add_entry;

	//increment in_offs
	buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

	if(buffer->full == true)
	{
		buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; 
	}

	if(buffer->in_offs == buffer->out_offs)
	{
		buffer->full = true;
	}

	return extra_byte;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
   
    buffer->in_offs = 0;     
    buffer->out_offs = 0;            
    buffer->full = false;            
}


void aesd_circular_buffer_clean(struct aesd_circular_buffer *buffer)
{
	uint8_t out_offs = buffer->out_offs;
	uint8_t temp_count = 0;
        uint8_t i = 0;

        // check each entry
        for(i = out_offs; temp_count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i = (i+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
                temp_count++;
                
		if(buffer->entry[i].buffptr != NULL)
		{
			#ifdef __KERNEL__
			kfree(buffer->entry[i].buffptr);
			#else
			free(buffer->entry[i].buffptr);
			#endif
		}
        }
}

