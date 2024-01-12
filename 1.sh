#!/bin/bash
gnome-terminal -e "bash -c './datalink -u -p 59145 -d 7 -l au.log A; exec bash'"
gnome-terminal -e "bash -c './datalink -u -p 59145 -d 7 -l bu.log B; exec bash'"

