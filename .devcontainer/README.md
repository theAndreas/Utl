# Utl development containers

VS Code offers two configurations when running **Dev Containers: Reopen in Container**:

- **Utl (Linux)** for native Linux and remote environments. It mounts the host's SSH agent.
- **Utl (WSL2)** when VS Code runs in WSL2. It relies on VS Code's own SSH-agent forwarding and
  mounts nothing.

Open the repository from a checkout on the host, in WSL2 the Linux side of it: a folder inside
another container is lost with that container.

## What the container carries

| Tool | What for |
|---|---|
| `arduino-cli` with the core of `tools/project.sh` | the library and its examples with the real toolchain, pinned as in CI |
| `simavr` | `examples/SelfCheck` as the board runs it: `tools/self_check.sh` |
| `g++`, `gdb` (from the base image) | the tests on the host, and what `tools/project.sh` prepares |
| `cmake`, `ninja`, GoogleTest | the tests on the host: `cmake --workflow --preset default` |
| `uv` | the Python tools that `uv.lock` pins, in `.venv`: clang-format, clang-tidy, cpplint, gcovr, shellcheck, actionlint, ruff, copier |
| `clang` | the tests a second time, by another compiler: `make clang` |
| `clangd` | IntelliSense, see below |
| `jq` | the compilation database of `tools/tidy.sh` |
| `cppcheck` | a second analyser beside clang-tidy: `tools/cppcheck.sh` |
| `arduino-lint` | the rules of the Arduino Library Manager |
| `gh` | branches and pull requests from the container |

The `postCreateCommand` runs `tools/setup.sh`, which installs the core and builds what
`tools/project.sh` prepares, configures CMake and writes the compilation databases, so that a new container can build, check and complete code at once.

**Flashing is not covered.** The container builds; getting a USB serial adapter into it is another
matter, and under WSL2 it needs `usbipd-win` on the Windows side first. Build here, flash from the
host.

## IntelliSense: clangd

clangd does completion, navigation and the findings of clang-tidy, with the repository's
`.clang-tidy`, so the editor shows what CI would. cpptools stays installed for debugging, with its
IntelliSense off, so that the two do not answer twice.

clangd reads the compilation database that `.clangd` names for a path, as no single one covers
everything:

- `src/` and `test/` from CMake, against the mock of the core, in `build/host/`.
- `examples/` from `tools/tidy.sh --database-only`, in `build/tidy/default/`, with the flags the
  board is compiled with: only that one has the sketches, an `.ino` as C++ with `Arduino.h` included
  first.

Both are written when the container is created. After adding a file, run `cmake --preset default`
or `tools/tidy.sh --database-only` again. clangd inserts no includes: a sketch includes
`<Utl.h>` first, which an include put in by the editor would not know.

clangd comes from PyPI at 22.1.1, the newest there, one behind the clang-tidy 23 of CI, which stays
the measure. The package wraps the binary in a Python script whose interpreter would lie in root's
home; `/usr/local/bin/clangd` links to the binary itself.

## Volumes

The Arduino core and its toolchain, and the checkers uv fetches, are not part of the image. They live
in the named volumes `utl-arduino15` and `utl-uv-cache`, so that a rebuild does not download them
again. Two more carry a login across a rebuild: `utl-codex` at `/home/vscode/.codex` and
`utl-gh` at `/home/vscode/.config/gh`, both at mode 0700, as `gh` refuses a config directory others can
enter. Those two hold credentials: do not share, export or remove one unless the login should go.

The `Dockerfile` creates each path as the remote user first: an empty named volume takes its
ownership from the path it covers, and one that does not exist in the image gives a volume owned by
root that nothing can write into.

## SSH agent on Linux

The Linux variant mounts the host's `SSH_AUTH_SOCK` at the fixed path `/tmp/ssh-agent`, as the host
socket carries a random name. An `initializeCommand` checks that the agent runs first, so that a
missing one is reported as such rather than as a docker error about an empty `source=`:

```sh
ssh-add -l
```

GitHub's SSH host key is written into `/etc/ssh/ssh_known_hosts` by the `Dockerfile`, as published
by GitHub, so the first SSH operation does not fail with `Host key verification failed`.

## Pinned tools

`arduino-cli`, `arduino-lint`, `gh` and `uv` are pinned release downloads, each verified against its
published SHA-256 in the `Dockerfile`; `clangd` is pinned on PyPI. Bumping a version means bumping
its checksums as well. `gh auth login` has to run in a terminal, and `gh auth setup-git` afterwards
lets `git push` over HTTPS use the same token.

`gh` is pointed at `github.com` by the shared feature's `containerEnv`. A base image that sets
`GH_HOST` to a company GitHub needs `"remoteEnv": {"GH_HOST": "github.com"}` in its
`devcontainer.json` as well: the VS Code server starts with the image's value, and `remoteEnv` is
applied last.

## Shared configuration

Extensions, settings, volumes and the `postCreateCommand` live in the local feature in `shared/`,
which every configuration references with `"features": { "./../shared": {} }`. A variant can add
extensions of its own on top. Feature metadata is baked into the image, so a change to
`shared/devcontainer-feature.json` needs **Dev Containers: Rebuild Container**.

## Site-specific variants

Another directory beside `linux/` and `wsl2/` shows up as another entry in the same picker: the
place for what must not be committed, such as an internal base image through the `BASE_IMAGE`
build argument. `/.devcontainer/vector/` is listed in `.gitignore` for that.

Behind a TLS-intercepting proxy the public base image cannot build, as it does not trust the proxy's
certificate and the first download fails with `curl` exit code 60. A base image that carries the
proxy's CA, set through `BASE_IMAGE`, fixes that.
