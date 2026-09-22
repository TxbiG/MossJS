# JavaMoss

JavaMoss is a JNI binding layer for [MossFramework](../MossFramework). It exposes an
idiomatic, ownership-safe Java API instead of exposing C++ layouts or symbols directly.

## Current coverage

- `MossWindow`: create, close, title, close state, and dimensions.
- `Moss`: event polling, clipboard text, and CPU/system memory queries.
- `Vec2`: Java-side value type for future math-facing APIs.

## Build the native library

Install a JDK (not just a JRE), set `JAVA_HOME`, then configure:

```powershell
cmake -S . -B build -DMOSS_ROOT="C:/Users/TobyG/Documents/MossFramework"
cmake --build build --config Debug
```

Start Java with the library directory in `java.library.path`, or set
`-Dmoss.native.path=C:\absolute\path\to\moss_jni.dll`.

## Example

```java
try (var window = MossWindow.create("JavaMoss", 1280, 720)) {
    while (!window.shouldClose()) {
        Moss.pollEvents();
    }
}
```

## Design

Every native resource is an `AutoCloseable` Java owner around an opaque `long` handle.
Native methods are implementation details; public methods validate Java inputs before
crossing JNI. This keeps C++ template types, callbacks, and ownership rules out of the
Java ABI. Callbacks are intentionally deferred because they need a separate lifetime and
thread-attachment design.