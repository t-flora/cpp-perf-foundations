# Day 7 Analysis: Where a Round Trip Goes

**Setup**: Apple M2 (4 performance + 4 efficiency cores), macOS 15.7, Apple Clang 17, Release. Linux runs: Ubuntu 24.04 arm64 + GCC 13 in Docker Desktop's VM (kernel 6.10, 8 vCPUs), client and server both inside the container. All traffic is loopback (127.0.0.1). Each configuration ran 3 times; tables show medians. The host was busy (load average ~3.5, a browser using ~2.5 cores). This Mac also runs **Cisco Secure Client**, whose socket-filter network extension is active (question 1). Raw data: [`macos.json`](macos.json), [`linux-docker.json`](linux-docker.json), plot: [`echo.png`](echo.png).

The client is closed-loop: each connection keeps a fixed number of requests in flight and sends the next one only when a response arrives. Latency is the round trip of one 64-byte request (68 bytes on the wire with its header) unless stated.

## 1. What Does One Round Trip Cost, and Where Does It Go?
`transport_probe` ping-pongs one byte between two threads. With `block`, the reader sleeps in `read()`; with `spin`, it polls a non-blocking socket and never sleeps.

| p50 round trip | pipe | Unix socket | TCP loopback |
|---|---|---|---|
| macOS, block | 4.4 µs | 4.6 µs | **84 µs** |
| macOS, spin | 2.1 µs | 2.6 µs | **93 µs** |
| Linux VM, block | **42 µs** | **39 µs** | 50 µs |
| Linux VM, spin | 1.3 µs | 2.4 µs | 4.5 µs |

The two environments are slow for different reasons:
- **Linux VM: waking a thread is the cost.** Every blocking transport costs ~40 µs, even a pipe, which involves no networking at all. Spinning drops TCP to 4.5 µs. An idle virtual CPU is halted, and waking it goes through the hypervisor. The TCP stack itself costs ~2 µs per round trip more than a Unix socket.
- **macOS: TCP itself is the cost.** Thread wakeups are cheap natively (4.4 µs for a pipe), and spinning doesn't help TCP at all. Splitting one TCP hop shows where the time goes: `send()` takes **17.7 µs** (0.54 µs for a Unix socket), and the byte becomes readable **29.5 µs** after `send()` returns (0.5 µs). Both are kernel time (`time ./transport_probe`: 4.0 s of system time against 0.8 s of user time). On Linux the same split is 1.8 + 1.8 µs.
- **The macOS delivery step is shared system-wide.** With four copies of the one-way probe running at once, `send()` stays at 18 µs but delivery grows from 30 to 79 µs. Something asynchronous after `send()` processes loopback data for every process in turn.
- **What that something is, I haven't confirmed.** Cisco Secure Client's socket filter is the main suspect: macOS content filters hook TCP and UDP in the kernel but not pipes or Unix sockets, which is exactly the split measured. Its user-space process used 0% CPU under load, so it isn't inspecting every packet; a kernel hook could still be on the path. The alternative is macOS's own loopback path. Telling them apart means rerunning `transport_probe` with the filter disabled, or on a Mac without it. I didn't touch it here.

## 2. When Does Message Size Matter?

| Payload | 64 B | 4 KB | 32 KB | 256 KB | 1 MB |
|---|---|---|---|---|---|
| macOS p50 | 75 µs | 76 µs | 143 µs | 611 µs | 2.25 ms |
| Linux VM p50 | 49 µs | 49 µs | 61 µs | 116 µs | 371 µs |

Up to 4 KB, size is free: fixed per-message costs dominate, and a 4 KB request costs the same as a 64-byte one on both systems. Beyond that, copying takes over. One connection echoes 1 MB in 2.25 ms on macOS (441 MiB/s of payload in each direction) and in 0.37 ms on Linux (2.6 GiB/s), **6× faster**. So the macOS overhead from question 1 is not only per packet; it also scales with bytes.

The practical rule: **batch small messages**. Sending 64 messages of 64 bytes one at a time costs 64 round trips; sending them as one 4 KB write costs one.

## 3. Nagle's Algorithm and Delayed ACKs

| p50 round trip, 64 B | macOS | Linux VM |
|---|---|---|
| `TCP_NODELAY`, one `send()` per request | 75 µs | 44 µs |
| `TCP_NODELAY`, header and payload in two `send()`s | 94 µs | 49 µs |
| Nagle on, one `send()` | 77 µs | 48 µs |
| Nagle on, two `send()`s | 117 µs | **41,411 µs** |

On Linux, the last cell is **850× slower**: 24 round trips per second. The interaction:
1. The client sends the 4-byte header, which goes out immediately because nothing is unacknowledged.
2. It sends the payload. Nagle's algorithm holds a small segment back while earlier data is unacknowledged, so the payload waits for the header's ACK.
3. The server can't answer a header alone: it needs the whole frame. So there's no response to carry the ACK, and the server delays the ACK, hoping one comes along. Linux's minimum delayed-ACK timeout is 40 ms.
4. 40 ms later the ACK goes out, the payload follows, and the server answers.

Each mechanism makes sense alone; together they deadlock until a timer fires. Either fix works: `TCP_NODELAY`, or one write per message (build the frame in one buffer, or use `writev`). The client here does both by default.

**macOS doesn't stall on loopback** (117 µs). I didn't establish why; its delayed-ACK policy (`net.inet.tcp.delayed_ack = 3`) acknowledges sooner in some cases. That makes this bug easy to ship: it doesn't show up on a developer's Mac, then costs 40 ms per request on a Linux server. Even without the stall, the second `send()` costs macOS 19 µs, about one `send()` call (question 1). Small writes cost syscalls even when Nagle stays out of the way.

