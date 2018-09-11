#!/bin/bash

PE_ID=$1
DIR=$(cd $(dirname $0) && pwd)
(cd $DIR && ./networkcom $PE_ID)
