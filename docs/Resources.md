# Resources


## Resources
- [Overview](/docs/#overview)
    - [Noise](/docs/), [Tween](/docs/)
    - [Font](/docs/), [Shader](/docs/), [Mesh](/docs/), [Texture](/docs/)
    - [Config](/docs/), [Json](/docs/), [PAK](/docs/), [Wav](/docs/),
    - [Animations](/docs/Animations.md#animations)

    
## Config
```.ini``` and ```.cfg```
```cpp
```
## Json
```.json```
```cpp
```

## Plain text
```.txt```
```cpp
```

## Curve
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Curve2
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Curve3
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Gradient
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## JSON
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## JSON
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## WAV
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Font
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Material
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Texture
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Mesh
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Haptic
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## InputEventType
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Tween
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_Window
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_Monitor
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_Curser
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_GammaRamp
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_VideoMode
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_Image
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_Timer
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_Time
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |

## Moss_Renderer
```cpp
Vec4i();
Vec4i(int x, int y, int z, int w);
```
Example
```cpp
```
| Variant | Function |
| --- | --- |



## Asset Lifecycle

Moss uses `Moss_AssetManager` for file-backed resources that need stable handles, reference counting, reload, unload, and development hot reload.

```cpp
Moss_AssetManagerDesc desc{};
desc.root_path = "assets";
desc.enable_hot_reload = true;
Moss_AssetManager* assets = Moss_AssetManagerCreate(&desc);

Moss_AssetData bytes = Moss_AssetReadFile(assets, "textures/player.png");
Moss_AssetFreeData(bytes);

Moss_AssetLoadDesc load{};
load.type = MOSS_ASSET_BINARY;
load.path = "data/config.json";
Moss_AssetHandle handle = Moss_AssetLoad(assets, &load);
Moss_AssetRetain(assets, handle);
Moss_AssetRelease(assets, handle);
Moss_AssetUnload(assets, handle);

Moss_AssetPollHotReload(assets);
Moss_AssetManagerDestroy(assets);
```

Rules:
- Use forward slashes in virtual paths.
- Mount physical folders or PCK archives under a virtual root.
- Use `Moss_AssetReadFile` for raw bytes from disk or PCK.
- Use `Moss_AssetLoad`, `Moss_AssetRetain`, `Moss_AssetRelease`, and `Moss_AssetUnload` for ownership.
- Use reload callbacks for development hot reload.

## PCK/PAK Archives

`PCKFile` creates and reads Moss pack archives. The format stores a table of UTF-8 virtual names and payload offsets. Encryption is optional and uses AES-256-GCM when Moss is built with OpenSSL support; otherwise files are stored plain.

```cpp
std::vector<std::pair<std::string, std::string>> files = {
    {"assets/player.png", "textures/player.png"},
    {"assets/config.json", "config/game.json"},
};
PCKFile::create("game.pck", files, nullptr);

Moss_AssetMountDesc mount{};
mount.virtual_root = "/game";
mount.pck_path = "game.pck";
Moss_AssetManagerMount(assets, &mount);
Moss_AssetData config = Moss_AssetReadFile(assets, "/game/config/game.json");
Moss_AssetFreeData(config);
```

## Typed Asset Helpers

Renderer, GPU, and audio headers expose convenience helpers that return `Moss_AssetHandle` values:

```cpp
Moss_TextureAssetDesc texture{};
texture.path = "textures/player.png";
Moss_AssetHandle tex = Moss_GPUAssetLoadTexture(assets, gpu, &texture);

Moss_MeshAssetDesc mesh{};
mesh.path = "models/level.obj";
Moss_AssetHandle mesh_handle = Moss_RendererAssetLoadMesh(assets, renderer, &mesh);
Moss_Mesh* mesh_ptr = Moss_RendererAssetGetMesh(assets, mesh_handle);

Moss_AudioAssetDesc audio{};
audio.path = "audio/theme.ogg";
Moss_AssetHandle audio_handle = Moss_AudioAssetLoad(assets, &audio);
```