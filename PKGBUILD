# This PKGBUILD is intented for our github workflows. For building your custom packages use the file PKGBUILD_AUR

# Contributor: Wellington <wellingtonwallace@gmail.com>
# Maintainer: Upstream https://github.com/wwmm/easyeffects 

pkgname=easyeffects-git
pkgver=pkgvernotupdated
pkgrel=1
pkgdesc='Audio Effects for PipeWire Applications'
arch=(x86_64)
url='https://github.com/wwmm/easyeffects'
license=('GPL3')
depends=('libadwaita' 'pipewire-pulse' 'lilv' 'libsigc++-3.0' 'libsamplerate' 'zita-convolver' 
         'libebur128' 'rnnoise' 'soundtouch' 'libbs2b' 'nlohmann-json' 'tbb' 'gsl' 'speexdsp')
makedepends=('meson' 'itstool' 'appstream-glib' 'git' 'mold' 'ladspa')
optdepends=('calf: limiter, exciter, bass enhancer and others'
            'lsp-plugins: equalizer, compressor, delay, loudness'
            'zam-plugins: maximizer'
            'mda.lv2: bass loudness'
            'yelp: in-app help'
            'libdeep_filter_ladspa: noise remover')
conflicts=(easyeffects)
provides=(easyeffects)
replaces=('pulseeffects')
sha512sums=()

pkgver() {
  description=$(git describe --long | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g')
  # if in github actions environment
  if test -f "../GITHUB_COMMIT_DESC"; then 
    # remove last commit from git describe output (which may sometimes be a merge commit),
    # and replace it with a human friendly version
    description_short=$(echo "$description" | sed -r 's/(.*)\..*/\1/')
    github_commit_desc_no_hyphen=$(sed 's/-/./g' ../GITHUB_COMMIT_DESC)
    printf "%s" "${description_short}.${github_commit_desc_no_hyphen}"
  else
    printf "%s" "$description"
  fi
}

build() {
  cd ..
  # set werror to true if the CI file exists, otherwise false
  # arch-meson sets --buildtype plain by default, so don't set -Dbuildtype=debug 
  arch-meson . build -Ddevel=true -Dwerror="$( test -f "./GITHUB_COMMIT_DESC" && echo "true" || echo "false")"

  ninja -C build
}

package() {
  cd ..
  DESTDIR="${pkgdir}" ninja install -C build
}
