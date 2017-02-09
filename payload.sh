#!/bin/bash
#
# Proof of concept payload
#
# Just an interactive prompt will appear
#

SHELL="bash --rcfile <(cat ~/.bashrc; echo 'PS1=\"\u:\W >\"')"

echo "This is a dummy script file that only output text"
echo "You just successfully ran the payload"

echo "Entering a prompt (^d to exit)"
eval "$SHELL"
