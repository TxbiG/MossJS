/*
 * Browser-side adapters for the Moss subsystems that do not map one-to-one to
 * a desktop OS API. The actual Moss renderer/GPU and Jolt physics sources are
 * still compiled into WebAssembly; these helpers expose browser-native layers
 * to JavaScript as well.
 */

const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));

function asVec3(v, fallback = [0, 0, 0]) {
    if (!v) return fallback.slice();
    return [Number(v[0] ?? v.x ?? 0), Number(v[1] ?? v.y ?? 0), Number(v[2] ?? v.z ?? 0)];
}

function add3(a, b, s = 1) {
    return [a[0] + b[0] * s, a[1] + b[1] * s, a[2] + b[2] * s];
}

function sub3(a, b) {
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
}

function dot3(a, b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

function length3(a) {
    return Math.hypot(a[0], a[1], a[2]);
}

function normalize3(a) {
    const n = length3(a);
    return n > 1e-8 ? [a[0] / n, a[1] / n, a[2] / n] : [0, 1, 0];
}

function installWebGPU(Module) {
    class WebGL2Device {
        constructor(canvasOrSelector, options = {}) {
            this.canvas = typeof canvasOrSelector === "string"
                ? document.querySelector(canvasOrSelector)
                : canvasOrSelector;
            if (!this.canvas)
                throw new Error("Moss GPU requires an HTMLCanvasElement");

            this.gl = this.canvas.getContext("webgl2", {
                alpha: options.alpha ?? false,
                antialias: options.antialias ?? true,
                depth: options.depth ?? true,
                stencil: options.stencil ?? false,
                premultipliedAlpha: options.premultipliedAlpha ?? false,
                preserveDrawingBuffer: options.preserveDrawingBuffer ?? false,
                powerPreference: options.powerPreference ?? "high-performance"
            });
            if (!this.gl)
                throw new Error("WebGL2 is unavailable");
        }

        resize(width = this.canvas.clientWidth || this.canvas.width,
               height = this.canvas.clientHeight || this.canvas.height,
               dpr = globalThis.devicePixelRatio || 1) {
            const w = Math.max(1, Math.round(width * dpr));
            const h = Math.max(1, Math.round(height * dpr));
            if (this.canvas.width !== w) this.canvas.width = w;
            if (this.canvas.height !== h) this.canvas.height = h;
            this.gl.viewport(0, 0, w, h);
            return { width: w, height: h, dpr };
        }

        capabilities() {
            const gl = this.gl;
            return {
                api: "webgl2",
                maxTextureSize: gl.getParameter(gl.MAX_TEXTURE_SIZE),
                maxTextureUnits: gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS),
                maxUniformBufferBindings: gl.getParameter(gl.MAX_UNIFORM_BUFFER_BINDINGS),
                maxVertexAttribs: gl.getParameter(gl.MAX_VERTEX_ATTRIBS),
                maxSamples: gl.getParameter(gl.MAX_SAMPLES),
                extensions: gl.getSupportedExtensions() || []
            };
        }

        createShader(type, source) {
            const gl = this.gl;
            const shader = gl.createShader(type === "fragment" || type === gl.FRAGMENT_SHADER ? gl.FRAGMENT_SHADER : gl.VERTEX_SHADER);
            gl.shaderSource(shader, source);
            gl.compileShader(shader);
            if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                const log = gl.getShaderInfoLog(shader) || "unknown shader error";
                gl.deleteShader(shader);
                throw new Error(log);
            }
            return shader;
        }

        createProgram(vertexSource, fragmentSource) {
            const gl = this.gl;
            const vs = this.createShader("vertex", vertexSource);
            const fs = this.createShader("fragment", fragmentSource);
            const program = gl.createProgram();
            gl.attachShader(program, vs);
            gl.attachShader(program, fs);
            gl.linkProgram(program);
            gl.deleteShader(vs);
            gl.deleteShader(fs);
            if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
                const log = gl.getProgramInfoLog(program) || "unknown program link error";
                gl.deleteProgram(program);
                throw new Error(log);
            }
            return program;
        }

        createBuffer(data, target = "array", usage = "static") {
            const gl = this.gl;
            const targetEnum = target === "index" ? gl.ELEMENT_ARRAY_BUFFER : gl.ARRAY_BUFFER;
            const usageEnum = {
                static: gl.STATIC_DRAW,
                dynamic: gl.DYNAMIC_DRAW,
                stream: gl.STREAM_DRAW
            }[usage] ?? gl.STATIC_DRAW;
            const buffer = gl.createBuffer();
            gl.bindBuffer(targetEnum, buffer);
            if (typeof data === "number") gl.bufferData(targetEnum, data, usageEnum);
            else gl.bufferData(targetEnum, data, usageEnum);
            gl.bindBuffer(targetEnum, null);
            return { handle: buffer, target: targetEnum };
        }

        updateBuffer(buffer, data, offset = 0) {
            const gl = this.gl;
            gl.bindBuffer(buffer.target, buffer.handle);
            gl.bufferSubData(buffer.target, offset, data);
            gl.bindBuffer(buffer.target, null);
        }

        destroyBuffer(buffer) {
            if (buffer?.handle) this.gl.deleteBuffer(buffer.handle);
        }

        createVertexArray() {
            return this.gl.createVertexArray();
        }

        deleteVertexArray(vao) {
            if (vao) this.gl.deleteVertexArray(vao);
        }

        createTexture(source, options = {}) {
            const gl = this.gl;
            const texture = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, texture);
            gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, options.flipY ? 1 : 0);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, options.minFilter ?? gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, options.magFilter ?? gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, options.wrapS ?? gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, options.wrapT ?? gl.CLAMP_TO_EDGE);

            if (typeof ImageData !== "undefined" && source instanceof ImageData) {
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, source);
            } else if (source instanceof Uint8Array || source instanceof Uint8ClampedArray) {
                const width = options.width;
                const height = options.height;
                if (!width || !height) throw new Error("Raw WebGL texture data requires width and height");
                gl.texImage2D(gl.TEXTURE_2D, 0, options.internalFormat ?? gl.RGBA8,
                    width, height, 0, options.format ?? gl.RGBA, options.type ?? gl.UNSIGNED_BYTE, source);
            } else {
                gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, source);
            }
            gl.bindTexture(gl.TEXTURE_2D, null);
            return texture;
        }

        deleteTexture(texture) {
            if (texture) this.gl.deleteTexture(texture);
        }

        createFramebuffer(texture, depthTexture = null) {
            const gl = this.gl;
            const fb = gl.createFramebuffer();
            gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
            gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
            if (depthTexture)
                gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.TEXTURE_2D, depthTexture, 0);
            const status = gl.checkFramebufferStatus(gl.FRAMEBUFFER);
            gl.bindFramebuffer(gl.FRAMEBUFFER, null);
            if (status !== gl.FRAMEBUFFER_COMPLETE) {
                gl.deleteFramebuffer(fb);
                throw new Error(`Framebuffer is incomplete: ${status}`);
            }
            return fb;
        }

        deleteFramebuffer(framebuffer) {
            if (framebuffer) this.gl.deleteFramebuffer(framebuffer);
        }

        clear(r = 0, g = 0, b = 0, a = 1, depth = 1) {
            const gl = this.gl;
            gl.clearColor(r, g, b, a);
            gl.clearDepth(depth);
            gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        }

        setDepthTest(enabled) {
            const gl = this.gl;
            if (enabled) gl.enable(gl.DEPTH_TEST); else gl.disable(gl.DEPTH_TEST);
        }

        setBlend(enabled, src = this.gl.SRC_ALPHA, dst = this.gl.ONE_MINUS_SRC_ALPHA) {
            const gl = this.gl;
            if (enabled) {
                gl.enable(gl.BLEND);
                gl.blendFunc(src, dst);
            } else {
                gl.disable(gl.BLEND);
            }
        }

        setViewport(x, y, width, height) {
            this.gl.viewport(x, y, width, height);
        }

        useProgram(program) { this.gl.useProgram(program); }
        drawArrays(mode, first, count) { this.gl.drawArrays(mode, first, count); }
        drawElements(mode, count, type = this.gl.UNSIGNED_SHORT, offset = 0) {
            this.gl.drawElements(mode, count, type, offset);
        }

        present() {
            // Browser compositing presents the WebGL backbuffer automatically.
        }
    }

    class WebGPUDevice {
        constructor(adapter, device, format) {
            this.adapter = adapter;
            this.device = device;
            this.queue = device.queue;
            this.format = format;
        }

        static async request(options = {}) {
            if (!navigator.gpu) throw new Error("WebGPU is unavailable");
            const adapter = await navigator.gpu.requestAdapter({ powerPreference: options.powerPreference ?? "high-performance" });
            if (!adapter) throw new Error("No WebGPU adapter is available");
            const device = await adapter.requestDevice(options.deviceDescriptor ?? {});
            const format = options.format ?? navigator.gpu.getPreferredCanvasFormat();
            return new WebGPUDevice(adapter, device, format);
        }

        createBuffer(dataOrSize, usage, mappedAtCreation = false) {
            const data = typeof dataOrSize === "number" ? null : dataOrSize;
            const descriptor = {
                size: data ? data.byteLength : dataOrSize,
                usage,
                mappedAtCreation
            };
            const buffer = this.device.createBuffer(descriptor);
            if (data && !mappedAtCreation) this.queue.writeBuffer(buffer, 0, data);
            if (data && mappedAtCreation) new Uint8Array(buffer.getMappedRange()).set(new Uint8Array(data.buffer ?? data, data.byteOffset ?? 0, data.byteLength));
            if (mappedAtCreation) buffer.unmap();
            return buffer;
        }

        writeBuffer(buffer, data, offset = 0) {
            this.queue.writeBuffer(buffer, offset, data);
        }

        createTexture(descriptor) { return this.device.createTexture(descriptor); }
        createSampler(descriptor = {}) { return this.device.createSampler(descriptor); }
        createShaderModule(code) { return this.device.createShaderModule({ code }); }
        createBindGroupLayout(descriptor) { return this.device.createBindGroupLayout(descriptor); }
        createBindGroup(descriptor) { return this.device.createBindGroup(descriptor); }
        createPipelineLayout(descriptor) { return this.device.createPipelineLayout(descriptor); }
        createRenderPipeline(descriptor) { return this.device.createRenderPipeline(descriptor); }
        createComputePipeline(descriptor) { return this.device.createComputePipeline(descriptor); }
        createCommandEncoder(descriptor) { return this.device.createCommandEncoder(descriptor); }
        submit(commandBuffers) { this.queue.submit(commandBuffers); }
    }

    Module.WebGL2Device = WebGL2Device;
    Module.WebGPUDevice = WebGPUDevice;
    Module.GPU = {
        createWebGL2(canvas, options) {
            return new WebGL2Device(canvas, options);
        },
        requestWebGPU(options) {
            return WebGPUDevice.request(options);
        },
        webgpuSupported() {
            return !!navigator.gpu;
        }
    };

    return Module.GPU;
}

