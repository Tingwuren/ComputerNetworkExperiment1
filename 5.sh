#!/bin/bash
gnome-terminal -e "bash -c './datalink  -b 1e-4 -p 59149 -l afber.log A; exec bash'"
gnome-terminal -e "bash -c './datalink  -b 1e-4 -p 59149 -l bfber.log B; exec bash'"

