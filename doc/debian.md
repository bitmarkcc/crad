To install on debian-based Linux distros:

1) First, follow the installation [guide for radicle](https://radicle.xyz/download).

2) Next, add our [PGP key](https://cradicle.xyz/cradicle.gpg) to your keyring

`curl -fsSL https://deb.cradicle.xyz/key.gpg | sudo gpg --dearmor -o /usr/share/keyrings/cradicle.gpg`

3) Next, add our repo to your list

`echo "deb [signed-by=/usr/share/keyrings/cradicle.gpg] https://deb.cradicle.xyz trixie main" | sudo tee /etc/apt/sources.list.d/cradicle.list`

For **Tails** use `tor+https` instead of `https` in the url

4) Update APT: `sudo apt update`

5. Install cradicle: `sudo apt install cradicle`

To build from source
```
sudo apt build-dep libssh-rad
apt source --compile libssh-rad
sudo apt install ./libssh-rad*.deb
sudo apt build-dep cradicle
apt source --compile cradicle
sudo apt install ./cradicle*.deb
```

6) As regular user, you can now run `cradicle-install` and follow the instructions there.

For **Tails**, add the following to $RAD_HOME/config.json (default $HOME/.radicle/config.json), in the "node" object:
```
"proxy": "10.0.2.2:9050",
"onion": {
  "mode": "proxy",
  "address": "10.0.2.2:9050"
}
```