function installWebRenderer(Module) {
    class WebRenderer {
        constructor(canvasOrApp, options = {}) {
            const canvas = canvasOrApp?.canvas instanceof Function
                ? canvasOrApp.canvas()
                : canvasOrApp;
            this.device = options.device ?? new Module.WebGL2Device(canvas, options);
            this.gl = this.device.gl;
            this.canvas = this.device.canvas;
            this.autoResize = options.autoResize !== false;
            this.clearColor = options.clearColor ?? [0, 0, 0, 1];
        }

        beginFrame() {
            if (this.autoResize) this.device.resize();
            this.gl.bindFramebuffer(this.gl.FRAMEBUFFER, null);
            this.device.setViewport(0, 0, this.canvas.width, this.canvas.height);
            const c = this.clearColor;
            this.gl.clearColor(c[0], c[1], c[2], c[3]);
            this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);
        }

        endFrame() { this.device.present(); }
        clear(color = this.clearColor) {
            this.clearColor = color.slice(0, 4);
            this.device.clear(...this.clearColor);
        }
        resize() { return this.device.resize(); }
        capabilities() { return this.device.capabilities(); }
        createShader(type, source) { return this.device.createShader(type, source); }
        createProgram(vs, fs) { return this.device.createProgram(vs, fs); }
        createBuffer(data, target, usage) { return this.device.createBuffer(data, target, usage); }
        createVertexArray() { return this.device.createVertexArray(); }
        createTexture(source, options) { return this.device.createTexture(source, options); }
    }

    Module.WebRenderer = WebRenderer;
    Module.Renderer = {
        create(canvasOrApp, options = {}) { return new WebRenderer(canvasOrApp, options); }
    };
    return Module.Renderer;
}

