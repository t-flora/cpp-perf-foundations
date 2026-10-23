# Day 7: TCP Echo Server and Load Generator
**Goal**: Learn socket programming, and find out what a network round trip costs before the network is even involved

You'll build an echo server and a client that loads it and times every round trip, then run experiments that separate what TCP, your OS, and your write patterns each cost. All of it runs on loopback (127.0.0.1), with no network card involved, and the numbers will still vary by more than 10× between configurations. Readings are listed under Day 7 in the [main README](../README.md#day-7-tcp-echo-server-and-load-generator).

POSIX sockets: Linux, macOS, or WSL.

## Deliverables

```
day7-tcp-echo/
├── CMakeLists.txt                # Starter skeleton provided
├── include/net/
│   ├── socket.hpp                # Starter skeleton provided
│   ├── framing.hpp               # Starter skeleton provided
│   └── echo_server.hpp           # Starter skeleton provided
├── src/
│   ├── echo_server.cpp           # CLI: --port, prints the port, stops on SIGINT/SIGTERM
│   └── echo_client.cpp           # The load generator
├── tests/
├── scripts/                      # Run the experiments, plot the results
├── Dockerfile                    # The same experiments on Linux (Day 3)
└── results/ANALYSIS.md
```

## Milestones

### 1. Sockets (~1h)
Fill in `socket.hpp`: an RAII `Fd` that closes its descriptor exactly once, plus `listen_tcp`, `connect_tcp`, `accept_tcp`, `send_all`, `recv_some`, and `recv_exact`.
- `send()` may write only part of the buffer, and `recv()` may return only part of what was sent. Which of your functions loop, and on what?
- What happens to your process if it writes to a socket whose peer has closed it? Make that an error instead.
- Listen on port 0 and read the port back with `getsockname`, so tests never collide.

Test: 8 MB through one connection (more than the socket buffers, so the sender must loop while the receiver reads), EOF after the peer closes, and `ECONNREFUSED` when nothing listens.

### 2. Framing (~1h)
TCP delivers a byte stream, not messages. Prefix each message with its length (4 bytes, big-endian) and write a `FrameDecoder` that accepts bytes in any split and hands out complete payloads.
- Test it with many random frames cut at random points, and one byte at a time.
- A peer sends a header claiming 2 GB. What should happen?

### 3. Echo Server (~1.5h)
`EchoServer` with a thread per connection: receive, decode, and send every complete frame straight back. Answer all the frames from one `recv()` with one `send()`.
- `stop()` must return promptly even while clients sit idle on open connections. What wakes a thread blocked in `accept()` or `recv()`?
- A thread whose client disconnected has finished. When do you join it, and when do you close its socket?
- Test: several concurrent clients with requests pipelined, a frame larger than the socket buffers, a client sending a bad length (it gets dropped; everyone else keeps being served), and `stop()` with idle clients connected. Run the tests under ThreadSanitizer too (Day 6).

### 4. Load Generator (~1.5h)
`echo_client --port P --connections C --depth D --size S --messages M --warmup W`:
- C connections, each on its own thread, each keeping D requests in flight. Start them together.
- Time every round trip from send to response, and report throughput plus p50/p90/p99/p99.9/max, excluding warmup. Print JSON so a script can collect it.
- Check every response: right size, and back in order (put a sequence number in the payload).
- `--nagle` leaves Nagle's algorithm on (default: `TCP_NODELAY`). `--split` sends each request's header and payload with two `send()` calls.

### 5. Experiments and Analysis (~1.5h)
Script the runs (start a server, run each configuration 3 times, save the JSON), then run them natively *and* in a Linux container. Plot round-trip time vs. size, and throughput vs. connections and vs. depth. In `results/ANALYSIS.md`, answer each question with numbers:
1. What does a 64-byte round trip cost? Compare it with a 1-byte ping-pong between two threads over a pipe and a Unix-domain socket, with blocking reads and with spinning non-blocking reads. Where does the time go on each OS?
2. Sweep the size from 64 B to 1 MB. Up to what size is the payload free? What bandwidth does one connection reach?
3. Measure all four combinations of Nagle on/off × one/two writes per request, on both OSes. Explain any result over a millisecond step by step: who is waiting for whom?
4. Sweep the depth from 1 to 128 on one connection. Where does throughput plateau? Check Little's law: requests in flight = throughput × latency.
5. Sweep connections from 1 to 128. Where does total throughput stop growing, and what limits it there?
6. Your client sends a request only after an earlier one returns. What does that hide about latency when the server stalls? (Look up *coordinated omission*.)

## Done When
- [ ] `ctest` passes, and passes again under ThreadSanitizer with zero reports
- [ ] The server survives a client that sends a huge length, and one that disconnects mid-frame
- [ ] `ANALYSIS.md` answers every question with numbers from both OSes

## If Time Permits
- A single-threaded `poll()` server with non-blocking sockets: buffer partial writes, wait for `POLLOUT`, and stop reading from a client whose responses pile up (backpressure). Compare it with thread per connection as connections grow. What does each design buy?
- An open-loop client that sends at a fixed rate and measures from each request's *scheduled* send time. How do the tails change?
- Run the client on a second machine. What does a real network add?
- A UDP echo: what do you give up, and what does it cost?
