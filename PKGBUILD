# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later
# Maintainer: Strandzen <strandzen@gmail.com>
#
# Builds the latest commit on GitHub. For the latest release, use
# packaging/sirkel/PKGBUILD.
pkgname=sirkel-git
_pkgname=Sirkel
pkgver=0.5.2
pkgrel=1
pkgdesc="Loop-inspired directional window snapping for KDE Plasma (KWin effect)"
arch=('x86_64' 'aarch64')
url="https://github.com/strandzen/Sirkel"
license=('GPL-2.0-or-later')
depends=('kwin' 'qt6-base' 'qt6-declarative' 'kconfig' 'kcolorscheme' 'kcoreaddons'
         'kglobalaccel' 'ki18n' 'kcmutils' 'kwindowsystem' 'kirigami' 'kdeclarative')
optdepends=('keyd: use Caps Lock as the trigger (otherwise pick a key combination in the settings)')
makedepends=('git' 'cmake' 'extra-cmake-modules' 'vulkan-headers')
provides=('sirkel')
conflicts=('sirkel')
source=("git+${url}.git")
sha256sums=('SKIP')

pkgver() {
  cd "$_pkgname"
  # project() version from CMakeLists.txt, plus the commit count and hash.
  local base
  base=$(sed -n 's/^project(sirkel VERSION \([0-9.]*\)).*/\1/p' CMakeLists.txt)
  printf "%s.r%s.g%s" "$base" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cmake -B build -S "$_pkgname" \
    -DCMAKE_BUILD_TYPE=None \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=ON
  cmake --build build
}

check() {
  QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
}

package() {
  DESTDIR="$pkgdir" cmake --install build
  # KWin only loads effects built for its exact version: remind to rebuild.
  install -Dm644 "$_pkgname/packaging/sirkel/sirkel-rebuild.hook" "$pkgdir/usr/share/libalpm/hooks/sirkel-rebuild.hook"
}