function installWebAudio(Module) {
    class AudioSystem {
        constructor(options = {}) {
            const Ctor = globalThis.AudioContext || globalThis.webkitAudioContext;
            this.context = Ctor ? new Ctor(options.contextOptions) : null;
            if (!this.context) throw new Error("Web Audio API is unavailable");
            this.master = this.context.createGain();
            this.master.gain.value = options.masterVolume ?? 1;
            this.master.connect(this.context.destination);
            this.buffers = new Map();
            this.sources = new Map();
            this.nextHandle = 1;
            this.microphoneStream = null;
            this.microphoneSource = null;
        }

        async resume() {
            if (this.context.state !== "running") await this.context.resume();
        }

        async suspend() {
            if (this.context.state === "running") await this.context.suspend();
        }

        async load(url, cacheKey = url) {
            if (this.buffers.has(cacheKey)) return this.buffers.get(cacheKey);
            const response = await fetch(url);
            if (!response.ok) throw new Error(`Audio HTTP ${response.status}: ${url}`);
            const bytes = await response.arrayBuffer();
            const buffer = await this.context.decodeAudioData(bytes);
            this.buffers.set(cacheKey, buffer);
            return buffer;
        }

        playBuffer(buffer, options = {}) {
            const source = this.context.createBufferSource();
            source.buffer = buffer;
            source.loop = !!options.loop;
            source.playbackRate.value = options.pitch ?? 1;

            const gain = this.context.createGain();
            gain.gain.value = options.volume ?? 1;
            source.connect(gain);

            if (options.position) {
                const panner = this.context.createPanner();
                panner.panningModel = options.panningModel ?? "HRTF";
                panner.distanceModel = options.distanceModel ?? "inverse";
                panner.refDistance = options.refDistance ?? 1;
                panner.positionX.value = options.position[0] ?? 0;
                panner.positionY.value = options.position[1] ?? 0;
                panner.positionZ.value = options.position[2] ?? 0;
                gain.connect(panner);
                panner.connect(this.master);
            } else {
                gain.connect(this.master);
            }

            const handle = this.nextHandle++;
            this.sources.set(handle, { source, gain });
            source.addEventListener("ended", () => {
                if (!source.loop) this.sources.delete(handle);
            });
            source.start(0, options.offset ?? 0);
            return handle;
        }

        async play(url, options = {}) {
            await this.resume();
            const buffer = await this.load(url, options.cacheKey);
            return this.playBuffer(buffer, options);
        }

        stop(handle) {
            const item = this.sources.get(handle);
            if (!item) return false;
            try { item.source.stop(); } catch (_) {}
            this.sources.delete(handle);
            return true;
        }

        setVolume(handle, volume) {
            const item = this.sources.get(handle);
            if (!item) return false;
            item.gain.gain.value = clamp(volume, 0, 1);
            return true;
        }

        setMasterVolume(volume) {
            this.master.gain.value = clamp(volume, 0, 1);
        }

        setListener(position, forward = [0, 0, -1], up = [0, 1, 0]) {
            const l = this.context.listener;
            const p = asVec3(position);
            const f = normalize3(asVec3(forward, [0, 0, -1]));
            const u = normalize3(asVec3(up, [0, 1, 0]));
            if ("positionX" in l) {
                l.positionX.value = p[0];
                l.positionY.value = p[1];
                l.positionZ.value = p[2];
                l.forwardX.value = f[0];
                l.forwardY.value = f[1];
                l.forwardZ.value = f[2];
                l.upX.value = u[0];
                l.upY.value = u[1];
                l.upZ.value = u[2];
            } else {
                l.setPosition(p[0], p[1], p[2]);
                l.setOrientation(f[0], f[1], f[2], u[0], u[1], u[2]);
            }
        }

        async startMicrophone(options = {}) {
            if (!navigator.mediaDevices?.getUserMedia)
                throw new Error("Microphone capture is unavailable");
            this.microphoneStream = await navigator.mediaDevices.getUserMedia({
                audio: options.audio ?? true,
                video: false
            });
            this.microphoneSource = this.context.createMediaStreamSource(this.microphoneStream);
            const destination = options.destination ?? this.master;
            this.microphoneSource.connect(destination);
            return this.microphoneStream;
        }

        stopMicrophone() {
            this.microphoneSource?.disconnect();
            for (const track of this.microphoneStream?.getTracks?.() ?? []) track.stop();
            this.microphoneSource = null;
            this.microphoneStream = null;
        }
    }

    Module.Audio = AudioSystem;
    Module.createAudio = (options) => new AudioSystem(options);
    return AudioSystem;
}

