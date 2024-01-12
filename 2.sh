#!/bin/bash
gnome-terminal -e "bash -c './datalink -p 59146 -l a.log A; exec bash'"
gnome-terminal -e "bash -c './datalink -p 59146 -l b.log B; exec bash'"

