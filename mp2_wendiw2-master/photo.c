/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:		Steve Lumetta
 * Version:		3
 * Creation Date:	Fri Sep  9 21:44:10 2011
 * Filename:		photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>

#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"


/* types local to this file (declared in types.h) */

/*
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
	photo_header_t hdr;			/* defines height and width */
	uint8_t        palette[192][3];     /* optimized palette colors */
	uint8_t*       img;                 /* pixel data               */
};

/*
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have
 * been stored as 2:2:2-bit RGB values (one byte each), including
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos,
 * pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of the
 * second row, and so forth.  No padding is used.
 */
struct image_t {
	photo_header_t hdr;			/* defines height and width */
	uint8_t*       img;                 /* pixel data               */
};

// The structure storing colors and nodes
typedef struct color_t {
    int index;      // Index of the octree
    int count;      // Count the number of pixels belonging to this node
    int sum_r;      // Count the sum of pixels' red bits
    int sum_g;      // Count the sum of pixels' green bits
    int sum_b;      // Count the sum of pixels' blue bits
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} color_t;

/*
 * compare
 *   DESCRIPTION: Compare the frequency of pixels appeared in one node.
 *   INPUTS: a, b - two pointers of structure to be compared
 *   OUTPUTS: none
 *   RETURN VALUE: The order of two structures (0, -1, or 1)
 */
int compare(const void* a, const void* b) 
{	
	return ((color_t*) b)->count - ((color_t*) a)->count;
}														

/* file-scope variables */

/*
 * The room currently shown on the screen.  This value is not known to
 * the mode X code, but is needed when filling buffers in callbacks from
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set
 * by calling prep_room.
 */
static const room_t* cur_room = NULL;


/*
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost
 *                pixel of a line to be drawn on the screen, this routine
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
	int            idx;   /* loop index over pixels in the line          */
	object_t*      obj;   /* loop index over objects in the current room */
	int            imgx;  /* loop index over pixels in object image      */
	int            yoff;  /* y offset into object image                  */
	uint8_t        pixel; /* pixel from object image                     */
	const photo_t* view;  /* room photo                                  */
	int32_t        obj_x; /* object x position                           */
	int32_t        obj_y; /* object y position                           */
	const image_t* img;   /* object image                                */

	/* Get pointer to current photo of current room. */
	view = room_photo (cur_room);

	/* Loop over pixels in line. */
	for (idx = 0; idx < SCROLL_X_DIM; idx++) {
		buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
			view->img[view->hdr.width * y + x + idx] : 0);
	}

	/* Loop over objects in the current room. */
	for (obj = room_contents_iterate (cur_room); NULL != obj;
		 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

		/* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
		x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
		continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/*
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
		idx = obj_x - x;
		imgx = 0;
	} else {
		idx = 0;
		imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
		pixel = img->img[yoff + imgx];

		/* Don't copy transparent pixels. */
		if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
		}
	}
	}
}


/*
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of
 *                a vertical line to be drawn on the screen, this routine
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
	int            idx;   /* loop index over pixels in the line          */
	object_t*      obj;   /* loop index over objects in the current room */
	int            imgy;  /* loop index over pixels in object image      */
	int            xoff;  /* x offset into object image                  */
	uint8_t        pixel; /* pixel from object image                     */
	const photo_t* view;  /* room photo                                  */
	int32_t        obj_x; /* object x position                           */
	int32_t        obj_y; /* object y position                           */
	const image_t* img;   /* object image                                */

	/* Get pointer to current photo of current room. */
	view = room_photo (cur_room);

	/* Loop over pixels in line. */
	for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
		buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
			view->img[view->hdr.width * (y + idx) + x] : 0);
	}

	/* Loop over objects in the current room. */
	for (obj = room_contents_iterate (cur_room); NULL != obj;
		 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

		/* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
		y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
		continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/*
	 * The y offsets depend on whether the object starts below or
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
		idx = obj_y - y;
		imgy = 0;
	} else {
		idx = 0;
		imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
		pixel = img->img[xoff + img->hdr.width * imgy];

		/* Don't copy transparent pixels. */
		if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
		}
	}
	}
}


/*
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t
image_height (const image_t* im)
{
	return im->hdr.height;
}


/*
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t
image_width (const image_t* im)
{
	return im->hdr.width;
}

/*
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t
photo_height (const photo_t* p)
{
	return p->hdr.height;
}


/*
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t
photo_width (const photo_t* p)
{
	return p->hdr.width;
}


/*
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r)
{
	/* Record the current room. */
    cur_room = r;
    
    /* Call function set_palette_color in modex.c */
    set_palette_color (room_photo(r) -> palette);     // loads 192 colors of the photo to VGA
}