function installWebGUI(Module) {
    class GUI {
        constructor(options = {}) {
            this.root = options.root instanceof HTMLElement
                ? options.root
                : document.createElement("div");
            if (!this.root.parentElement) document.body.appendChild(this.root);
            this.root.classList.add("moss-gui-root");
            Object.assign(this.root.style, {
                position: "fixed",
                inset: "0",
                pointerEvents: "none",
                zIndex: String(options.zIndex ?? 1000),
                fontFamily: options.fontFamily ?? "system-ui, sans-serif"
            });
            this.elements = new Set();
        }

        _add(tag, options = {}) {
            const element = document.createElement(tag);
            Object.assign(element.style, options.style ?? {});
            element.textContent = options.text ?? "";
            if (options.className) element.className = options.className;
            if (options.attributes)
                for (const [k, v] of Object.entries(options.attributes)) element.setAttribute(k, String(v));
            (options.parent ?? this.root).appendChild(element);
            element.style.pointerEvents = options.pointerEvents ?? "auto";
            this.elements.add(element);
            return element;
        }

        panel(options = {}) {
            return this._add("div", {
                ...options,
                style: {
                    boxSizing: "border-box",
                    padding: "12px",
                    background: "rgba(0,0,0,.72)",
                    color: "white",
                    borderRadius: "8px",
                    ...options.style
                }
            });
        }

        text(text, options = {}) {
            return this._add("div", { ...options, text });
        }

        button(text, callback, options = {}) {
            const element = this._add("button", { ...options, text });
            if (typeof callback === "function") element.addEventListener("click", callback);
            return element;
        }

        checkbox(label, checked = false, callback, options = {}) {
            const wrapper = this._add("label", {
                ...options,
                style: { display: "flex", alignItems: "center", gap: "8px", ...options.style }
            });
            const input = document.createElement("input");
            input.type = "checkbox";
            input.checked = !!checked;
            wrapper.appendChild(input);
            wrapper.appendChild(document.createTextNode(label));
            if (callback) input.addEventListener("change", () => callback(input.checked, input));
            return input;
        }

        slider(min, max, value, callback, options = {}) {
            const input = this._add("input", { ...options, attributes: { type: "range", min, max, value, step: options.step ?? 0.01 } });
            if (callback) input.addEventListener("input", () => callback(Number(input.value), input));
            return input;
        }

        remove(element) {
            if (!this.elements.has(element)) return false;
            element.remove();
            this.elements.delete(element);
            return true;
        }

        clear() {
            for (const element of this.elements) element.remove();
            this.elements.clear();
        }

        destroy() {
            this.clear();
            this.root.remove();
        }
    }

    Module.GUI = GUI;
    Module.createGUI = (options) => new GUI(options);
    return GUI;
}

