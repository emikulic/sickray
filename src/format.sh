#!/bin/sh
clang-format-9 -style=Google -i *.cc *.h
git status
