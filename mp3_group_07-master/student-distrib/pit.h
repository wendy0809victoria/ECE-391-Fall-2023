//
//  pit.h
//  
//
//  Created by Wendi Wang on 2023/11/24.
//

#ifndef pit_h
#define pit_h

#include "types.h"
#include "filesystem.h"
#include "terminal.h"

#define PIT_CMD   0x43          // Command port for pit
#define PIT_DATA  0x40          // Data port for pit

void pit_init(void);
void pit_handler(void);

#endif /* pit_h */
