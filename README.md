C implementation of Radicle CLI and Node

Dependencies:

- Rust-based radicle (heartwood): https://seed.radicle.garden/z3gqcJUoA1n9HaHKufZs5FCSGazv5.git
- A custom verison of libssh: rad:z3azMM7wahAi6AKgfhynEnZ5XzrSq
- libgit2
- libjson-c
- libsqlite3
- git
- sshd
- Bubblewrap
- tor
- torsocks
- slirp4netns
- python3 (todo: remove this need)
- bash
- basic Linux utilities like rsync

The radicle-heartwood node will run inside a bubblewrap container for security purposes.

Installation: 
 
`make` 
`make install` 
Add $RAD_HOME/bin ($HOME/.radicle/bin by default) and $CRAD_HOME/bin ($HOME/.cradicle/bin) to your $PATH. Put $CRAD_HOME/bin before $RAD_HOME/bin, so that git-remote-rad comes from $CRAD_HOME/bin.
Add $CRAD_HOME/lib to your $LD_LIBRARY_PATH.

Usage:

`crad -h` for list of options
See the project rad-gui (rad:zVKd2asJa4AzDkQ3VQSoxanBipAX) for a graphical interface

Private Repos:
 
Running radicle-node-wrapped will start an SSH server listening on port 8777. You can then run this as a Tor hidden service by adding the line`HiddenServicePort 8777` to your torrc, under the `HiddenService Dir <dir>` line where `<dir>` is the hidden service directory. Upon restarting tor, this directory should automatically be populated with a keypair and a `hostname` file that contains the onion address for your service.

In order for another peer to sync or clone from your private repo, you must add them to the allow list via `crad id update` and they must run the `crad clone` or `crad sync` command with the `-s` argument with the value `<user>@myonionaddress.onion:8777`, where <user> is the Linux user name running for the `radicle-node-wrapped`.
