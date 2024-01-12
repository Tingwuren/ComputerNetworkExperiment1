#!/bin/bash
gnome-terminal -e "bash -c './datalink -f -u -p 59147 -l afu.log A; exec bash'"
gnome-terminal -e "bash -c './datalink -f -u -p 59147 -l bfu.log B; exec bash'"

