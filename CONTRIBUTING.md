# Contributing to MossJS

Thank you for contributing to **MossJS**, the JavaScript/WebAssembly binding for Moss Framework.

MossJS bridges Moss functionality to JavaScript and WebAssembly environments, so changes should consider both native-side behaviour and browser/runtime constraints.

## Before You Start

For public JavaScript API, WebAssembly ABI, or binding architecture changes, open an issue before substantial implementation.

Please search existing issues and pull requests before starting work.

## Development Requirements

- Git
- CMake
- C++17
- Emscripten
- A local Moss Framework checkout
- Node.js/npm where required by the web package/tooling
- A modern browser for runtime testing

## Building

MossJS requires an Emscripten toolchain.

A typical configuration is:

```bash
emcmake cmake -S . -B build
cmake --build build
```

Use the repository's browser examples and existing web configuration for runtime testing.

## Repository Structure

- `MossJS/` — JavaScript/WebAssembly binding source
- `web/` — web-facing integration
- `docs/` — documentation
- `examples/` — examples
- `performance/` — benchmarks
- `build/workflows/` — CI/build configuration
- `package.json` — JavaScript package metadata

## Areas for Contribution

- WebAssembly bindings
- JavaScript API design
- Browser platform support
- Rendering and GPU integration
- Audio
- Input
- Physics
- XR/WebXR
- GUI
- Packaging
- Examples
- Documentation
- Performance
- CI

## WebAssembly Guidelines

Keep the JavaScript/WebAssembly boundary explicit.

Consider:

- Memory ownership
- Typed arrays/views
- Native handle lifetime
- Main-thread restrictions
- Browser event loops
- Async APIs
- WebAudio
- WebGPU/WebGL
- WebXR
- Browser security restrictions

Do not assume that a desktop Moss backend can be exposed unchanged in a browser.

## JavaScript API Design

Prefer APIs that are predictable for JavaScript users while retaining clear Moss semantics.

Document whether an operation is:

- Synchronous
- Asynchronous
- Browser-only
- WebAssembly-only
- Subject to browser/user-gesture restrictions

## Runtime and Performance

Avoid unnecessary JS↔Wasm crossings in hot loops.

For high-frequency operations, consider batching data or providing APIs that minimise boundary transitions and copying.

## Testing

Test both build-time and runtime behaviour where practical:

- Emscripten compilation
- JavaScript module loading
- Browser execution
- Rendering
- Audio
- Input
- Physics
- Resource lifetime
- WebAssembly memory use
- XR/WebXR paths where available

For browser issues, include browser/version information.

## Commit Messages

Recommended prefixes:

```text
feat: add WebXR input bindings
fix: release WebAssembly texture handles
docs: add browser setup guide
test: add JS resource lifetime test
perf: reduce JS-Wasm calls during rendering
build: update Emscripten configuration
```

## Pull Requests

Include:

- Description
- Browsers tested
- Emscripten version
- Build configuration
- Runtime examples where appropriate
- API/ABI compatibility notes
- Performance information for hot-path changes

## Licence

MossJS is distributed under the **MIT License**. Contributions should be compatible with the repository's licence and applicable Emscripten/Moss/third-party licence requirements.

Thank you for contributing.
