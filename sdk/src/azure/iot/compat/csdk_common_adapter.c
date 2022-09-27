// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**
 * @brief Common C-SDK Compat API implementations.
 * 
 */
#include <malloc.h>
#include <string.h>

#define	ENOMEM		12	/* Out of memory */
#define	EINVAL		22	/* Invalid argument */

int mallocAndStrcpy_s(char** destination, const char* source) {
    int result;
    int copied_result;
    /*Codes_SRS_CRT_ABSTRACTIONS_99_036: [destination parameter or source parameter is NULL, the error code returned shall be EINVAL and destination shall not be modified.]*/
    if ((destination == NULL) || (source == NULL))
    {
        /*If strDestination or strSource is a NULL pointer[...]these functions return EINVAL */
        result = EINVAL;
    }
    else
    {
        size_t l = strlen(source);
        char* temp = (char*)malloc(l + 1);

        /*Codes_SRS_CRT_ABSTRACTIONS_99_037: [Upon failure to allocate memory for the destination, the function will return ENOMEM.]*/
        if (temp == NULL)
        {
            result = ENOMEM;
        }
        else
        {
            *destination = temp;
            /*Codes_SRS_CRT_ABSTRACTIONS_99_039: [mallocAndstrcpy_s shall copy the contents in the address source, including the terminating null character into location specified by the destination pointer after the memory allocation.]*/
            strcpy(*destination, source);
            result = 0;
        }
    }
    return result;
}
