export interface MossApplicationOptions {
    title?: string;
    width?: number;
    height?: number;
    canvas?: HTMLCanvasElement;
}

export interface MossGamepadSnapshot {
    index: number;
    id: string;
    connected: boolean;
    mapping: string;
    buttons: Array<{ pressed: boolean; value: number }>;
    axes: number[];
}

export interface MossInput {
    isKeyDown(code: string): boolean;
    wasKeyPressed(code: string): boolean;
    wasKeyReleased(code: string): boolean;
    isMouseDown(button?: number): boolean;
    wasMousePressed(button?: number): boolean;
    wasMouseReleased(button?: number): boolean;
    mousePosition(): { x: number; y: number };
    mouseWheel(): { x: number; y: number };
    touches(): Array<{ id: number; x: number; y: number; pressure: number }>;
    gamepads(): MossGamepadSnapshot[];
    updateGamepads(): MossGamepadSnapshot[];
    openGamepad(index: number): any;
}

export interface MossDialogs {
    openFile(options?: { accept?: string }): Promise<File | null>;
    openFiles(options?: { accept?: string }): Promise<File[]>;
    saveFile(filename?: string, data?: BlobPart | BlobPart[]): void;
}

export interface MossApplication {
    attachCanvas(canvas: HTMLCanvasElement): void;
    canvas(): HTMLCanvasElement;
    resize(): void;
    beginFrame(): void;
    endFrame(): void;
    fullscreen(): void;
    exitFullscreen(): void;
    pointerLock(): void;
    exitPointerLock(): void;
    isFullscreen(): boolean;
    isPointerLocked(): boolean;
    isFocused(): boolean;
    shouldClose(): boolean;
    close(): void;
    setTitle(title: string): void;
    setCursor(cursor: string): void;
    focus(): void;
    on(event: string, callback: (...args: any[]) => void): void;
    off(event: string): void;
    width(): number;
    height(): number;
    devicePixelRatio(): number;
    nativeWindow?: unknown;
}

export interface MossWebGLDevice {
    canvas: HTMLCanvasElement;
    gl: WebGL2RenderingContext;
    resize(width?: number, height?: number, dpr?: number): { width: number; height: number; dpr: number };
    capabilities(): Record<string, any>;
    createShader(type: "vertex" | "fragment", source: string): WebGLShader;
    createProgram(vertexSource: string, fragmentSource: string): WebGLProgram;
    createBuffer(data: ArrayBufferView | number, target?: "array" | "index", usage?: "static" | "dynamic" | "stream"): { handle: WebGLBuffer; target: number };
    updateBuffer(buffer: { handle: WebGLBuffer; target: number }, data: ArrayBufferView, offset?: number): void;
    destroyBuffer(buffer: { handle: WebGLBuffer; target: number }): void;
    createVertexArray(): WebGLVertexArrayObject | null;
    deleteVertexArray(vao: WebGLVertexArrayObject): void;
    createTexture(source: TexImageSource | ImageData | Uint8Array | Uint8ClampedArray, options?: Record<string, any>): WebGLTexture | null;
    deleteTexture(texture: WebGLTexture): void;
    createFramebuffer(texture: WebGLTexture, depthTexture?: WebGLTexture | null): WebGLFramebuffer;
    deleteFramebuffer(framebuffer: WebGLFramebuffer): void;
    clear(r?: number, g?: number, b?: number, a?: number, depth?: number): void;
    setDepthTest(enabled: boolean): void;
    setBlend(enabled: boolean, src?: number, dst?: number): void;
    setViewport(x: number, y: number, width: number, height: number): void;
    useProgram(program: WebGLProgram | null): void;
    drawArrays(mode: number, first: number, count: number): void;
    drawElements(mode: number, count: number, type?: number, offset?: number): void;
    present(): void;
}

