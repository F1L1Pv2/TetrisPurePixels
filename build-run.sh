#!/bin/bash
echo "------------------------------"
echo "            Build"
echo "------------------------------"
set -xe
sh build.sh
sh run.sh