## 4. Pipelining and Little's Law

| Requests in flight | 1 | 4 | 8 | 16 | 128 |
|---|---|---|---|---|---|
| macOS round trips/s | 12.9K | 31.6K | 29.5K | 30.0K | 29.5K |
| macOS p50 | 75 µs | 121 µs | 271 µs | 536 µs | 4.36 ms |
| Linux round trips/s | 19.8K | 77.4K | 234K | 367K | 371K |
| Linux p50 | 50 µs | 50 µs | **25 µs** | 43 µs | 339 µs |

- **Little's law holds exactly.** Requests in flight = throughput × latency. At depth 128 on macOS, 128 / 29.5K/s = 4.35 ms, against a measured 4.36 ms. On Linux, 128 / 371K/s = 345 µs, against 339 µs. The dotted lines in the plot are that prediction; they sit on the measurements.
- **Once throughput plateaus, more depth only adds queueing.** Past depth 4 (macOS) or 16 (Linux), every extra request in flight adds latency and no throughput. The useful depth is the smallest one that reaches the plateau.
- **On Linux, latency *drops* at depth 8** (50 → 25 µs) while throughput triples. With enough requests in flight, the client and server threads always have work and never sleep, so nobody pays the 40 µs wakeup from question 1. The same thing happens with 8 connections (question 5). In a VM, a busier system can be a faster one.
- macOS plateaus at ~30K round trips/s, the ceiling from question 1.

## 5. Thread per Connection vs. a `poll()` Loop
Total round trips per second, each connection with one request in flight:

| Connections | 1 | 4 | 8 | 16 | 32 | 128 |
|---|---|---|---|---|---|---|
| macOS, threads | 12.9K | 30.9K | 31.6K | 31.5K | 28.1K | 24.7K |
| macOS, poll | 12.4K | 31.3K | 31.6K | 31.8K | 21.6K | 27.3K |
| Linux, threads | 19.9K | 59.2K | 256K | 500K | 693K | 711K |
| Linux, poll | 19.6K | 86.9K | 86.2K | 89.6K | 88.9K | 101K |

- **On macOS the design doesn't matter.** Both hit the same ~31K ceiling, set by the kernel (question 1), not by the server. Comparing server designs on this machine would measure the TCP path, not the servers.
- **On Linux, the `poll()` loop is one core.** It plateaus at ~90K from 4 connections on, about 11 µs of server thread time per round trip: at least three system calls each (`poll`, `recv` until it would block, `send`), on one thread. Thread per connection spreads the same work over all 8 vCPUs and reaches 711K, **7× more**.
- **So why use an event loop?** For connection count, not throughput per connection. A thread costs a stack (512 KB to 8 MB of reserved address space) and scheduler work; 10,000 mostly-idle connections are 10,000 threads but one `poll` set. And `poll()` itself is O(connections) per call, which `epoll` (Linux) and `kqueue` (macOS) fix. Production servers combine the two: one event loop per core.
- Thread per connection's tail gets worse with load: on Linux at 128 connections, p99 is 627 µs and max 29 ms, with 256 threads on 8 vCPUs.

## 6. Is It the Network, or the Machine?
None of this traffic touched a network card, yet loopback latency ranged from 4.5 µs (spinning TCP in the VM) to 84 µs (blocking TCP on macOS), for reasons that had nothing to do with networking. Thread wakeups under virtualization, a kernel-side delivery stage shared system-wide, and delayed-ACK policy each dominated somewhere.
- **Before blaming the network, measure the floor on loopback.** It's the cost of your stack, OS, and threading, with the network removed.
- **Benchmark on the OS you deploy on.** The Nagle stall only exists on one of the two systems here; the macOS ceiling only on the other.
- A real network adds propagation delay (~1 ms of round trip per 100 km of fibre), a NIC, interrupt handling, and loss and retransmission. Run the client on a second machine to see it.

## 7. Methodology Notes
- **Closed-loop clients understate tail latency under load.** When the server stalls, a closed-loop client stops sending, so the requests that *would* have arrived during the stall are never sent and never measured. This is *coordinated omission*. For latency-under-load numbers, send at a fixed rate and measure from each request's scheduled send time (wrk2 does this). The tails in question 5 are therefore optimistic.
- **Throughput includes warmup**: it's all messages over the whole run; percentiles exclude the first 10% of each connection's requests.
- **The clock is fine here**: `steady_clock` has 41.7 ns resolution (Day 5), small against round trips of tens of microseconds.
- **Noise**: most configurations have a CV under 5% across repetitions. The exceptions are Linux with 8–32 connections (12–25%), which sits right where vCPUs start staying awake, and macOS with 32 connections (5–7%).

## 8. Robustness
`test_echo_server` checks both servers against misbehaving clients:
- **A 2 GB length prefix**: the server disconnects that client instead of trying to buffer 2 GB, and keeps serving everyone else. Every length from the network gets a limit (16 MB here).
- **A 4 MB frame**, larger than the socket buffers in both directions: echoed intact. That exercises partial sends, and in the poll server the `POLLOUT` path.
- **A client that sends but doesn't read**: in the poll server, once 1 MB of responses is waiting, it stops reading that client. That's backpressure, as in Day 6's bounded queue. Without it, one slow client grows the server's memory without limit. The thread-per-connection server gets backpressure for free: its blocking `send()` stops that thread from reading.
- **`stop()` with idle clients connected** returns in milliseconds. `shutdown()` wakes threads blocked in `recv()`, and a self-pipe wakes `poll()`.
