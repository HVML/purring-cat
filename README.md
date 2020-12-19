# Purring Cat

Purring Cat is a reference implementation of HVML.

- [Introduction to HVML](#introduction-to-hvml)
- [Source Tree of Purring Cat](#source-tree-of-purring-cat)
- [Current Status](#current-status)
- [Building](#building)
   + [Commands](#commands)
   + [Using the test samples](#using-the-test-samples)
- [Building in Windows](#building-in-windows)
- [Contributors](#contributors)
- [Copying](#copying)

## Introduction to HVML

With the development of Internet technology and applications, the Web front-end
development technology around HTML/CSS/JavaScript has evolved
rapidly, and it can even be described as "thousand miles in a day". Five years ago,
front-end frameworks based on jQuery and Bootstrap became popular. Since 2019,
frameworks based on virtual DOM (Document Object Model) technology have been favored 
by front-end developers, such as the famous React.js (https://reactjs.org/), 
Vue.js (https://cn.vuejs.org) etc. It is worth noting that WeChat
mini-programs and quick-apps etc, also use the virtual DOM technology
to build application frameworks at the same time.

The so-called "virtual DOM" refers to a front-end application that uses
JavaScript to create and maintain a virtual DOM tree.
Application scripts do not directly manipulate the real DOM tree.
In the virtual DOM tree, some process control based on data is realized
through some special attributes, such as conditions and loops.
virtual DOM technology provides the following benefits:

1. Because the script does not directly manipulate the real DOM tree. On the one hand, 
   the existing framework    simplifies the complexity of front-end development, 
   on the other hand,  it reduces the frequent operations on the DOM tree through 
   dynamic modification of page content by optimizing the operation of the real DOM tree, 
   thus improving page rendering efficiency and user experience.
   
2. With the virtual DOM technology, the modification of a certain data
   by the program can directly be reflected on the content of the data-bound page,
   and the developer does not need to actively or directly call the relevant
   interface to operate the DOM tree. This technology provides so-called
   "responsive" programming, which greatly reduces the workload of developers.

Front-end frameworks represented by React.js and Vue.js have achieved
great success, but have the following deficiencies and shortcomings:

1. These technologies are based on mature Web standards and require browsers
   that fully support the relevant front-end specifications to run, so they
   cannot be applied to other occasions. For example, if you want to use
   this kind of technology in Python scripts, there is currently no solution;
   another example is in traditional GUI application programming, you cannot benefit
   from this technology.
   
2. These technologies implement data-based conditions and loop flow controls
   by introducing virtual attributes such as `v-if`, `v-else`, and `v-for`. However,
   this method brings a sharp drop in code readability, which in turn brings drop of 
   code maintainability. Below is an example in Vue.js:

```html
<div v-if="Math.random() > 0.5">
  Now you see "{{ name }}"
</div>
<div v-else>
  Now you don't
</div>
```

During the development of [HybridOS](https://hybridos.fmsoft.cn),
[Vincent Wei](https://github.com/VincentWei) proposed a complete,
general purpose, elegant and easy-to-learn markup language, HVML (the
Hybrid Virtual Markup Language), based on the idea of virtual DOM.
HVML is a general purpose dynamic markup language, mainly used to generate
actual XML/HTML document content. HVML realizes the ability to
dynamically generate and update XML/HTML documents through
data-driven action tags and preposition attributes; HVML also provides
methods to integrate with existing programming languages, such as C/C++,
Python, Lua, and JavaScript, thus supporting more complex functions.

The classical `helloworld` program in HVML looks like:

```html
<!DOCTYPE hvml>
<hvml target="html" script="python">
    <head>
        <init as="_" with="https://foo.bar/messages/$_SYSTEM.locale">
        </init>

        <title>Hello, world!</title>
    </head>

    <body>
        <p>$_("Hello, world!")</p>
    </body>

</hvml>
```

Or,

```html
<!DOCTYPE hvml>
<hvml target="html" script="python">
    <head>
        <title>Hello, world!</title>

        <init as="messages">
            {
              "zh_CN" : "世界，您好！",
              "en_US" : "Hello, world!"
            }
        </init>
    </head>

    <body>
        <p>
            <choose on="$messages" to="update" by="KEY: $_SYSTEM.locale">
                <update on="$@" textContent="$?" />
                <except on="KeyError">
                    No valid locale defined.
                </except>
            </choose>
        </p>
    </body>
</hvml>
```

For more information about HVML, please refer to the following articles:

- [A brief introduction to HVML](https://github.com/HVML/hvml-docs/blob/master/zh/brief-introduction-to-hvml-zh.md) - Chinese Version
- [Overview of HVML](https://github.com/HVML/hvml-docs/blob/master/zh/hvml-overview-zh.md) - Chinese Version

## Source Tree of Purring Cat

Purring Cat is a reference implementation of HVML. It is mainly written
in C/C++ language and provides bindings for Python.

The source tree of Purring Cat contains the following modules:

- `include/`: The global header files.
- `parser/`: The HVML parser. The parser reads a HVML document and outputs a vDOM.
- `interpreter/`: The interpreter of vDOM.
- `json-eval/`: The parser of JSON evaluation expression.
- `json-objects/`: The built-in dynamic JSON objects.
- `web-renderer/`: A HTML/CSS renderer without JavaScript; It is derived from hiWebKit.
- `bindings/`: The bindings for Python, Lua, and other programming languages.
- `test/`: The unit test programs.
- `docs/`: Some notes for developers.

## Current Status

This project was launched in Aug. 2020.

Currently, a loosely collaborative team is actively developing Purring Cat.
Now, we have the initial code for the following module(s):

- HVML Parser.

We welcome anybody to take part in the development and contribute your effort!

For the community conduct, please refer to [Code of Conduct](CODE_OF_CONDUCT.md).

For the coding style, please refer to [HybridOS-Code-and-Development-Convention](https://github.com/FMSoftCN/hybridos/blob/master/docs/specs/HybridOS-Code-and-Development-Convention.md).

## Building

### Commands

To build:

```
rm -rf build && cmake -B build && cmake --build build
```

To build with Debug:

```
rm -rf build && cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build
```

To build with verbose information:

```
rm -rf build && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -B build && cmake --build build
```

To run the test-parser program:

```
./build/test/parser/hp ./test/parser/test/sample.hvml && echo yes
```

To run the test-parser-dom-clone program:

```
./build/test/parser/hp -c ./test/parser/test/sample.hvml && echo yes
```

To run the test-interpreter program:

...
./build/test/interpreter/interpreter ./test/interpreter/test/calculator.hvml && echo yes
...

To test it with `ctest`:

```
pushd build/test/parser && ctest -VV; popd

```

To run the test program with Valgrind (build the library with Debug first):

```
valgrind --leak-check=full ./build/test/parser/hp ./test/parser/test/sample.hvml && echo yes
```

### Using the test samples

1. Write any HVML file (`.hvml`) for test in `./test/parser/test`.
1. Put its related output file (`.hvml.output`) in `./test/parser/test`.
1. Run `rm -rf build && cmake -DCMAKE_BUILD_TYPE=Debug -B build && cmake --build build`.
1. Run `pushd build/test/parser && ctest -VV; popd`


## Building in Windows

### Prerequisites

1. Visual Studio, downloadable from http://www.visualstudio.com
2. JDK, either Oracle JDK or OpenJDK.
3. Maven, available in http://maven.apache.org
4. CMake, 3.16 or later
5. Python 3, available from https:?/www.python.org. Better to use 3.7 or 3.8. Note: Don't install the python 3 from Microsoft Windows Store. CMake cannot detect it.
6. Flex and bison Windows port. It is better to use WinFlexBison from https://github.com/lexxmark/winflexbison

### Steps

1. Make all required building tools are already in your PATH environment.
2. Start a Visual Studio building environment, like 'x64 Native Tool Command Prompt for VS 2019'
3. Enter purring-cat directory, execute commands:
```
   mkdir build
   cd build
   cmake ..
   "cmake --build . --config Release" or "cmake --build . --config Debug", according to what kind of configuration you want in Windows.
```   

### Other documents

- [Setup dev and test environment for HVML using docker](docs/setup-dev-env-using-docker-en.md)

## Contributors

- Freemine
- Tian Siyuan
- Vincent Wei

## Copying

Copyright (C) 2020, The Purring Cat Team.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.


[Beijing FMSoft Technologies Co., Ltd.]: https://www.fmsoft.cn
[FMSoft Technologies]: https://www.fmsoft.cn
[FMSoft]: https://www.fmsoft.cn
[HybridOS Official Site]: https://hybridos.fmsoft.cn
[HybridOS]: https://hybridos.fmsoft.cn

[MiniGUI]: http:/www.minigui.com
[WebKit]: https://webkit.org
[HTML 5.3]: https://www.w3.org/TR/html53/
[DOM Specification]: https://dom.spec.whatwg.org/
[WebIDL Specification]: https://heycam.github.io/webidl/
[CSS 2.2]: https://www.w3.org/TR/CSS22/
[CSS Box Model Module Level 3]: https://www.w3.org/TR/css-box-3/

[Vincent Wei]: https://github.com/VincentWei
