// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

/** @file   iothub_message.h
 *    @brief  The @c IoTHub_Message component encapsulates one message that
 *           can be transferred by an IoT hub client.
 */

#ifndef IOTHUB_MESSAGE_H
#define IOTHUB_MESSAGE_H

#include <stdbool.h>

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

  /** @brief Enumeration specifying the status of calls to various
   *  APIs in this module.
   */
  typedef enum
  {
    IOTHUB_MESSAGE_OK,
    IOTHUB_MESSAGE_INVALID_ARG,
    IOTHUB_MESSAGE_INVALID_TYPE,
    IOTHUB_MESSAGE_ERROR
  } IOTHUB_MESSAGE_RESULT;

  /** @brief Enumeration specifying the content type of a given message.
   */
  typedef enum
  {
    IOTHUBMESSAGE_BYTEARRAY,
    IOTHUBMESSAGE_STRING,
    IOTHUBMESSAGE_UNKNOWN
  } IOTHUBMESSAGE_CONTENT_TYPE;

  /** @brief Handle representing an IoT Hub message */
  typedef struct IOTHUB_MESSAGE_HANDLE_DATA_TAG* IOTHUB_MESSAGE_HANDLE;

  /**
   * @brief   Creates a new IoT hub message from a byte array. The type of the
   *          message will be set to @c IOTHUBMESSAGE_BYTEARRAY.
   *
   * @param   byteArray   The byte array from which the message is to be created.
   * @param   size        The size of the byte array.
   *
   * @return  A valid #IOTHUB_MESSAGE_HANDLE if the message was successfully
   *          created or @c NULL in case an error occurs.
   */
  IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromByteArray(
      const unsigned char* byteArray,
      size_t size);

  /**
   * @brief   Creates a new IoT hub message from a null terminated string.  The
   *          type of the message will be set to @c IOTHUBMESSAGE_STRING.
   *
   * @param   source  The null terminated string from which the message is to be
   *                  created.
   *
   * @return  A valid #IOTHUB_MESSAGE_HANDLE if the message was successfully
   *          created or @c NULL in case an error occurs.
   */
  IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromString(const char* source);

  /**
   * @brief   Creates a new IoT hub message with the content identical to that
   *          of the @p iotHubMessageHandle parameter.
   *
   * @param   iotHubMessageHandle Handle to the message that is to be cloned.
   *
   * @return  A valid #IOTHUB_MESSAGE_HANDLE if the message was successfully
   *          cloned or @c NULL in case an error occurs.
   */
  IOTHUB_MESSAGE_HANDLE IoTHubMessage_Clone(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Fetches a pointer and size for the data associated with the IoT
   *          hub message handle. If the content type of the message is not
   *          @c IOTHUBMESSAGE_BYTEARRAY then the function returns
   *          @c IOTHUB_MESSAGE_INVALID_ARG.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   buffer              Pointer to the memory location where the
   *                              pointer to the buffer will be written.
   * @param   size                The size of the buffer will be written to
   *                              this address.
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the byte array was fetched successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_GetByteArray(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const unsigned char** buffer,
      size_t* size);

  /**
   * @brief   Returns the null terminated string stored in the message.
   *          If the content type of the message is not @c IOTHUBMESSAGE_STRING
   *          then the function returns @c NULL. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  @c NULL if an error occurs or a pointer to the stored null
   *          terminated string otherwise.
   */
  const char* IoTHubMessage_GetString(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Returns the content type of the message given by parameter
   *          @c iotHubMessageHandle.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @remarks This function retrieves the standardized type of the payload, which indicates if @c
   * iotHubMessageHandle was created using a String or a Byte Array.
   *
   * @return  An #IOTHUBMESSAGE_CONTENT_TYPE value.
   */
  IOTHUBMESSAGE_CONTENT_TYPE IoTHubMessage_GetContentType(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets the content-type of the message payload, as per supported values on RFC 2046.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @param   contentType String defining the type of the payload (e.g., text/plain).
   *
   * @return  An #IOTHUB_MESSAGE_RESULT value.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentTypeSystemProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* contentType);

  /**
   * @brief   Returns the content-type of the message payload, if defined. No new memory is
   * allocated, the caller is not responsible for freeing the memory.  The memory is valid until
   * IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A string with the content-type value if defined (or NULL otherwise).
   */
  const char* IoTHubMessage_GetContentTypeSystemProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets the content-encoding of the message payload, as per supported values on RFC 2616.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @param   contentEncoding String defining the encoding of the payload (e.g., utf-8).
   *
   * @return  An #IOTHUB_MESSAGE_RESULT value.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentEncodingSystemProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* contentEncoding);

  /**
   * @brief   Returns the content-encoding of the message payload, if defined. No new memory is
   * allocated, the caller is not responsible for freeing the memory. The memory is valid until
   * IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A string with the content-encoding value if defined (or NULL otherwise).
   */
  const char* IoTHubMessage_GetContentEncodingSystemProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets a property on a IoT Hub message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @param   key name of the property to set. Note that when sending messages via the HTTP
   * transport, this value must not contain spaces.
   *
   * @param   value of the property to set.
   *
   *            @b NOTE: The accepted character sets for the key name and value parameters are
   * dependent on different factors, such as the protocol being used. For more information on the
   * character sets accepted by Azure IoT Hub, see <a
   * href="https://docs.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-construct">Create and
   * read IoT Hub messages</a>.
   *
   * @return  An #IOTHUB_MESSAGE_RESULT value indicating the result of setting the property.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* key,
      const char* value);

  /**
   * @brief   Gets a IoT Hub message's properties item. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @param   key name of the property to retrieve.
   *
   * @return  A string with the property's value, or NULL if it does not exist in the properties
   * list.
   */
  const char* IoTHubMessage_GetProperty(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle, const char* key);

  /**
   * @brief   Gets the messageId from the IOTHUB_MESSAGE_HANDLE. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the messageId.
   */
  const char* IoTHubMessage_GetMessageId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets the messageId for the IOTHUB_MESSAGE_HANDLE.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   messageId Pointer to the memory location of the messageId
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the messageId was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageId(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* messageId);

  /**
   * @brief   Gets the CorrelationId from the IOTHUB_MESSAGE_HANDLE. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the Correlation Id.
   */
  const char* IoTHubMessage_GetCorrelationId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets the CorrelationId for the IOTHUB_MESSAGE_HANDLE.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   correlationId Pointer to the memory location of the messageId
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the CorrelationId was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetCorrelationId(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* correlationId);

  /**
   * @brief   Gets the output name from the IOTHUB_MESSAGE_HANDLE. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the Output Id.
   */
  const char* IoTHubMessage_GetOutputName(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets output for named queues. CAUTION: SDK user should not call it directly, it is for
   * internal use only.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   outputName Pointer to the queue to output message to
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the outputName was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetOutputName(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* outputName);

  /**
   * @brief   Gets the input name from the IOTHUB_MESSAGE_HANDLE. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the Input Id.
   */
  const char* IoTHubMessage_GetInputName(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets input for named queues. CAUTION: SDK user should not call it directly, it is for
   * internal use only.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   inputName Pointer to the queue to input message to
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the inputName was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetInputName(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* inputName);

  /**
   * @brief   Gets the module name from the IOTHUB_MESSAGE_HANDLE. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the connection module Id.
   */
  const char* IoTHubMessage_GetConnectionModuleId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets connection module ID. CAUTION: SDK user should not call it directly, it is for
   * internal use only.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   connectionModuleId Pointer to the module ID of connector
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the connectionModuleId was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetConnectionModuleId(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* connectionModuleId);

  /**
   * @brief   Gets the connection device ID from the IOTHUB_MESSAGE_HANDLE. No new memory is
   * allocated, the caller is not responsible for freeing the memory. The memory is valid until
   * IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the connection device Id.
   */
  const char* IoTHubMessage_GetConnectionDeviceId(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets the message creation time in UTC.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   messageCreationTimeUtc Pointer to the message creation time as null-terminated string
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the messageCreationTimeUtc was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageCreationTimeUtcSystemProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* messageCreationTimeUtc);

  /**
   * @brief   Gets the message creation time in UTC from the IOTHUB_MESSAGE_HANDLE. No new memory is
   * allocated, the caller is not responsible for freeing the memory. The memory is valid until
   * IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the message creation time in UTC.
   */
  const char* IoTHubMessage_GetMessageCreationTimeUtcSystemProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets the message user id. CAUTION: SDK user should not call it directly, it is for
   * internal use only.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   userId Pointer to the message user id as null-terminated string
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the userId was set successfully or an error code
   * otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageUserIdSystemProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* userId);

  /**
   * @brief   Gets the message user id from the IOTHUB_MESSAGE_HANDLE. No new memory is allocated,
   *          the caller is not responsible for freeing the memory. The memory
   *          is valid until IoTHubMessage_Destroy is called on the message.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  A const char* pointing to the message user id.
   */
  const char* IoTHubMessage_GetMessageUserIdSystemProperty(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Sets connection device Id. CAUTION: SDK user should not call it directly, it is for
   * internal use only.
   *
   * @param   iotHubMessageHandle Handle to the message.
   * @param   connectionDeviceId Pointer to the device ID of connector
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the DiagnosticData was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetConnectionDeviceId(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
      const char* connectionDeviceId);

  /**
   * @brief   Marks a IoT Hub message as a security message. CAUTION: security messages are special
   * messages not easily accessable by the user.
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  Returns IOTHUB_MESSAGE_OK if the security message was set successfully
   *          or an error code otherwise.
   */
  IOTHUB_MESSAGE_RESULT IoTHubMessage_SetAsSecurityMessage(
      IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   returns if this message is a IoT Hub security message or not
   *
   * @param   iotHubMessageHandle Handle to the message.
   *
   * @return  Returns true if the message is a security message false otherwise.
   */
  bool IoTHubMessage_IsSecurityMessage(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

  /**
   * @brief   Frees all resources associated with the given message handle.
   *
   * @param   iotHubMessageHandle Handle to the message.
   */
  void IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);

#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_MESSAGE_H */
