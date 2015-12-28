pkgname=teerank
pkgver=$(git describe --long | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g')
pkgrel=1
pkgdesc="Fast and simple ranking system for Teeworlds"
arch=('i686' 'x86_64')
url="http://teerank.com/"
license=("GPL3")
source=()

prepare() {
	ln -snf "$startdir" "$srcdir/$pkgname"
}

build() {
	cd "$pkgname"
	make
}

package() {
	cd "$pkgname"
	make prefix="$pkgdir" install
}
