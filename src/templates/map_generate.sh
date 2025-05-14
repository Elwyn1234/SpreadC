#!/bin/env bash

config_file=mapConfig.json
template_source_filepath=map.c.template
template_header_filepath=map.h.template
generated_source_filepath=../generated/map.c
generated_header_filepath=../generated/map.h



cp map_utils.c ../generated/map_utils.c
cp map_utils.h ../generated/map_utils.h



[[ -f "$generated_source_filepath" ]] && mv "$generated_source_filepath" /tmp/

cat >> "$generated_source_filepath" << HEREDOC
#include "map.h"
#include "map_utils.h"
#include "../common.h"
#include "../types.h"

HEREDOC

type_list=$(jq -er .[].name "$config_file")
for name in $type_list; do
  echo Generating $name source && \
      type_map_postfix=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_map_postfix' "$config_file") && \
      type_array=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_array' "$config_file") && \
      type_node=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_node' "$config_file") && \
      type_key=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_key' "$config_file") && \
      type_value=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_value' "$config_file") && \
      function_prefix_array=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .function_prefix_array' "$config_file") && \
      comparator_function_name=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .comparator_function_name' "$config_file") && \
  if [[ $? -ne 0 ]]; then
    echo "[ERROR] jq failed"
    exit 1
  fi
  cat "$template_source_filepath" | \
      sed "s@<TYPE_MAP_POSTFIX>@$type_map_postfix@g" | \
      sed "s@<TYPE_ARRAY>@$type_array@g" | \
      sed "s@<TYPE_NODE>@$type_node@g" | \
      sed "s@<TYPE_KEY>@$type_key@g" | \
      sed "s@<TYPE_VALUE>@$type_value@g" | \
      sed "s@<FUNCTION_PREFIX_ARRAY>@$function_prefix_array@g" | \
      sed "s@<COMPARATOR_FUNCTION_NAME>@$comparator_function_name@g" \
      >> "$generated_source_filepath"
done



[[ -f "$generated_header_filepath" ]] && mv "$generated_header_filepath" /tmp/

cat >> "$generated_header_filepath" << HEREDOC
#include "../types.h"
#include "dArray.h"
#ifndef _MAP_H
#define _MAP_H

HEREDOC

type_list=$(jq -er .[].name "$config_file")
for name in $type_list; do
  echo Generating $name header && \
      type_map_postfix=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_map_postfix' "$config_file") && \
      type_array=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_array' "$config_file") && \
      type_node=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_node' "$config_file") && \
      type_key=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_key' "$config_file") && \
      type_value=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .type_value' "$config_file") && \
      function_prefix_array=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .function_prefix_array' "$config_file") && \
      comparator_function_name=$(jq -er --arg name "$name" '.[] | select(.name == $name) | .comparator_function_name' "$config_file") && \
  if [[ $? -ne 0 ]]; then
    echo "[ERROR] jq failed"
    exit 1
  fi
  cat "$template_header_filepath" | \
      sed "s@<TYPE_MAP_POSTFIX>@$type_map_postfix@g" | \
      sed "s@<TYPE_ARRAY>@$type_array@g" | \
      sed "s@<TYPE_NODE>@$type_node@g" | \
      sed "s@<TYPE_KEY>@$type_key@g" | \
      sed "s@<TYPE_VALUE>@$type_value@g" | \
      sed "s@<FUNCTION_PREFIX_ARRAY>@$function_prefix_array@g" | \
      sed "s@<COMPARATOR_FUNCTION_NAME>@$comparator_function_name@g" \
      >> "$generated_header_filepath"
done

cat >> "$generated_header_filepath" << HEREDOC
#endif

HEREDOC

