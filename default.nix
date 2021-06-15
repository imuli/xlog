{ pkgs ? import <nixpkgs> {}
, stdenv ? pkgs.stdenv
, lib ? pkgs.lib
, xorg ? pkgs.xorg
}:
stdenv.mkDerivation {
  name = "xlog";
  version = "0.1";
  src = lib.cleanSource ./.;
  buildInputs = [ xorg.libX11 xorg.libXi xorg.libXext ];
  installPhase = ''
    mkdir -p $out/bin
    cp bin/* $out/bin/
  '';
  meta = {
    homepage = "https://github.com/imuli/xlog";
    license = lib.licenses.unlicense;
    platforms = lib.platforms.unix;
    maintainers = with lib.maintainers; [ imuli ];
  };
}
