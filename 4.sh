#!/bin/bash
gnome-terminal -e "bash -c './datalink -f -p 59148 -l af.log A; exec bash'"
gnome-terminal -e "bash -c './datalink -f -p 59148 -l bf.log B; exec bash'"

