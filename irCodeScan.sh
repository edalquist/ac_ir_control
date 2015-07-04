#!/bin/bash

while true; do
	particle call ac-controller-1 sendLast
	particle call ac-controller-1 sendLast
	particle call ac-controller-1 incrCode
done;