function installWebXR(Module) {
    class XRSystem {
        constructor() {
            this.session = null;
            this.referenceSpace = null;
            this.frameHandle = null;
            this.frameCallback = null;
            this.lastFrame = null;
        }

        async isSupported(mode = "immersive-vr") {
            return !!navigator.xr && await navigator.xr.isSessionSupported(mode);
        }

        async requestSession(mode = "immersive-vr", options = {}) {
            if (!navigator.xr) throw new Error("WebXR is unavailable");
            if (options.device?.gl?.makeXRCompatible)
                await options.device.gl.makeXRCompatible();
            const sessionOptions = {
                optionalFeatures: options.optionalFeatures ?? ["local-floor", "bounded-floor", "hand-tracking"]
            };
            this.session = await navigator.xr.requestSession(mode, sessionOptions);
            if (options.device?.gl && typeof XRWebGLLayer !== "undefined") {
                const baseLayer = new XRWebGLLayer(this.session, options.device.gl);
                this.session.updateRenderState({ baseLayer });
            }
            this.referenceSpace = await this.session.requestReferenceSpace(options.referenceSpaceType ?? "local-floor");
            this.session.addEventListener("end", () => {
                this.cancelFrameLoop();
                this.session = null;
                this.referenceSpace = null;
            }, { once: true });
            return this.session;
        }

        async endSession() {
            if (this.session) await this.session.end();
        }

        startFrameLoop(callback) {
            if (!this.session) throw new Error("No active WebXR session");
            this.frameCallback = callback;
            const onFrame = (time, frame) => {
                if (!this.session || !this.frameCallback) return;
                const pose = frame.getViewerPose(this.referenceSpace);
                const views = pose ? Array.from(pose.views).map(view => ({
                    eye: view.eye,
                    projectionMatrix: Array.from(view.projectionMatrix),
                    transformMatrix: Array.from(view.transform.matrix),
                    viewport: this.session.renderState.baseLayer?.getViewport(view) ?? null
                })) : [];
                const inputs = Array.from(this.session.inputSources ?? []).map(input => ({
                    handedness: input.handedness,
                    targetRayMode: input.targetRayMode,
                    gripSpace: !!input.gripSpace,
                    gamepad: input.gamepad ? {
                        buttons: Array.from(input.gamepad.buttons, b => ({ pressed: b.pressed, value: b.value })),
                        axes: Array.from(input.gamepad.axes)
                    } : null
                }));
                const info = { time, frame, pose, views, inputs };
                this.lastFrame = info;
                this.frameCallback(info);
                this.frameHandle = this.session.requestAnimationFrame(onFrame);
            };
            this.frameHandle = this.session.requestAnimationFrame(onFrame);
            return () => this.cancelFrameLoop();
        }

        cancelFrameLoop() {
            this.frameCallback = null;
            this.frameHandle = null;
        }

        async capabilities() {
            return {
                webxr: !!navigator.xr,
                immersiveVR: !!navigator.xr && await navigator.xr.isSessionSupported("immersive-vr").catch(() => false),
                immersiveAR: !!navigator.xr && await navigator.xr.isSessionSupported("immersive-ar").catch(() => false)
            };
        }
    }

    Module.XR = new XRSystem();
    Module.XRSystem = XRSystem;
    Module.createXR = () => new XRSystem();
    return Module.XR;
}

