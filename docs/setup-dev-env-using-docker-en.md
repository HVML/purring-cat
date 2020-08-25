# Setup dev and test environment for HVML using docker

Linux（Ubuntu）is the recommended developmentt and testing environment for HVML. However, some people are using macOS or Windows systems. The following are the steps  to setup dev and test environmnt for HVML using docker on macOS. 

Similiar steps are used On Windows.

### Step 1, install and start docker

Download docker at https://download.docker.com/mac/stable/Docker.dmg

Install it and start running it.

### Step 2, get and run Ubuntu image

```
# docker pull ubuntu
# docker run -it ubuntu /bin/bash 
```

The Ubuntu image get at this time is:

```
20.04.1 LTS (Focal Fossa)
```

### Step 3, install software packages used for development and testing

```
# apt-get update
# apt-get install -y git cmake g++ valgrind python3
```
Note that if g++ is not installed，the following error message is displayed：
```
gcc: fatal error: cannot execute 'cc1plus': execvp: No such file or directory
compilation terminated.
```
With the evolvement of the project, other tools could be used. Install them using the above command.

### Step 4, Develop and test

For example:
```
# git clone https://github.com/HVML/purring-cat.git
# cd purring-cat/parser
```
Read the README.md file in this directory and try the steps mentioned inside.

