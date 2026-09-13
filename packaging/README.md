# Packaging

`PKGBUILD` builds the AUR package. To publish a release:

1. Tag and push the release, so the `source` tarball URL resolves:

   ```sh
   git tag -a v1.0.0 -m 'v1.0.0'
   git push origin v1.0.0
   ```

2. Replace the `SKIP` checksum with the real one:

   ```sh
   updpkgsums
   ```

3. Build, test and lint:

   ```sh
   makepkg -si
   namcap PKGBUILD
   namcap cdeez-1.0.0-1-x86_64.pkg.tar.zst
   ```

4. Regenerate `.SRCINFO` and push to the AUR:

   ```sh
   makepkg --printsrcinfo > .SRCINFO
   git -C aur commit -am 'cdeez 1.0.0' && git -C aur push
   ```

`pkgver` must match the git tag. `CMAKE_BUILD_TYPE=None` is deliberate: Arch
supplies its own optimisation flags through `CXXFLAGS`, and a `Release` build
would override them.

## Test builds

A release tarball carries no repository, so `cdeez --version` reports a bare
version and two test builds look alike. Pass the commit in when handing builds
round for testing, so a bug report names the build it came from:

```sh
CDEEZ_REVISION=$(git describe --always --dirty --abbrev=8) makepkg -f
```

Leave it unset for the real release: there the tag is what identifies the
build.

## Expected namcap output

`namcap PKGBUILD` is clean. `namcap` on the built package reports three
warnings that cannot be cleared:

```
W: Dependency libstdc++ detected and implicitly satisfied
W: Dependency libgcc detected and implicitly satisfied
W: Dependency included, but may not be needed ('gcc-libs')
```

These are informational. `gcc-libs` and `glibc` are listed explicitly because
that is the convention for compiled packages; dropping them simply trades these
warnings for an equivalent set about `glibc`. Verified both ways in a clean
container.
