const encoder = new TextEncoder();
const decoder = new TextDecoder();

let wasmInstance = null;

function memory() {
    if (!wasmInstance) throw new Error('WebAssembly module is not initialised');
    return wasmInstance.exports.memory;
}

function dataView() {
    return new DataView(memory().buffer);
}

function writeU32(ptr, value) {
    dataView().setUint32(ptr, value >>> 0, true);
}

function writeU64(ptr, value) {
    dataView().setBigUint64(ptr, BigInt(value), true);
}

function makeWasiImports() {
    const ENOSYS = 52;
    const wasi = {
        args_sizes_get(argcPtr, argvBufSizePtr) {
            writeU32(argcPtr, 0);
            writeU32(argvBufSizePtr, 0);
            return 0;
        },
        args_get() {
            return 0;
        },
        environ_sizes_get(countPtr, bufSizePtr) {
            writeU32(countPtr, 0);
            writeU32(bufSizePtr, 0);
            return 0;
        },
        environ_get() {
            return 0;
        },
        clock_time_get(clockId, precision, timePtr) {
            const nowNs = BigInt(Date.now()) * 1000000n;
            writeU64(timePtr, nowNs);
            return 0;
        },
        random_get(ptr, length) {
            let offset = 0;
            while (offset < length) {
                const chunk = Math.min(length - offset, 65536);
                const view = new Uint8Array(memory().buffer, ptr + offset, chunk);
                crypto.getRandomValues(view);
                offset += chunk;
            }
            return 0;
        },
        fd_write(fd, iovsPtr, iovsLen, writtenPtr) {
            const dv = dataView();
            let written = 0;
            for (let i = 0; i < iovsLen; ++i) {
                const base = iovsPtr + i * 8;
                const ptr = dv.getUint32(base, true);
                const len = dv.getUint32(base + 4, true);
                written += len;
                if ((fd === 1 || fd === 2) && len) {
                    const bytes = new Uint8Array(memory().buffer, ptr, len);
                    const text = decoder.decode(bytes);
                    if (fd === 2) console.error(text);
                }
            }
            if (writtenPtr) writeU32(writtenPtr, written);
            return 0;
        },
        fd_close() {
            return 0;
        },
        fd_fdstat_get(fd, statPtr) {
            new Uint8Array(memory().buffer, statPtr, 24).fill(0);
            return 0;
        },
        sched_yield() {
            return 0;
        },
        proc_exit(code) {
            throw new Error(`WebAssembly terminated with exit code ${code}`);
        }
    };

    return new Proxy(wasi, {
        get(target, property) {
            if (property in target) return target[property];
            return () => ENOSYS;
        }
    });
}

async function loadModule() {
    const response = await fetch('./asteroid-v23.wasm', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Unable to load asteroid-v23.wasm (${response.status})`);
    const bytes = await response.arrayBuffer();
    const wasi = makeWasiImports();
    const result = await WebAssembly.instantiate(bytes, {
        wasi_snapshot_preview1: wasi
    });
    wasmInstance = result.instance;
    if (typeof wasmInstance.exports._initialize === 'function') {
        wasmInstance.exports._initialize();
    }
    if (wasmInstance.exports.asteroid_mesh_version() !== 23) {
        throw new Error(`WebAssembly/backend version mismatch: expected 23, got ${wasmInstance.exports.asteroid_mesh_version()}`);
    }
    return wasmInstance.exports;
}

const exportsPromise = loadModule();

function writeCString(exports, text) {
    const bytes = encoder.encode(`${text}\0`);
    const ptr = exports.malloc(bytes.length);
    if (!ptr) throw new Error('WebAssembly allocation failed');
    new Uint8Array(memory().buffer, ptr, bytes.length).set(bytes);
    return ptr;
}

function readCString(ptr) {
    if (!ptr) return '';
    const heap = new Uint8Array(memory().buffer);
    let end = ptr;
    while (end < heap.length && heap[end] !== 0) ++end;
    return decoder.decode(heap.subarray(ptr, end));
}

function copyFloat32(ptr, count) {
    if (!ptr || count <= 0) return new Float32Array();
    return new Float32Array(memory().buffer, ptr, count).slice();
}

function copyUint32(ptr, count) {
    if (!ptr || count <= 0) return new Uint32Array();
    return new Uint32Array(memory().buffer, ptr, count).slice();
}

self.onmessage = async event => {
    const message = event.data;
    if (!message || message.type !== 'generate') return;

    const { requestId, query, preview } = message;

    try {
        const wasm = await exportsPromise;
        const queryPtr = writeCString(wasm, query || '');
        let result;
        try {
            result = wasm.asteroid_generate(queryPtr);
        } finally {
            wasm.free(queryPtr);
        }

        if (result !== 0) {
            const error = readCString(wasm.asteroid_error());
            throw new Error(error || `WebAssembly generator returned ${result}`);
        }

        const vertexCount = wasm.asteroid_vertex_count();
        const triangleCount = wasm.asteroid_triangle_count();
        const surfaceStride = wasm.asteroid_surface_stride();

        const vertices = copyFloat32(wasm.asteroid_vertices_ptr(), vertexCount * 3);
        const faces = copyUint32(wasm.asteroid_indices_ptr(), triangleCount * 3);
        const surface = copyFloat32(wasm.asteroid_surface_ptr(), vertexCount * surfaceStride);
        const centresFlat = copyFloat32(wasm.asteroid_centres_ptr(), 6);
        const radii = copyFloat32(wasm.asteroid_radii_ptr(), 2);
        const basesFlat = copyFloat32(wasm.asteroid_bases_ptr(), 18);

        const stats = {
            meshVersion: wasm.asteroid_mesh_version(),
            name: readCString(wasm.asteroid_name()),
            objectId: readCString(wasm.asteroid_object_id()),
            vertices: vertexCount,
            triangles: triangleCount,
            grid: [wasm.asteroid_grid_x(), wasm.asteroid_grid_y(), wasm.asteroid_grid_z()],
            centres: [
                [centresFlat[0], centresFlat[1], centresFlat[2]],
                [centresFlat[3], centresFlat[4], centresFlat[5]]
            ],
            radii: Array.from(radii),
            bases: [
                Array.from(basesFlat.subarray(0, 9)),
                Array.from(basesFlat.subarray(9, 18))
            ],
            boundaryEdges: wasm.asteroid_boundary_edges(),
            nonManifoldEdges: wasm.asteroid_non_manifold_edges(),
            windingErrors: wasm.asteroid_winding_errors(),
            components: wasm.asteroid_components(),
            watertight: wasm.asteroid_watertight() !== 0
        };

        self.postMessage({
            type: 'mesh',
            requestId,
            preview: Boolean(preview),
            vertices,
            faces,
            surface,
            surfaceStride,
            stats
        }, [vertices.buffer, faces.buffer, surface.buffer]);
    } catch (error) {
        self.postMessage({
            type: 'error',
            requestId,
            message: error instanceof Error ? error.message : String(error)
        });
    }
};