/*
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
	FILE*    in;		/* input file               */
	image_t* img = NULL;	/* image structure          */
	uint16_t x;			/* index over image columns */
	uint16_t y;			/* index over image rows    */
	uint8_t  pixel;		/* one pixel from the file  */

	/*
	 * Open the file, allocate the structure, read the header, do some
	 * sanity checks on it, and allocate space to hold the image pixels.
	 * If anything fails, clean up as necessary and return NULL.
	 */
	if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
		if (NULL != img->img) {
			free (img->img);
		}
		free (img);
	}
	if (NULL != in) {
		(void)fclose (in);
	}
	return NULL;
	}

	/*
	 * Loop over rows from bottom to top.  Note that the file is stored
	 * in this order, whereas in memory we store the data in the reverse
	 * order (top to bottom).
	 */
	for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

		/*
		 * Try to read one 8-bit pixel.  On failure, clean up and
		 * return NULL.
		 */
		if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
			(void)fclose (in);
		return NULL;
		}

		/* Store the pixel in the image data. */
		img->img[img->hdr.width * y + x] = pixel;
	}
	}

	/* All done.  Return success. */
	(void)fclose (in);
	return img;
}


/*
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB.  You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
	FILE*    in;	/* input file               */
	photo_t* p = NULL;	/* photo structure          */
	uint16_t x;		/* index over image columns */
	uint16_t y;		/* index over image rows    */
	uint16_t pixel;	/* one pixel from the file  */

	/*
	 * Open the file, allocate the structure, read the header, do some
	 * sanity checks on it, and allocate space to hold the photo pixels.
	 * If anything fails, clean up as necessary and return NULL.
	 */
	if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
		if (NULL != p->img) {
			free (p->img);
		}
		free (p);
	}
	if (NULL != in) {
		(void)fclose (in);
	}
	return NULL;
	}

    /* 64 - the number of nodes in the 2nd level */
    // color_t level_2[64];        // 64 = 8 * 8
    
    /* 4096 - the number of nodes in the 4th level */
    // color_t level_4[4096];      // 4096 = 8 * 8 * 8 * 8
    
    /* Index of the 4th level nodes */
    uint16_t idx4;
    
    /* Index of the 2nd level nodes */
    uint16_t idx2;
    
    /* RGB values of the 4th level nodes */
    uint16_t r4;
    uint16_t g4;
    uint16_t b4;
    
    /* RGB values of the 2nd level nodes */
    uint16_t r2;
    uint16_t g2;
    uint16_t b2;
    
    /* Declaration of loop indices */
    unsigned int i;
    unsigned int j;

	/* Pixels stores all pixels in the photo for reusing */
	color_t level_4[4096];									
	color_t level_2[64];									
	
	uint16_t* pixels = malloc(sizeof(uint16_t) * p->hdr.height * p->hdr.width);		
	
	/* pre_idx stores the index of pixels before sorting */
	uint16_t prev_idx[p->hdr.width * p->hdr.height];	

	/* in4 judges whether a pixel is used for calculating a 4th level color */		
	uint8_t in4[p->hdr.width * p->hdr.height];	
	
	// sorted list used to store the list after qsort
    uint16_t sorted_list [4096];				

	/* Initialize the octree and other arrays */
	for (i = 0; i < 4096; i++) {
        level_4[i].count = 0;
        level_4[i].index = i;
        level_4[i].sum_r = 0;
        level_4[i].sum_g = 0;
        level_4[i].sum_b = 0;
        level_4[i].red = 0;
        level_4[i].green = 0;
        level_4[i].blue = 0;
    }

    for (j = 0; j < 64; j++) {
        level_2[j].count = 0;
        level_2[j].index = j;
        level_2[j].sum_r = 0;
        level_2[j].sum_g = 0;
        level_2[j].sum_b = 0;
        level_2[i].red = ((j & 0x0000F800) >> 14);
        level_2[j].green = ((j & 0x000007E0) >> 9);
        level_2[j].blue = ((j & 0x0000001F) >> 3);
    }

	for (i = 0; i < p->hdr.width * p->hdr.height; i++) {
		in4[i] = 0;
	}

	/*
	 * Loop over rows from bottom to top.  Note that the file is stored
	 * in this order, whereas in memory we store the data in the reverse
	 * order (top to bottom).
	 */
	for (y = p->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; p->hdr.width > x; x++) {

		/*
		 * Try to read one 16-bit pixel.  On failure, clean up and
		 * return NULL.
		 */
		if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (p->img);
		free (p);
			(void)fclose (in);
		return NULL;

		}
		/*
		 * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
		 * and 6 bits blue).  We change to 2:2:2, which we've set for the
		 * game objects.  You need to use the other 192 palette colors
		 * to specialize the appearance of each photo.
		 *
		 * In this code, you need to calculate the p->palette values,
		 * which encode 6-bit RGB as arrays of three uint8_t's.  When
		 * the game puts up a photo, you should then change the palette
		 * to match the colors needed for that photo.
		 */

        // rrrr rggg gggb bbbb = pixel
        // pixel & 0xF800 = rrrr r000 0000 0000
        // (pixel & 0xF800) >> 11 = rrrrr
        // ((pixel & 0xF800) >> 11) << 1 = 0000 0000 00rr rrr0
		r4 = (((pixel & 0xF800) >> 11) << 1);       // Make 5-bit red bits to 6 bits
        g4 = ((pixel & 0x07E0) >> 5);               // Green bits are 6 bits
        b4 = (((pixel & 0x001F)) << 1);             // Make 5-bit blue bits to 6 bits
		idx4 = (r4 >> 2) * 256 + (g4 >> 2) * 16 + (b4 >> 2);        // Choose 4 bits for index
        level_4[idx4].count++;                // 8 + 64 + 512 = 584 nodes in 1&2&3 level
        level_4[idx4].sum_r += r4;
        level_4[idx4].sum_g += g4;
        level_4[idx4].sum_b += b4;
        pixels[p->hdr.width * y + x] = pixel;
		prev_idx[p->hdr.width * y + x] = idx4;
	}
	}

	// Sort the order according to the frequency of pixels
	// 4096 - the number of level 4 nodes
	qsort(level_4, 4096, sizeof(color_t), compare);	

	// 4096 - the number of level 4 nodes
	for (i = 0; i < 4096; i++) {								
		sorted_list[level_4[i].index] = i;							
		if (level_4[i].count != 0) {
			// 128 - if in first 128 level 4 nodes (chosen)
            if (i < 128) {
                p->palette[i][0] = level_4[i].sum_r / level_4[i].count;	
                p->palette[i][1] = level_4[i].sum_g / level_4[i].count;
                p->palette[i][2] = level_4[i].sum_b / level_4[i].count;
		    }
        }
	}
	for (i = 0; i < p->hdr.width * p->hdr.height; i++) {
		if (sorted_list[prev_idx[i]] < 128) {		
			// 64 - colors already in VGA palette			
			p->img[i] = sorted_list[prev_idx[i]] + 64;
			in4[i] = 1;
		}
	}

	for (y = p->hdr.height; y-- > 0; ) {						
	for (x = 0; p->hdr.width > x; x++) {
		if (in4[p->hdr.width * y + x] != 1) {
            // rrrr rggg gggb bbbb = pixel
            // pixel & 0xF800 = rrrr r000 0000 0000
            // (pixel & 0xF800) >> 11 = rrrrr
            // ((pixel & 0xF800) >> 11) << 1 = 0000 0000 00rr rrr0
            r2 = (((pixels[p->hdr.width * y + x] & 0xF800) >> 11) << 1);		// 11 - blue + green
            g2 = ((pixels[p->hdr.width * y + x] & 0x07E0) >> 5);				// 5 -  blue
            b2 = (((pixels[p->hdr.width * y + x] & 0x001F)) << 1);				// 1 - 5 - 4, using 4 bits for the color
			// 16 - two bytes
			// 4 - using 4 bits for the color
            idx2 = (r2 >> 4) * 16 + (g2 >> 4) * 4 + (b2 >> 4);			
            level_2[idx2].sum_r += r2;
            level_2[idx2].sum_g += g2;
            level_2[idx2].sum_b += b2;
            level_2[idx2].count++;
			// 192 - the number of colors already in VGA palette
            p->img[p->hdr.width * y + x] = idx2 + 192;
        }
	}
	}

	// 64 - the number of level 2 nodes
	for (i = 0; i < 64; i++) {									
		if (level_2[i].count != 0) {
            level_2[i].red = level_2[i].sum_r / level_2[i].count;
            level_2[i].green = level_2[i].sum_g / level_2[i].count;
            level_2[i].blue = level_2[i].sum_b / level_2[i].count;
        }
		// 128 - if in first 128 level 4 nodes (chosen)
        p->palette[i + 128][0] = level_2[i].red;
        p->palette[i + 128][1] = level_2[i].green;
        p->palette[i + 128][2] = level_2[i].blue;
	}

	/* All done.  Return success. */
	(void)fclose (in);
	return p;
}

