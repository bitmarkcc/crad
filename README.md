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
- basic Linux utilities like rsync

radicle-heartwood will run inside a bubblewrap container for security purposes.

Installation:

`make`
`make install`
Add $RAD_HOME/bin ($HOME/.radicle/bin by default) and $CRAD_HOME/bin ($HOME/.cradicle/bin) to your $PATH. Put $CRAD_HOME/bin before $RAD_HOME/bin, so that git-remote-rad comes from $CRAD_HOME/bin.

Usage:

`crad -h` for list of options
See the project rad-gui (rad:zVKd2asJa4AzDkQ3VQSoxanBipAX) for a graphical interface
