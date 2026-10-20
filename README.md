# netdiag

A small line-oriented helper for network diagnostics, meant to run behind
`inetd`, `socat`, or an SSH forced command so that operators can check
reachability from a server without a shell on it.

```
$ ./netdiag
netdiag ready
LOOKUP localhost
OK localhost 127.0.0.1
CHECK 127.0.0.1 22
OK 127.0.0.1:22 closed
QUIT
```

## Commands

| Command | Effect |
|---|---|
| `PING <host>` | sends one ICMP echo with the system `ping`, and relays its output |
| `LOOKUP <host>` | resolves a host name to its addresses |
| `CHECK <host> <port>` | tries a TCP connection with a 2-second timeout |
| `HELP` | lists the commands |
| `QUIT` | ends the session |

Every reply starts with `OK` or `ERR`; relayed output lines start with `| `.
Lines longer than 511 bytes are rejected.

## Building

`make` builds `./netdiag`; `make test` runs the tests. `PING` needs `ping`
(iputils) at run time.
