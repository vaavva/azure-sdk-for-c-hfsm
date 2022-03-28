// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief This header defines the types and functions your application uses to read or write JSON
 * objects.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_JSON_INTERNAL_H
#define _az_JSON_INTERNAL_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

// API_REVIEW: az_json_reader copy constructor.

/**
 * @brief Creates a copy of an #az_json_reader.
 *
 * @param[out] out_json_reader A pointer to an #az_json_reader instance to initialize.
 * @param[in] in_json_reader A pointer to an #az_json_reader to copy.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK The #az_json_reader is initialized successfully.
 * @retval other Initialization failed.
 *
 * @remarks An instance of #az_json_reader must not outlive the lifetime of the JSON payload within
 * the \p json_buffer.
 */
AZ_NODISCARD az_result _az_json_reader_clone(
    az_json_reader in_json_reader,
    az_json_reader* out_json_reader);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_JSON_INTERNAL_H
