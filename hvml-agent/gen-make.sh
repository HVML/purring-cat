#!/bin/bash
cp ../build/parser/src/libhvml_parser.so ./
cp ../build/interpreter/src/libhvml_interpreter.so ./

$ACE_ROOT/bin/mpc.pl -type make hvml-agent.mpc
