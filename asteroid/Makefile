CXX := g++
WASM_CXX ?= /usr/bin/clang++
WEB_DIR := /var/www/html

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O3

WASM_EXPORTS := \
	-Wl,--export=malloc \
	-Wl,--export=free \
	-Wl,--export=asteroid_generate \
	-Wl,--export=asteroid_mesh_version \
	-Wl,--export=asteroid_vertex_count \
	-Wl,--export=asteroid_triangle_count \
	-Wl,--export=asteroid_vertices_ptr \
	-Wl,--export=asteroid_indices_ptr \
	-Wl,--export=asteroid_surface_ptr \
	-Wl,--export=asteroid_surface_stride \
	-Wl,--export=asteroid_centres_ptr \
	-Wl,--export=asteroid_radii_ptr \
	-Wl,--export=asteroid_bases_ptr \
	-Wl,--export=asteroid_grid_x \
	-Wl,--export=asteroid_grid_y \
	-Wl,--export=asteroid_grid_z \
	-Wl,--export=asteroid_boundary_edges \
	-Wl,--export=asteroid_non_manifold_edges \
	-Wl,--export=asteroid_winding_errors \
	-Wl,--export=asteroid_components \
	-Wl,--export=asteroid_watertight \
	-Wl,--export=asteroid_name \
	-Wl,--export=asteroid_object_id \
	-Wl,--export=asteroid_error

WASMFLAGS := --target=wasm32-wasi -std=c++17 -O3 \
	-stdlib=libc++ \
	-nostdinc++ \
	-isystem /usr/include/wasm32-wasi/c++/v1 \
	-isystem /usr/include/wasm32-wasi \
	-L/usr/lib/wasm32-wasi \
	-fno-exceptions -fno-rtti -fno-stack-protector \
	-DASTEROID_WASM -DASTEROID_NO_MAIN \
	-mexec-model=reactor \
	-Wl,--export-memory \
	-Wl,--initial-memory=67108864 \
	-Wl,--max-memory=2147483648 \
	-Wl,--stack-first \
	-Wl,-z,stack-size=8388608 \
	$(WASM_EXPORTS)

.PHONY: all toolchain wasm native deploy clean

all: wasm

toolchain:
	@if ! command -v $(WASM_CXX) >/dev/null 2>&1 || \
	   ! test -f /usr/lib/wasm32-wasi/libc++.a || \
	   ! test -f /usr/lib/wasm32-wasi/libc++abi.a || \
	   ! test -d /usr/include/wasm32-wasi/c++/v1; then \
		echo "Installing Debian WebAssembly compiler support..."; \
		sudo apt-get update; \
		sudo apt-get install -y --no-install-recommends \
			clang lld wasi-libc libc++-dev-wasm32 libclang-rt-dev-wasm32; \
	fi
	@command -v $(WASM_CXX) >/dev/null 2>&1
	@test -f /usr/lib/wasm32-wasi/libc++.a
	@test -f /usr/lib/wasm32-wasi/libc++abi.a
	@test -d /usr/include/wasm32-wasi/c++/v1

wasm: toolchain asteroid-v24.wasm

asteroid-v24.wasm: asteroid.cpp
	$(WASM_CXX) $(WASMFLAGS) asteroid.cpp -o asteroid-v24.wasm
	@test -s asteroid-v24.wasm

native: asteroid-native

asteroid-native: asteroid.cpp
	$(CXX) $(CXXFLAGS) asteroid.cpp -o asteroid-native

deploy: wasm
	sudo install -m 644 index.html $(WEB_DIR)/index.html
	sudo install -m 644 mesh-worker-v24.js $(WEB_DIR)/mesh-worker-v24.js
	sudo install -m 644 asteroid-v24.wasm $(WEB_DIR)/asteroid-v24.wasm
	sudo rm -f $(WEB_DIR)/mesh-worker-v23.js $(WEB_DIR)/asteroid-v23.wasm $(WEB_DIR)/mesh-worker-v22.js $(WEB_DIR)/asteroid-v22.wasm $(WEB_DIR)/mesh-worker-v21.js $(WEB_DIR)/asteroid-v21.js $(WEB_DIR)/asteroid-v21.wasm
	sudo rm -f /usr/lib/cgi-bin/asteroid
	sudo systemctl restart lighttpd
	@echo "Deployed v24. Mesh computation runs in each visitor's browser."

clean:
	rm -f asteroid-native asteroid-v24.wasm