export interface MossWebRenderer {
    device: MossWebGLDevice;
    gl: WebGL2RenderingContext;
    canvas: HTMLCanvasElement;
    beginFrame(): void;
    endFrame(): void;
    clear(color?: [number, number, number, number]): void;
    resize(): any;
    capabilities(): Record<string, any>;
    createShader(type: "vertex" | "fragment", source: string): WebGLShader;
    createProgram(vertexSource: string, fragmentSource: string): WebGLProgram;
    createBuffer(data: ArrayBufferView | number, target?: "array" | "index", usage?: "static" | "dynamic" | "stream"): { handle: WebGLBuffer; target: number };
    createVertexArray(): WebGLVertexArrayObject | null;
    createTexture(source: TexImageSource | ImageData | Uint8Array | Uint8ClampedArray, options?: Record<string, any>): WebGLTexture | null;
}

export interface MossAudioSystem {
    context: AudioContext;
    resume(): Promise<void>;
    suspend(): Promise<void>;
    load(url: string, cacheKey?: string): Promise<AudioBuffer>;
    play(url: string, options?: Record<string, any>): Promise<number>;
    playBuffer(buffer: AudioBuffer, options?: Record<string, any>): number;
    stop(handle: number): boolean;
    setVolume(handle: number, volume: number): boolean;
    setMasterVolume(volume: number): void;
    setListener(position: number[] | { x: number; y: number; z: number }, forward?: number[], up?: number[]): void;
    startMicrophone(options?: Record<string, any>): Promise<MediaStream>;
    stopMicrophone(): void;
}

export interface MossGUI {
    root: HTMLElement;
    panel(options?: Record<string, any>): HTMLElement;
    text(text: string, options?: Record<string, any>): HTMLElement;
    button(text: string, callback?: (event: MouseEvent) => void, options?: Record<string, any>): HTMLButtonElement;
    checkbox(label: string, checked?: boolean, callback?: (checked: boolean, input: HTMLInputElement) => void, options?: Record<string, any>): HTMLInputElement;
    slider(min: number, max: number, value: number, callback?: (value: number, input: HTMLInputElement) => void, options?: Record<string, any>): HTMLInputElement;
    remove(element: HTMLElement): boolean;
    clear(): void;
    destroy(): void;
}

export interface MossXRSystem {
    session: XRSession | null;
    referenceSpace: XRReferenceSpace | null;
    isSupported(mode?: XRSessionMode): Promise<boolean>;
    requestSession(mode?: XRSessionMode, options?: { optionalFeatures?: string[]; referenceSpaceType?: XRReferenceSpaceType }): Promise<XRSession>;
    endSession(): Promise<void>;
    startFrameLoop(callback: (info: any) => void): () => void;
    cancelFrameLoop(): void;
    capabilities(): Promise<Record<string, any>>;
}

export interface MossWebGPUDevice {
    adapter: GPUAdapter;
    device: GPUDevice;
    queue: GPUQueue;
    format: GPUTextureFormat;
    createBuffer(dataOrSize: ArrayBufferView | number, usage: GPUBufferUsageFlags, mappedAtCreation?: boolean): GPUBuffer;
    writeBuffer(buffer: GPUBuffer, data: ArrayBufferView, offset?: number): void;
    createTexture(descriptor: GPUTextureDescriptor): GPUTexture;
    createSampler(descriptor?: GPUSamplerDescriptor): GPUSampler;
    createShaderModule(code: string): GPUShaderModule;
    createBindGroupLayout(descriptor: GPUBindGroupLayoutDescriptor): GPUBindGroupLayout;
    createBindGroup(descriptor: GPUBindGroupDescriptor): GPUBindGroup;
    createPipelineLayout(descriptor: GPUPipelineLayoutDescriptor): GPUPipelineLayout;
    createRenderPipeline(descriptor: GPURenderPipelineDescriptor): GPURenderPipeline;
    createComputePipeline(descriptor: GPUComputePipelineDescriptor): GPUComputePipeline;
    createCommandEncoder(descriptor?: GPUCommandEncoderDescriptor): GPUCommandEncoder;
    submit(commandBuffers: GPUCommandBuffer[]): void;
}

export interface MossPhysicsBody {
    id: number;
    type: "box" | "sphere";
    position: number[];
    velocity: number[];
    size: number[];
    radius: number;
    mass: number;
    static: boolean;
    kinematic: boolean;
}

