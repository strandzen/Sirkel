# Maintainer: Strandzen <strandzen@gmail.com>
pkgname=sirkel-git
_pkgname=Sirkel
pkgver=0.5.0
pkgrel=1
pkgdesc="Loop-inspired directional window snapping for KDE Plasma (KWin effect)"
arch=('x86_64')
url="https://github.com/strandzen/Sirkel"
license=('GPL-2.0-or-later')
depends=('kwin' 'qt6-base' 'qt6-declarative' 'kconfig' 'kcolorscheme' 'kcoreaddons'
         'kglobalaccel' 'ki18n' 'kcmutils' 'kwindowsystem' 'kirigami' 'kdeclarative')
optdepends=('keyd: use Caps Lock as the trigger (otherwise pick a key combination in the settings)')
makedepends=('git' 'cmake' 'extra-cmake-modules')
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
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=OFF
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
