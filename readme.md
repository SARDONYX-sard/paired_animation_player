# PairedAnimationPlayer

## Requirements(User)

- AddressLibrary
- SKSE Menu Framework

## Requirements(Dev)

- Rust: >= v1.91
- MSVC
- xmake: >= 3.0.6

## Build

```shell
git submodule update --init --recursive --depth=1 && xmake PairedAnimationPlayer
```

- And then install(`./build/artifact/SKSE/Data/<PLUGIN_NAME>.dll`)

```shell
xmake install -o ./build/artifact PairedAnimationPlayer
```

## Language server(For `clangd`)

```shell
xmake project -k compile_commands --lsp=clangd --outputdir=build
```

## Change build mode

```shell
xmake f -m release
```

## Format

- NOTE: Need LLVM(clang-format.exe path)

```shell
xmake format
```

## License

MIT OR Apache2.0