export interface MossPhysicsWorld {
    setGravity(gravity: number[]): void;
    getGravity(): number[];
    createBox(options?: Record<string, any>): number;
    createSphere(options?: Record<string, any>): number;
    destroyBody(id: number): boolean;
    body(id: number): MossPhysicsBody | null;
    applyForce(id: number, force: number[]): boolean;
    applyImpulse(id: number, impulse: number[]): boolean;
    step(dt: number): void;
}

export interface MossPersistentStorage {
    mountPoint: string;
    exists(path: string): boolean;
    sync(): Promise<void>;
}

export interface MossStorageHandle {
    ready(): boolean;
    close(): void;
    writeFile(path: string, data: Uint8Array): boolean;
    readFile(path: string): Uint8Array | null;
}

export interface MossSubsystems {
    audio: boolean;
    physics: boolean;
    xr: boolean;
    gpu: boolean;
    gui: boolean;
    renderer: boolean;
    navigation: false;
}

export interface MossModule {
    Application: new (options: MossApplicationOptions) => MossApplication;
    NativeRenderer?: new (app: MossApplication) => { valid(): boolean; beginFrame(): void; endFrame(): void; destroy(): void };
    Input: MossInput;
    Dialogs: MossDialogs;
    GPU: {
        createWebGL2(canvas: HTMLCanvasElement, options?: Record<string, any>): MossWebGLDevice;
        requestWebGPU(options?: Record<string, any>): Promise<MossWebGPUDevice>;
        webgpuSupported(): boolean;
    };
    WebGL2Device: new (canvas: HTMLCanvasElement, options?: Record<string, any>) => MossWebGLDevice;
    Renderer: { create(canvasOrApp: HTMLCanvasElement | MossApplication, options?: Record<string, any>): MossWebRenderer };
    WebRenderer: new (canvasOrApp: HTMLCanvasElement | MossApplication, options?: Record<string, any>) => MossWebRenderer;
    Audio: new (options?: Record<string, any>) => MossAudioSystem;
    createAudio(options?: Record<string, any>): MossAudioSystem;
    GUI: new (options?: Record<string, any>) => MossGUI;
    createGUI(options?: Record<string, any>): MossGUI;
    XR: MossXRSystem;
    XRSystem: new () => MossXRSystem;
    createXR(): MossXRSystem;
    Physics: new (options?: Record<string, any>) => MossPhysicsWorld;
    createPhysicsWorld(options?: Record<string, any>): MossPhysicsWorld;
    Subsystems: MossSubsystems;
    Native: { createRenderer(app: MossApplication): any };
    createApplication(options?: MossApplicationOptions): MossApplication;
    openStorage(root?: string): MossStorageHandle;
    start(app: MossApplication, update: (dt: number, app: MossApplication) => void, options?: { maxDelta?: number }): () => void;
    fetchBytes(url: string, init?: RequestInit): Promise<Uint8Array>;
    getTicks(): bigint;
    getSeconds(ticks: bigint): number;
    getMilliseconds(ticks: bigint): number;
    getDeltaMilliseconds(): number;
    getWindowWidth(): number;
    getWindowHeight(): number;
    getDevicePixelRatio(): number;
    isKeyPressed(key: number): boolean;
    isKeyReleased(key: number): boolean;
    isKeyJustPressed(key: number): boolean;
    isKeyJustReleased(key: number): boolean;
    isMousePressed(button: number): boolean;
    isMouseReleased(button: number): boolean;
    isMouseJustPressed(button: number): boolean;
    isMouseJustReleased(button: number): boolean;
    gamepadCount(): number;
    updateGamepads(): void;
    cpuCores(): number;
    cpuCacheLineSize(): number;
    systemRAM(): number;
    openURL(url: string): boolean;
    locale(): { country: string; language: string };
    Storage?: MossPersistentStorage;
    Camera: {
        open(constraints?: MediaStreamConstraints): Promise<MediaStream>;
        attach(video: HTMLVideoElement, constraints?: MediaStreamConstraints): Promise<MediaStream>;
        stop(stream: MediaStream): void;
        devices(): Promise<MediaDeviceInfo[]>;
    };
}

export function installMossSubsystems(module: MossModule): MossSubsystems;
export function installMossWeb(module: MossModule): MossModule;
export function createMoss(moduleFactory: (options?: object) => Promise<MossModule>, options?: object): Promise<MossModule>;
