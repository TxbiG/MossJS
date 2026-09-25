import { installMossSubsystems } from "./moss_subsystems.js";

function inputState(Module) {
    return Module.mossWebInput || {
        keys: new Set(),
        pressed: new Set(),
        released: new Set(),
        mouseButtons: new Set(),
        mousePressed: new Set(),
        mouseReleased: new Set(),
        mouseX: 0,
        mouseY: 0,
        wheelX: 0,
        wheelY: 0,
        touches: new Map()
    };
}

function installInput(Module) {
    Module.Input = {
        isKeyDown(code) {
            return inputState(Module).keys.has(code);
        },
        wasKeyPressed(code) {
            return inputState(Module).pressed.has(code);
        },
        wasKeyReleased(code) {
            return inputState(Module).released.has(code);
        },
        isMouseDown(button = 0) {
            return inputState(Module).mouseButtons.has(button);
        },
        wasMousePressed(button = 0) {
            return inputState(Module).mousePressed.has(button);
        },
        wasMouseReleased(button = 0) {
            return inputState(Module).mouseReleased.has(button);
        },
        mousePosition() {
            const s = inputState(Module);
            return { x: s.mouseX, y: s.mouseY };
        },
        mouseWheel() {
            const s = inputState(Module);
            return { x: s.wheelX, y: s.wheelY };
        },
        touches() {
            const s = inputState(Module);
            return Array.from(s.touches.values()).map(t => ({ ...t }));
        },
        gamepads() {
            return Array.from(navigator.getGamepads?.() ?? [])
                .filter(Boolean)
                .map((p) => ({
                    index: p.index,
                    id: p.id,
                    connected: p.connected,
                    mapping: p.mapping,
                    buttons: Array.from(p.buttons, b => ({
                        pressed: b.pressed,
                        value: b.value
                    })),
                    axes: Array.from(p.axes)
                }));
        },
        openGamepad(index) {
            return new Module.Gamepad(index);
        }
    };

    Module.Input.updateGamepads = () => {
        Module.updateGamepads?.();
        return Module.Input.gamepads();
    };

    return Module.Input;
}

function installDialogs(Module) {
    const pick = ({ multiple = false, accept = "" } = {}) => new Promise((resolve) => {
        const input = document.createElement("input");
        input.type = "file";
        input.multiple = !!multiple;
        if (accept) input.accept = accept;
        input.style.position = "fixed";
        input.style.left = "-10000px";
        document.body.appendChild(input);

        const done = () => {
            const files = Array.from(input.files || []);
            input.remove();
            resolve(files);
        };

        input.addEventListener("change", done, { once: true });
        input.click();
    });

    Module.Dialogs = {
        openFile(options = {}) {
            return pick({ ...options, multiple: false }).then(files => files[0] ?? null);
        },
        openFiles(options = {}) {
            return pick({ ...options, multiple: true });
        },
        saveFile(filename = "download.bin", data = new Uint8Array()) {
            const blob = data instanceof Blob ? data : new Blob([data]);
            const url = URL.createObjectURL(blob);
            const a = document.createElement("a");
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            a.remove();
            setTimeout(() => URL.revokeObjectURL(url), 0);
        }
    };
}

async function installPersistentStorage(Module, mountPoint = "/moss/user") {
    if (!Module.FS) return false;

    try {
        const parent = mountPoint.slice(0, mountPoint.lastIndexOf("/")) || "/";
        try { Module.FS.mkdir(parent); } catch (_) {}
        try { Module.FS.mkdir(mountPoint); } catch (_) {}
        const idbfs = Module.FS.filesystems?.IDBFS;
        if (!idbfs) return false;
        Module.FS.mount(idbfs, { autoPersist: true }, mountPoint);

        await new Promise((resolve, reject) => {
            Module.FS.syncfs(true, (error) => error ? reject(error) : resolve());
        });

        Module.Storage = {
            mountPoint,
            exists(path) {
                try {
                    Module.FS.stat(path);
                    return true;
                } catch (_) {
                    return false;
                }
            },
            async sync() {
                await new Promise((resolve, reject) => {
                    Module.FS.syncfs(false, (error) => error ? reject(error) : resolve());
                });
            }
        };
        return true;
    } catch (error) {
        Module.Storage = {
            mountPoint,
            sync: async () => { throw error; }
        };
        return false;
    }
}

function installBrowserUtilities(Module) {
    Module.Canvas = {
        resize(app) {
            app.resize();
        },
        fullscreen(app) {
            app.fullscreen();
        },
        pointerLock(app) {
            app.pointerLock();
        }
    };

    Module.fetchBytes = async (url, init) => {
        const response = await fetch(url, init);
        if (!response.ok)
            throw new Error(`HTTP ${response.status} while loading ${url}`);
        return new Uint8Array(await response.arrayBuffer());
    };

    Module.Camera = {
        async open(constraints = { video: true, audio: false }) {
            if (!navigator.mediaDevices?.getUserMedia)
                throw new Error("getUserMedia is not available in this browser");
            return navigator.mediaDevices.getUserMedia(constraints);
        },
        async attach(video, constraints = { video: true, audio: false }) {
            const stream = await Module.Camera.open(constraints);
            video.srcObject = stream;
            await video.play();
            return stream;
        },
        stop(stream) {
            for (const track of stream?.getTracks?.() ?? []) track.stop();
        },
        async devices() {
            if (!navigator.mediaDevices?.enumerateDevices) return [];
            return navigator.mediaDevices.enumerateDevices();
        }
    };
}

export function installMossWeb(Module) {
    installInput(Module);
    installDialogs(Module);
    installBrowserUtilities(Module);
    installMossSubsystems(Module);

    Module.start = function start(app, update, options = {}) {
        let previous = performance.now();
        let running = true;

        const frame = (now) => {
            if (!running) return;

            const maxDelta = options.maxDelta ?? 0.1;
            const dt = Math.min(Math.max((now - previous) / 1000, 0), maxDelta);
            previous = now;

            app.beginFrame();
            try {
                if (typeof update === "function") update(dt, app);
            } finally {
                app.endFrame();
            }

            if (!app.shouldClose())
                requestAnimationFrame(frame);
        };

        requestAnimationFrame(frame);
        return () => { running = false; };
    };

    Module.createApplication = (options = {}) => new Module.Application(options);
    Module.openStorage = (root = "/moss/user") => new Module.StorageHandle(root);

    return Module;
}

export async function createMoss(moduleFactory, options = {}) {
    const Module = await moduleFactory(options);
    const installed = installMossWeb(Module);

    if (options.persistentStorage !== false)
        await installPersistentStorage(installed, options.storagePath ?? "/moss/user");

    return installed;
}