function installWebPhysics(Module) {
    class PhysicsWorld {
        constructor(options = {}) {
            this.gravity = asVec3(options.gravity, [0, -9.81, 0]);
            this.bodies = new Map();
            this.nextId = 1;
            this.ground = options.ground !== false;
        }

        setGravity(gravity) { this.gravity = asVec3(gravity); }
        getGravity() { return this.gravity.slice(); }

        _create(type, options = {}) {
            const mass = options.static || options.kinematic ? 0 : Math.max(0.0001, Number(options.mass ?? 1));
            const body = {
                id: this.nextId++,
                type,
                position: asVec3(options.position),
                velocity: asVec3(options.velocity),
                force: [0, 0, 0],
                size: asVec3(options.size, [1, 1, 1]),
                radius: Number(options.radius ?? 0.5),
                mass,
                inverseMass: mass > 0 ? 1 / mass : 0,
                restitution: clamp(Number(options.restitution ?? 0.0), 0, 1),
                friction: clamp(Number(options.friction ?? 0.5), 0, 1),
                static: !!options.static,
                kinematic: !!options.kinematic,
                enabled: true
            };
            this.bodies.set(body.id, body);
            return body.id;
        }

        createBox(options = {}) { return this._create("box", options); }
        createSphere(options = {}) { return this._create("sphere", options); }

        destroyBody(id) { return this.bodies.delete(id); }
        body(id) {
            const b = this.bodies.get(id);
            if (!b) return null;
            return {
                id: b.id,
                type: b.type,
                position: b.position.slice(),
                velocity: b.velocity.slice(),
                size: b.size.slice(),
                radius: b.radius,
                mass: b.mass,
                static: b.static,
                kinematic: b.kinematic
            };
        }

        applyForce(id, force) {
            const b = this.bodies.get(id);
            if (!b || b.inverseMass === 0) return false;
            b.force = add3(b.force, asVec3(force));
            return true;
        }

        applyImpulse(id, impulse) {
            const b = this.bodies.get(id);
            if (!b || b.inverseMass === 0) return false;
            b.velocity = add3(b.velocity, asVec3(impulse), b.inverseMass);
            return true;
        }

        _radius(body) {
            return body.type === "sphere" ? body.radius : Math.max(body.size[0], body.size[1], body.size[2]) * 0.5;
        }

        _resolveSphereSphere(a, b) {
            const d = sub3(b.position, a.position);
            const dist = length3(d);
            const radius = this._radius(a) + this._radius(b);
            if (dist >= radius) return;
            const n = dist > 1e-6 ? [d[0] / dist, d[1] / dist, d[2] / dist] : [0, 1, 0];
            const penetration = radius - dist;
            const invTotal = a.inverseMass + b.inverseMass;
            if (invTotal <= 0) return;
            const moveA = a.inverseMass / invTotal;
            const moveB = b.inverseMass / invTotal;
            a.position = add3(a.position, n, -penetration * moveA);
            b.position = add3(b.position, n, penetration * moveB);
            const rv = sub3(b.velocity, a.velocity);
            const velAlongNormal = dot3(rv, n);
            if (velAlongNormal > 0) return;
            const e = Math.min(a.restitution, b.restitution);
            const j = -(1 + e) * velAlongNormal / invTotal;
            const impulse = [n[0] * j, n[1] * j, n[2] * j];
            a.velocity = add3(a.velocity, impulse, -a.inverseMass);
            b.velocity = add3(b.velocity, impulse, b.inverseMass);
        }

        _resolveBoxGround(body) {
            if (body.type !== "box") return;
            const halfY = body.size[1] * 0.5;
            const bottom = body.position[1] - halfY;
            if (bottom >= 0) return;
            body.position[1] -= bottom;
            if (body.velocity[1] < 0) body.velocity[1] *= -body.restitution;
            body.velocity[0] *= Math.max(0, 1 - body.friction * 0.1);
            body.velocity[2] *= Math.max(0, 1 - body.friction * 0.1);
        }

        _resolveSphereGround(body) {
            const bottom = body.position[1] - body.radius;
            if (bottom >= 0) return;
            body.position[1] -= bottom;
            if (body.velocity[1] < 0) body.velocity[1] *= -body.restitution;
            body.velocity[0] *= Math.max(0, 1 - body.friction * 0.1);
            body.velocity[2] *= Math.max(0, 1 - body.friction * 0.1);
        }

        step(dt) {
            const delta = clamp(Number(dt), 0, 0.05);
            for (const body of this.bodies.values()) {
                if (!body.enabled || body.inverseMass === 0 || body.kinematic) continue;
                body.velocity = add3(body.velocity, this.gravity, delta);
                body.velocity = add3(body.velocity, body.force, body.inverseMass * delta);
                body.position = add3(body.position, body.velocity, delta);
                body.force = [0, 0, 0];
                if (this.ground) {
                    body.type === "sphere" ? this._resolveSphereGround(body) : this._resolveBoxGround(body);
                }
            }

            const dynamic = Array.from(this.bodies.values()).filter(b => b.enabled && b.inverseMass > 0);
            for (let i = 0; i < dynamic.length; ++i)
                for (let j = i + 1; j < dynamic.length; ++j)
                    if (dynamic[i].type === "sphere" && dynamic[j].type === "sphere")
                        this._resolveSphereSphere(dynamic[i], dynamic[j]);
        }
    }

    Module.Physics = PhysicsWorld;
    Module.createPhysicsWorld = (options) => new PhysicsWorld(options);
    return PhysicsWorld;
}

export function installMossSubsystems(Module) {
    installWebGPU(Module);
    installWebRenderer(Module);
    installWebAudio(Module);
    installWebGUI(Module);
    installWebXR(Module);
    installWebPhysics(Module);

    Module.Native = {
        createRenderer(app) {
            if (!Module.NativeRenderer) throw new Error("NativeRenderer binding was not compiled");
            return new Module.NativeRenderer(app);
        }
    };

    Module.Subsystems = {
        audio: true,
        physics: true,
        xr: true,
        gpu: true,
        gui: true,
        renderer: true,
        navigation: false
    };

    if (typeof Module.subsystemCapabilities === "function") {
        try { Object.assign(Module.Subsystems, Module.subsystemCapabilities()); } catch (_) {}
    }

    return Module.Subsystems;
}
