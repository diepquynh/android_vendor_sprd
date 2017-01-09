autoreconf -i -f
./configure --prefix=$HOME/vpu/mmf/openmax/out
echo export OMX_BELLAGIO_REGISTRY=$HOME/vpu/mmf/openmax/out/.omxregister >> $HOME/.profile
echo export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$HOME/vpu/mmf/openmax/out/lib/pkgconfig  >> $HOME/.profile
source ~/.profile

