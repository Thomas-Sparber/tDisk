#!/bin/bash


tdisk_shell_exit() {
	echo Goodbye
}


trap tdisk_shell_exit EXIT
