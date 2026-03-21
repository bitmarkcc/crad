To install on debian-based Linux distros:

1) Add our [PGP key](https://cradicle.xyz/cradicle.gpg) to your keyring

`curl -fsSL https://deb.cradicle.xyz/key.gpg | sudo gpg --dearmor -o /usr/share/keyrings/cradicle.gpg`

2) Next, add our repo to your list

`echo "deb [signed-by=/usr/share/keyrings/cradicle.gpg] https://deb.cradicle.xyz trixie main" | sudo tee /etc/apt/sources.list.d/cradicle.list`

For **Tails** use `tor+https` instead of `https` in the url

3) Update APT: `sudo apt update`

4) Install cradicle: `sudo apt install cradicle`

To build from source
```
sudo apt build-dep libssh-rad
apt source --compile libssh-rad
sudo apt install ./libssh-rad*.deb
sudo apt build-dep cradicle
apt source --compile cradicle
sudo apt install ./cradicle*.deb
```

5) As regular user, you can now run `cradicle-install` and follow the instructions there.

For **Tails**, add the following to $RAD_HOME/config.json (default $HOME/.radicle/config.json), in the "node" object, after you create an identity:
```
"proxy": "10.0.2.2:9050",
"onion": {
  "mode": "proxy",
  "address": "10.0.2.2:9050"
}
```
