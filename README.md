# Purring Cat

Purring Cat is a reference implementation of HVML.

## Introduction to HVML

With the development of Internet technology and applications, the Web front-end
development technology developed around HTML/CSS/JavaScript has developed
rapidly, and it can even be described as "thousands in a day". Five years ago,
front-end frameworks based on jQuery and Bootstrap became popular. Since 2019,
frameworks based on virtual DOM technology have been favored by front-end
developers, such as the famous React.js (https://reactjs.org/), 
Vue.js (https://cn.vuejs.org) etc. It is worth noting that WeChat
mini-programs and quick-apps etc. also use the virtual DOM technology
to build application frameworks.

The so-called "virtual DOM" refers to a front-end application that uses
JavaScript to create and maintain a virtual document object tree.
Application scripts do not directly manipulate the real DOM tree.
In the virtual DOM tree, some process control based on data is realized
through some special attributes, such as conditions and loops.
Virtual DOM technology provides the following benefits:

1. Because the script does not use the script program to directly
   manipulate the real DOM tree, on the one hand, the existing framework
   simplifies the complexity of front-end development, and on the other hand,
   it reduces the dynamic modification of page content by optimizing the
   operation of the real DOM tree. Frequent operations on the DOM tree can
   improve page rendering efficiency and user experience.

1. Through the virtual DOM technology, the modification of a certain data
   by the program can directly reflect the content of the data-bound page,
   and the developer does not need to actively or directly call the relevant
   interface to operate the DOM tree. This technology provides so-called
   "responsive" programming, which greatly reduces the workload of developers.

Front-end frameworks represented by React.js and Vue.js have achieved
great success, but have the following shortcomings and shortcomings:

1. These technologies are based on mature Web standards and require browsers
   thae fully support the relevant front-end specifications to run, so they
   cannot be applied to other occasions. For example, if you want to use
   this kind of technology in Python scripts, there is currently no solution;
   for example, in traditional GUI application programming, you cannot use
   the benefits of this technology.
1. These technologies implement data-based conditions and loop flow control
   by introducing virtual attributes such as v-if, v-else, and v-for. However,
   this method brings a sharp drop in code readability, and a drop in code
   readability The maintainability of the code has fallen. As an example of
   Vue.js below:

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
universal, elegant and easy-to-learn markup language HVML (the
Hybrid Virtual Markup Language) based on the idea of virtual DOM.
HVML is a general dynamic markup language, mainly used to generate
actual XML/HTML document content. HVML realizes the ability to
dynamically generate and update XML/HTML documents through
data-driven action tags and preposition attributes; HVML also provides
methods to integrate with existing programming languages, such as C/C++,
Python, Lua, and JavaScript. This can support more complex functions.

For more information about HVML, please refer to the following articles:

- [A brief introduction to HVML](https://github.com/HVML/hvml-docs/blob/master/zh/brief-introduction-to-hvml-zh.md)
- [Overview of HVML](https://github.com/HVML/hvml-docs/blob/master/zh/hvml-overview-zh.md)

## Source Tree of Purring Cat

Purring Cat is a reference implementation of HVML. It is mainly wrotten
in C/C++ language and provides bindings for Python.

The source tree of Purring Cat contains the following modules:

- `include/`: The global header files.
- `parser/`: The HVML parser. The parser reads a HVML document and output a vDOM.
- `interpreter/`: The interpreter of vDOM.
- `json-eval/`: The parser of JSON evaluation expression.
- `json-objects/`: The built-in dynamic JSON objects.
- `web-render/`: A HTML/CSS renderer without JavaScript; It is derived from hiWebKit.
- `bindings/`: The bindings for Python, Lua, and other programming languages.
- `test/`: The unit test programs.

## Current Status

Currently, a loosely collaborative group is actively developing Purring Cat.

Now, we have the initial code for the following module(s):

- HVML Parser.

We welcome anybody can take part in the development and contribute your effort!

## Building

### Commands

To build:

```
rm -rf debug && cmake -B debug && cmake --build debug
```

To build with debug:

```
rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -B debug && cmake --build debug
```

To build with verbose information:

```
rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -B debug && cmake --build debug
```

To build the test program:

```
./debug/test/parser/hp ./test/parser/test/sample.hvml && echo yes
```

To test it with `ctest`:

```
pushd debug/test/parser && ctest -vv; popd

```

To run the test program with Valgrind (build the library with debug first):

```
valgrind --leak-check=full ./debug/test/parser/hp ./test/parser/test/sample.hvml && echo yes
```

### Using the test samples

1. Write any HVML file (`.hvml`) for test in `./test/parser/test`.
1. Put its related output file (`.hvml.output`) in `./test/parser/test`.
1. Run `rm -rf debug && cmake -DCMAKE_BUILD_TYPE=Debug -B debug && cmake --build debug`.
1. Run `pushd debug/test/parser && ctest -VV; popd`

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
