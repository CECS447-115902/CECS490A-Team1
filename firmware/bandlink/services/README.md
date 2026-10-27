# services/

Hardware-independent logic. Must build on the host (see `firmware/tests/`).

| Folder | Responsibility |
|---|---|
| `link/` | Packet codec, GPS-timed TDMA slots, dedupe, relay queue (S2) |
| `crypto/` | AES-128-CCM wrapper (HW AES on target, software on host) |
| `nav/` | Peer table, haversine distance, bearing, heading from quaternion |
| `pairing/` | Group key + device ID exchange, key store |
| `ranging/` | DS-TWR session logic (S2) |
| `battery/` | SOC, charge state, low-battery mode |
| `haptics/` | Named vibration patterns |

Never include a HAL, CubeMX, or `bsp/` board header here.
