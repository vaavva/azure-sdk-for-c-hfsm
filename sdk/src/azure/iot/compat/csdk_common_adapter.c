// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

/**
 * @brief Common C-SDK Compat API implementations.
 *
 */
#include <azure/iot/compat/az_hfsm_event_clone.h>
#include <malloc.h>
#include <string.h>

#define ENOMEM 12 /* Out of memory */
#define EINVAL 22 /* Invalid argument */

int mallocAndStrcpy_s(char** destination, const char* source)
{
  int result;
  int copied_result;
  /*Codes_SRS_CRT_ABSTRACTIONS_99_036: [destination parameter or source parameter is NULL, the error
   * code returned shall be EINVAL and destination shall not be modified.]*/
  if ((destination == NULL) || (source == NULL))
  {
    /*If strDestination or strSource is a NULL pointer[...]these functions return EINVAL */
    result = EINVAL;
  }
  else
  {
    size_t l = strlen(source);
    char* temp = (char*)malloc(l + 1);

    /*Codes_SRS_CRT_ABSTRACTIONS_99_037: [Upon failure to allocate memory for the destination, the
     * function will return ENOMEM.]*/
    if (temp == NULL)
    {
      result = ENOMEM;
    }
    else
    {
      *destination = temp;
      /*Codes_SRS_CRT_ABSTRACTIONS_99_039: [mallocAndstrcpy_s shall copy the contents in the address
       * source, including the terminating null character into location specified by the destination
       * pointer after the memory allocation.]*/
      strcpy(*destination, source);
      result = 0;
    }
  }
  return result;
}

az_result az_hfsm_event_clone(az_hfsm_event original, az_hfsm_event* out_clone)
{
  az_result ret;

  switch (original.type)
  {
    default:
      out_clone = NULL;
      ret = AZ_ERROR_OUT_OF_MEMORY;
  }

  return ret;
}

az_result az_hfsm_event_destroy(az_hfsm_event* event)
{
  az_result ret;
  switch (event->type)
  {
    default:
        ret = AZ_ERROR_NOT_IMPLEMENTED;
  }

  return ret;
}
