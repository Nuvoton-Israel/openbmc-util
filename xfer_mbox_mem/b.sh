./bootstrap.sh
./configure
sudo make
sudo make install
sudo cp tools/xfer_mbox_mem ./xfer_mbox_mem
sudo chmod +x xfer_mbox_mem
