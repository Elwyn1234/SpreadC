#!/bin/env bash

type_list="String:String Int:int"

[[ -f src/dArray.c ]] && mv src/dArray.c /tmp/

cat >> src/dArray.c << HEREDOC
#include "dArray.h"

#include <stdlib.h>

HEREDOC

for type_map in $type_list; do
  type_name=$(echo $type_map | cut -f1 -d:)
  type=$(echo $type_map | cut -f2 -d:)
  cat src/templates/dArrayBody.c.template | \
      sed "s/<TYPE_NAME>/$type_name/g" | \
      sed "s/<TYPE>/$type/g" \
      >> src/dArray.c
done



[[ -f src/dArray.h ]] && mv src/dArray.h /tmp/

cat >> src/dArray.h << HEREDOC
#include "types.h"
#ifndef _DARRAY_H
#define _DARRAY_H

HEREDOC

for type_map in $type_list; do
  type_name=$(echo $type_map | cut -f1 -d:)
  type=$(echo $type_map | cut -f2 -d:)
  cat src/templates/dArrayHead.h.template | \
      sed "s/<TYPE_NAME>/$type_name/g" | \
      sed "s/<TYPE>/$type/g" \
      >> src/dArray.h
done

cat >> src/dArray.h << HEREDOC
#endif

HEREDOC

