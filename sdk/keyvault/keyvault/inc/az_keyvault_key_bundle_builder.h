// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_KEY_BUNDLE_H
#define AZ_KEYVAULT_KEY_BUNDLE_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

AZ_ACTION_TYPE(az_action, int)

AZ_ACTION_TYPE(az_bool_action, bool)

AZ_ACTION_TYPE(az_int64_action, int64_t)

// a string

typedef struct {
  az_action done;
  az_span_action span;
} az_str_builder;

AZ_ACTION_TYPE(az_str_create, az_str_builder *)

// a sequence of strings

typedef struct {
  az_action done;
  az_str_create item;
} az_str_seq_builder;

AZ_ACTION_TYPE(az_str_seq_create, az_str_seq_builder *)

// a pair

typedef struct {
  az_action done;
  az_str_create key;
  az_str_create value;
} az_pair_builder;

AZ_ACTION_TYPE(az_pair_create, az_pair_builder *)

// a sequence of pairs

typedef struct {
  az_action done;
  az_pair_create item;
} az_pair_seq_builder;

AZ_ACTION_TYPE(az_pair_seq_create, az_pair_seq_builder *)

// KeyVault.JsonWebKey

typedef struct {
  az_action done;
  struct {
    az_str_create kid;
    az_str_create kty;
    az_str_seq_create key_ops;
    az_str_create n;
    az_str_create e;
    az_str_create d;
    az_str_create dp;
    az_str_create dq;
    az_str_create qi;
    az_str_create p;
    az_str_create q;
    az_str_create k;
    az_str_create key_hsm;
    az_str_create crv;
    az_str_create x;
    az_str_create y;
  } properties;
} az_keyvault_json_web_key_builder;

AZ_ACTION_TYPE(az_keyvault_json_web_key_create, az_keyvault_json_web_key_builder *)

// KeyVault.KeyAttributes

typedef struct {
  az_action done;
  struct {
    // Attributes
    az_bool_action enabled;
    az_int64_action nbf;
    az_int64_action exp;
    az_int64_action created;
    az_int64_action updated;
    // KeyAttributes
    az_str_create recovery_level;
  } properties;
} az_keyvault_key_attributes_builder;

AZ_ACTION_TYPE(az_keyvault_key_attriubtes_create, az_keyvault_key_attributes_builder *)

// KeyVault.KeyBundle

typedef struct {
  az_action done;
  struct {
    az_keyvault_json_web_key_create key;
    az_keyvault_key_attriubtes_create attributes;
    az_pair_seq_create tags;
    az_bool_action managed;
  } properties;
} az_keyvault_key_bundle_builder;

AZ_ACTION_TYPE(az_keyvault_key_bundle_create, az_keyvault_key_bundle_builder *)

#include <_az_cfg_suffix.h>

#endif
