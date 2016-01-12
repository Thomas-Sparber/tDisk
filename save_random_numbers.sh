#!/bin/bash

for number in {0..2}; do value=$number; for i in {1..9}; do value=$value$value; done; echo $value | sudo dd of=/dev/td bs=512 count=1 seek=$number; done
