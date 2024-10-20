#!/bin/env bash

type_list="String:String Int:int MapNodeStringInt:MapNodeStringInt MapNodeIS:MapNodeIS"

[[ -f ../generated/dArray.c ]] && mv ../generated/dArray.c /tmp/

cat >> ../generated/dArray.c << HEREDOC
#include "dArray.h"

#include <stdlib.h>

HEREDOC

for type_map in $type_list; do
  type_name=$(echo $type_map | cut -f1 -d:)
  type=$(echo $type_map | cut -f2 -d:)
  cat dArrayBody.c.template | \
      sed "s/<TYPE_NAME>/$type_name/g" | \
      sed "s/<TYPE>/$type/g" \
      >> ../generated/dArray.c
done



[[ -f ../generated/dArray.h ]] && mv ../generated/dArray.h /tmp/

cat >> ../generated/dArray.h << HEREDOC
#include "../types.h"
#ifndef _DARRAY_H
#define _DARRAY_H

HEREDOC

for type_map in $type_list; do
  type_name=$(echo $type_map | cut -f1 -d:)
  type=$(echo $type_map | cut -f2 -d:)
  cat dArrayHead.h.template | \
      sed "s/<TYPE_NAME>/$type_name/g" | \
      sed "s/<TYPE>/$type/g" \
      >> ../generated/dArray.h
done

cat >> ../generated/dArray.h << HEREDOC
#endif

HEREDOC

