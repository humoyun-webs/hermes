"use strict";(self.webpackChunkhermes_website=self.webpackChunkhermes_website||[]).push([[282],{3905:function(e,t,n){n.d(t,{Zo:function(){return d},kt:function(){return h}});var i=n(7294);function r(e,t,n){return t in e?Object.defineProperty(e,t,{value:n,enumerable:!0,configurable:!0,writable:!0}):e[t]=n,e}function a(e,t){var n=Object.keys(e);if(Object.getOwnPropertySymbols){var i=Object.getOwnPropertySymbols(e);t&&(i=i.filter((function(t){return Object.getOwnPropertyDescriptor(e,t).enumerable}))),n.push.apply(n,i)}return n}function l(e){for(var t=1;t<arguments.length;t++){var n=null!=arguments[t]?arguments[t]:{};t%2?a(Object(n),!0).forEach((function(t){r(e,t,n[t])})):Object.getOwnPropertyDescriptors?Object.defineProperties(e,Object.getOwnPropertyDescriptors(n)):a(Object(n)).forEach((function(t){Object.defineProperty(e,t,Object.getOwnPropertyDescriptor(n,t))}))}return e}function o(e,t){if(null==e)return{};var n,i,r=function(e,t){if(null==e)return{};var n,i,r={},a=Object.keys(e);for(i=0;i<a.length;i++)n=a[i],t.indexOf(n)>=0||(r[n]=e[n]);return r}(e,t);if(Object.getOwnPropertySymbols){var a=Object.getOwnPropertySymbols(e);for(i=0;i<a.length;i++)n=a[i],t.indexOf(n)>=0||Object.prototype.propertyIsEnumerable.call(e,n)&&(r[n]=e[n])}return r}var u=i.createContext({}),s=function(e){var t=i.useContext(u),n=t;return e&&(n="function"==typeof e?e(t):l(l({},t),e)),n},d=function(e){var t=s(e.components);return i.createElement(u.Provider,{value:t},e.children)},p="mdxType",c={inlineCode:"code",wrapper:function(e){var t=e.children;return i.createElement(i.Fragment,{},t)}},m=i.forwardRef((function(e,t){var n=e.components,r=e.mdxType,a=e.originalType,u=e.parentName,d=o(e,["components","mdxType","originalType","parentName"]),p=s(n),m=r,h=p["".concat(u,".").concat(m)]||p[m]||c[m]||a;return n?i.createElement(h,l(l({ref:t},d),{},{components:n})):i.createElement(h,l({ref:t},d))}));function h(e,t){var n=arguments,r=t&&t.mdxType;if("string"==typeof e||r){var a=n.length,l=new Array(a);l[0]=m;var o={};for(var u in t)hasOwnProperty.call(t,u)&&(o[u]=t[u]);o.originalType=e,o[p]="string"==typeof e?e:r,l[1]=o;for(var s=2;s<a;s++)l[s]=n[s];return i.createElement.apply(null,l)}return i.createElement.apply(null,n)}m.displayName="MDXCreateElement"},4311:function(e,t,n){n.r(t),n.d(t,{assets:function(){return d},contentTitle:function(){return u},default:function(){return h},frontMatter:function(){return o},metadata:function(){return s},toc:function(){return p}});var i=n(3117),r=n(102),a=(n(7294),n(3905)),l=["components"],o={id:"building-and-running",title:"Building and Running"},u=void 0,s={unversionedId:"building-and-running",id:"building-and-running",title:"Building and Running",description:"This document describes how to build and run Hermes as a standalone compiler and VM. To use Hermes in the context of a React Native app, see the React Native documentation.",source:"@site/../doc/BuildingAndRunning.md",sourceDirName:".",slug:"/building-and-running",permalink:"/docs/building-and-running",draft:!1,editUrl:"https://github.com/facebook/hermes/blob/HEAD/website/../doc/BuildingAndRunning.md",tags:[],version:"current",lastUpdatedAt:1676648895,formattedLastUpdatedAt:"Feb 17, 2023",frontMatter:{id:"building-and-running",title:"Building and Running"},sidebar:"docs",next:{title:"Building with Emscripten",permalink:"/docs/emscripten"}},d={},p=[{value:"Dependencies",id:"dependencies",level:2},{value:"Building on Linux and macOS",id:"building-on-linux-and-macos",level:2},{value:"Release Build",id:"release-build",level:2},{value:"Building on Windows",id:"building-on-windows",level:2},{value:"Running Hermes",id:"running-hermes",level:2},{value:"Executing JavaScript with Hermes",id:"executing-javascript-with-hermes",level:3},{value:"Compiling and Executing JavaScript with Bytecode",id:"compiling-and-executing-javascript-with-bytecode",level:2},{value:"Running Tests",id:"running-tests",level:2},{value:"Formatting Code",id:"formatting-code",level:2},{value:"AddressSanitizer (ASan) Build",id:"addresssanitizer-asan-build",level:2},{value:"Other Tools",id:"other-tools",level:3}],c={toc:p},m="wrapper";function h(e){var t=e.components,n=(0,r.Z)(e,l);return(0,a.kt)(m,(0,i.Z)({},c,n,{components:t,mdxType:"MDXLayout"}),(0,a.kt)("p",null,"This document describes how to build and run Hermes as a standalone compiler and VM. To use Hermes in the context of a React Native app, see the ",(0,a.kt)("a",{parentName:"p",href:"https://reactnative.dev/docs/getting-started"},"React Native")," documentation."),(0,a.kt)("h2",{id:"dependencies"},"Dependencies"),(0,a.kt)("p",null,"Hermes is a C++14 project. clang, gcc, and Visual C++ are supported. Hermes also requires cmake, git, ICU, Python, and zip. It builds with ",(0,a.kt)("a",{parentName:"p",href:"https://cmake.org"},"CMake")," and ",(0,a.kt)("a",{parentName:"p",href:"https://ninja-build.org"},"ninja"),"."),(0,a.kt)("p",null,"The Hermes REPL will also use libreadline, if available."),(0,a.kt)("p",null,"To install dependencies on Ubuntu:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"apt install cmake git ninja-build libicu-dev python zip libreadline-dev\n")),(0,a.kt)("p",null,"On Arch Linux:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"pacman -S cmake git ninja icu python zip readline\n")),(0,a.kt)("p",null,"On Mac via Homebrew:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"brew install cmake git ninja\n")),(0,a.kt)("h2",{id:"building-on-linux-and-macos"},"Building on Linux and macOS"),(0,a.kt)("p",null,"Hermes will place its build files in the current directory by default.\nYou can also give explicit source and build directories, use ",(0,a.kt)("inlineCode",{parentName:"p"},"--help")," on the build scripts to see how."),(0,a.kt)("p",null,"Create a base directory to work in, e.g. ",(0,a.kt)("inlineCode",{parentName:"p"},"~/workspace"),", and cd into it.\n(Tip: avoid naming it ",(0,a.kt)("inlineCode",{parentName:"p"},"hermes"),", as ",(0,a.kt)("inlineCode",{parentName:"p"},"hermes")," will be one of several subdirectories in the workspace).\nAfter ",(0,a.kt)("inlineCode",{parentName:"p"},"cd"),"ing, follow the steps below to generate the Hermes build system:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"git clone https://github.com/facebook/hermes.git\ncmake -S hermes -B build -G Ninja\n")),(0,a.kt)("p",null,"The build system has now been generated in the ",(0,a.kt)("inlineCode",{parentName:"p"},"build")," directory. To perform the build:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"cmake --build ./build\n")),(0,a.kt)("h2",{id:"release-build"},"Release Build"),(0,a.kt)("p",null,"The above instructions create an unoptimized debug build. The ",(0,a.kt)("inlineCode",{parentName:"p"},"-DCMAKE_BUILD_TYPE=Release")," flag will create a release build:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"cmake -S hermes -B build_release -G Ninja -DCMAKE_BUILD_TYPE=Release\ncmake --build ./build_release\n")),(0,a.kt)("h2",{id:"building-on-windows"},"Building on Windows"),(0,a.kt)("p",null,"To build on Windows using Visual Studio with a checkout in the ",(0,a.kt)("inlineCode",{parentName:"p"},"hermes")," directory:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"cmake -S hermes -B build -G 'Visual Studio 16 2019'\ncmake --build ./build\n")),(0,a.kt)("h2",{id:"running-hermes"},"Running Hermes"),(0,a.kt)("p",null,"The primary binary is the ",(0,a.kt)("inlineCode",{parentName:"p"},"hermes")," tool, which will be found at ",(0,a.kt)("inlineCode",{parentName:"p"},"build/bin/hermes"),". This tool compiles JavaScript to Hermes bytecode. It can also execute JavaScript, from source or bytecode or be used as a REPL."),(0,a.kt)("h3",{id:"executing-javascript-with-hermes"},"Executing JavaScript with Hermes"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"hermes test.js\n")),(0,a.kt)("h2",{id:"compiling-and-executing-javascript-with-bytecode"},"Compiling and Executing JavaScript with Bytecode"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"hermes -emit-binary -out test.hbc test.js\nhermes test.hbc\n")),(0,a.kt)("h2",{id:"running-tests"},"Running Tests"),(0,a.kt)("p",null,"To run the Hermes test suite:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"cmake --build ./build --target check-hermes\n")),(0,a.kt)("p",null,"To run Hermes against the test262 suite, you need to have a Hermes binary built\nalready and a clone of the ",(0,a.kt)("a",{parentName:"p",href:"https://github.com/tc39/test262/"},"test262 repo"),":"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"hermes/utils/testsuite/run_testsuite.py -b <hermes_build> <test262>\n")),(0,a.kt)("p",null,"E.g. if we configured at ",(0,a.kt)("inlineCode",{parentName:"p"},"~/hermes_build")," (i.e. ",(0,a.kt)("inlineCode",{parentName:"p"},"~/hermes_build/bin/hermes")," is\nan executable) and cloned test262 at ",(0,a.kt)("inlineCode",{parentName:"p"},"~/test262"),", then perform:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"hermes/utils/testsuite/run_testsuite.py -b ~/hermes_build ~/test262/test\n")),(0,a.kt)("p",null,"Note that you can also only test against part of a test suite, e.g. to run the\nIntl402 subset of the test262, you can specifiy a subdir:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"hermes/utils/testsuite/run_testsuite.py -b ~/hermes_build ~/test262/test/intl402\n")),(0,a.kt)("h2",{id:"formatting-code"},"Formatting Code"),(0,a.kt)("p",null,"To automatically format all your changes, you will need ",(0,a.kt)("inlineCode",{parentName:"p"},"clang-format"),", then\nsimply run:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"hermes/utils/format.sh\n")),(0,a.kt)("h2",{id:"addresssanitizer-asan-build"},"AddressSanitizer (ASan) Build"),(0,a.kt)("p",null," The ",(0,a.kt)("inlineCode",{parentName:"p"},"-HERMES_ENABLE_ADDRESS_SANITIZER=ON")," flag will create a ASan build:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"git clone https://github.com/facebook/hermes.git\ncmake -S hermes -B asan_build -G Ninja -D HERMES_ENABLE_ADDRESS_SANITIZER=ON\ncmake --build ./asan_build\n")),(0,a.kt)("p",null,"You can verify the build by looking for ",(0,a.kt)("inlineCode",{parentName:"p"},"asan")," symbols in the ",(0,a.kt)("inlineCode",{parentName:"p"},"hermes")," binary:"),(0,a.kt)("pre",null,(0,a.kt)("code",{parentName:"pre"},"nm asan_build/bin/hermes | grep asan\n")),(0,a.kt)("h3",{id:"other-tools"},"Other Tools"),(0,a.kt)("p",null,"In addition to ",(0,a.kt)("inlineCode",{parentName:"p"},"hermes"),", the following tools will be built:"),(0,a.kt)("ul",null,(0,a.kt)("li",{parentName:"ul"},(0,a.kt)("inlineCode",{parentName:"li"},"hdb"),": JavaScript command line debugger"),(0,a.kt)("li",{parentName:"ul"},(0,a.kt)("inlineCode",{parentName:"li"},"hbcdump"),": Hermes bytecode disassembler"),(0,a.kt)("li",{parentName:"ul"},(0,a.kt)("inlineCode",{parentName:"li"},"hermesc"),": Standalone Hermes compiler. This can compile JavaScript to Hermes bytecode, but does not support executing it."),(0,a.kt)("li",{parentName:"ul"},(0,a.kt)("inlineCode",{parentName:"li"},"hvm"),": Standalone Hermes VM. This can execute Hermes bytecode, but does not support compiling it.")))}h.isMDXComponent=!0}}]);