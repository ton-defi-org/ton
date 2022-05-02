### how to Install zlib and openssl for emscripten compilation



### Install OpenSSL form source and build for wasm

`wget https://www.openssl.org/source/openssl-1.1.0h.tar.gz`
`tar xf openssl-1.1.0h.tar.gz`
`cd openssl-1.1.0h`

`emconfigure ./Configure linux-generic64 `
`sed -i 's|^CROSS_COMPILE.*$|CROSS_COMPILE=|g' Makefile`
`emmake make -j 12 build_generated libssl.a libcrypto.a`
`rm -rf $EMSCRIPTEN/system/include/openssl`
`cp -R include/openssl $EMSCRIPTEN/system/include`
`cp libcrypto.a libssl.a $EMSCRIPTEN/system/lib`
`cd ..`
`rm -rf openssl-1.1.0h*`
`emmake make -build . --target vm-exec`


### Install Zlib form source and build for wasm
`wget https://www.zlib.net/zlib-1.2.12.tar.gz`
`tar -xvzf zlib-1.2.12.tar.gz`

`cd zlib-1.2.12`
run `emconfigure ./configure`
run `emmake make`