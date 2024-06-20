#!/bin/bash

warble ()
  {
  freq=$1
  for i in {1..8} ; do
    echo volume 100 
    echo tone 15,$1
    echo volume 10
    echo tone 15,$1
  done
  }

warble "1000";
warble "500";
warble "800";

