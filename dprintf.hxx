#pragma once

#include <fstream>


#ifdef DEBUG

	#define dprintf(format_string, ...)                                     \
        {                                                                   \
            printf("\x1b[35m>> debug from <%s::%d>: ", __FILE__, __LINE__); \
            printf(format_string, __VA_ARGS__);                             \
            printf("\x1b[0m");                                              \
        }

    #define debug if(true)
						
#else

	#define dprintf(format_string, ...)
    #define debug if(false)
	
#endif
