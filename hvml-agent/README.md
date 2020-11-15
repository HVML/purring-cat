# How to compile the hvml-agent
NOTICE: Install the ACE first, reference to the file:
../third-party/how-to-install-ACE-in-linux.md

$ ./gen-make.sh
$ make -f Makefile.hvml_agent
$ ./copy-so.sh
$ ./run-test.sh