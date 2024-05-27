# Project Description

This is a project to test webgl and webgpu through C++. Based on OpenGL(GLES3), WebGPU(webgpu.h implementation：webgpu_cpp.h) from Emscripten, it is compiled into a WebAssembly module from C++ code, and ultimately uses webgl2 and webgpu in modern browsers.

- **Features**:
  - Support selecting to get context from canvas or OffscreenCanvas
  - Support multi-threaded rendering when using OffscreenCanvas (Rendering in a worker.)
  - Support selecting WebGL2 or WebGPU as rendering backend
  - ES6 style WebAssembly module

- **Known issues**:
  - It doesn't work yet when using webgl2 as the backend and directly geting context from canvas (not OffscreenCanvas). You can track this [issue #21954](https://github.com/emscripten-core/emscripten/issues/21954), and I may eventually resolve it.
  - Not necessarily working in safari.
  - Event monitoring cannot directly control the DragState member variable of the Event class, and can only use the global g-DragState variable in a bad way.

## 1. API and Usage

There is only one API at present, which requires two parameters:

- backend: String type, valid values: "webgl2" or "webgpu"(default)
- usingOffscreenCanvas: Boolean type, default value is true

```js
engine.startEngine(backend, usingOffscreenCanvas)
```

- Example:

  - **Note**: Need configure **"Cross-Origin-Opener-Policy:same-origin"** and **"Cross-Origin-Embedder-Policy:require-corp"** response headers in your server, check [https://developer.chrome.com/blog/coep-credentialless-origin-trial?hl=zh-cn](https://developer.chrome.com/blog/coep-credentialless-origin-trial?hl=zh-cn)

```js
<script type="module">
    import EngineCore from './lib/EngineCore.js'

    EngineCore().then(engine => {
        // engine.startEngine("webgl2", false);        // not working
        // engine.startEngine("webgl2", true);      // working
        engine.startEngine("webgpu", false);        // working
        // engine.startEngine("webgpu", true);      // working
    });
</script>
```

## 2. Compile and Build

- First, make a build directory

```ps
mkdir dist
cd dist
```

- Generate the Ninja project file

**Replace** the value of the parameter **"- DCMAKE-TOOLCHAIN-FILE"** with the absolute path of your Emscripten.cmake file.

- **Note**:
  - If **emscripten tool chain**(or call it **"emsdk"**) is not installed, you need to first download and install it according to this instructions: [https://emscripten.org/docs/getting_started/downloads.html](https://emscripten.org/docs/getting_started/downloads.html);
  - If there is no **Ninja** in your system env, you need to first download a from [https://github.com/ninja-build/ninja/releases](https://github.com/ninja-build/ninja/releases) and configure system environment variables for it!

```ps
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/env/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" -G "Ninja"
```

- Build

```ps
ninja
```

## 3. Run

- First, Install dependencies

```ps
pmpm i
```

- Prepare HTTPS service

Use OpenSSL tool to generate certificates for HTTPS protocol services:

```ps
.\openssl.exe req -nodes -new -x509 -keyout C:\Users\xrui94\Desktop\a\test_server.key -out C:\Users\xrui94\Desktop\a\test_server.cert
```

- Then, use app

After executing the command bellow, you can use the app by opening the url [http://localhost:3050/](http://localhost:3050/) in browser.

```ps
npm start
```
